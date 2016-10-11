#include "cpp_redis/network/io_service.hpp"
#include "cpp_redis/redis_error.hpp"

#ifdef _MSC_VER
#include <io.h>					//needed for fcntl, open etc.
#pragma warning(disable:4996)	//Disable "The POSIX name for this item is deprecated" warnings
#endif

namespace cpp_redis {

namespace network {

io_service&
  io_service::get_instance(void) {
  static io_service instance;
  return instance;
}

#ifdef _MSC_VER
io_service::io_service(size_t max_worker_threads /*= MAX_WORKER_THREADS*/)
  : m_should_stop(false)
{
   //Determine the size of the thread pool dynamically.
   //2 * number of processors in the system is our rule here.
   SYSTEM_INFO info;
   ::GetSystemInfo(&info);
   m_worker_thread_pool_size = (info.dwNumberOfProcessors * 2);
  
  if (m_worker_thread_pool_size > MAX_WORKER_THREADS)
    m_worker_thread_pool_size = MAX_WORKER_THREADS;

  WSADATA wsaData;
  int nRet = 0;
  if ((nRet = WSAStartup(0x202, &wsaData)) != 0)  //Start winsock before any other socket calls.
    throw cpp_redis::redis_error("Could not init cpp_redis::io_service, WSAStartup() failure");

  //Create completion port.  Pass 0 for parameter 4 to allow as many threads as there are processors in the system
  m_completion_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);;
  if( INVALID_HANDLE_VALUE == m_completion_port)
    throw cpp_redis::redis_error("Could not init cpp_redis::io_service, CreateIoCompletionPort() failure");

  //Now startup worker thread pool which will service our async io requests
  for (unsigned int i=0; i<m_worker_thread_pool_size; i++) {
    m_worker_threads.push_back(std::thread(&io_service::process_io, this));
  }
}

io_service::~io_service(void)
{
  m_should_stop = true;

  if (m_completion_port)
  {
    CloseHandle(m_completion_port); //This will cause the IO worker threads to shutdown.
    m_completion_port = NULL;
  }

  // Wait for the threads to finish
  for (auto& t : m_worker_threads)
    t.join();
}

void 
io_service::shutdown()
{
  m_should_stop = true;

  //close the completion port otherwise the worker threads will all be waiting on GetQueuedCompletionStatus()
  if (m_completion_port)
  {
    CloseHandle(m_completion_port);
    m_completion_port = NULL;

    //Sleep for 1 second so all worker threads can shut down gracefully
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

//! add or remove a given socket from the io service
//! untrack should never be called from inside a callback
void 
io_service::track(SOCKET sock, const disconnection_handler_t& handler)
{
  std::lock_guard<std::recursive_mutex> lock(m_socket_mutex);
  
  //Add the socket to our map and return the allocated struct
  auto& info = m_sockets[sock];
  
  memset(&info.io_info.overlapped, 0, sizeof(OVERLAPPED));
  info.hsock = sock;
  info.async_read = false;
  info.async_write = false;
  info.disconnection_handler = handler;

  //Associate the socket with our io completion port.
  if( NULL == CreateIoCompletionPort((HANDLE)sock, m_completion_port, (DWORD_PTR)&m_sockets[sock], 0))
  {
    printf("CreateIoCompletionPort() failed: %d\n", GetLastError());
    throw cpp_redis::redis_error("Track() failed to create CreateIoCompletionPort");
  }

  printf("Socket(%d) added to IOCP ok\n", sock);
}

void 
io_service::untrack(SOCKET sock)
{
  auto sock_it = m_sockets.find(sock);
  if (sock_it == m_sockets.end())
    return;
  auto& sockinfo = sock_it->second;

  //Wait until the posted i/o has completed. 
  //while (m_completion_port && !HasOverlappedIoCompleted((LPOVERLAPPED)&sockinfo.io_info.overlapped))
  //   std::this_thread::sleep_for(std::chrono::milliseconds(200));

  std::lock_guard<std::recursive_mutex> lock(m_socket_mutex);
  m_sockets.erase(sock);
}

bool
io_service::async_read(SOCKET sock, std::vector<char>& buffer, std::size_t read_size, const read_callback_t& callback) {
  std::lock_guard<std::recursive_mutex> lock(m_socket_mutex);

  auto sock_it = m_sockets.find(sock);
  if (sock_it == m_sockets.end())
    return false;

  auto& sockinfo = sock_it->second;
  bool expected = false;
  if (!sockinfo.async_read.compare_exchange_strong(expected, true))
    return false;

  //Resize the buffer to the correct size if we need to.
  if (buffer.size() < read_size)
    buffer.resize(read_size);

  sockinfo.read_buffer = &buffer;
  sockinfo.read_size = read_size;
  sockinfo.read_callback = callback;
  sockinfo.io_info.eOperation = IO_OP_READ;

  DWORD dwRecvNumBytes = 0;
  DWORD dwFlags = 0;
  WSABUF buffRecv;

  buffRecv.buf = sockinfo.read_buffer->data();
  buffRecv.len = sockinfo.read_size;

  int nRet = WSARecv(sock, &buffRecv, 1, &dwRecvNumBytes, &dwFlags, &sockinfo.io_info.overlapped, NULL);

  if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
  {
    printf("WSARecv() failed: %d Closing socket\n", WSAGetLastError());

    //Fire the disconnect handler
    sockinfo.disconnection_handler(*this);
    return false;
  }

  return true;
}

bool
io_service::async_write(SOCKET sock, const std::vector<char>& buffer, std::size_t write_size, const write_callback_t& callback) {
  std::lock_guard<std::recursive_mutex> lock(m_socket_mutex);

  auto sock_it = m_sockets.find(sock);
  if (sock_it == m_sockets.end())
    return false;

  auto& sockinfo = sock_it->second;
  bool expected = false;
  if (!sockinfo.async_write.compare_exchange_strong(expected, true))
    return false;

  sockinfo.write_buffer = buffer;
  sockinfo.write_size = write_size;
  sockinfo.write_callback = callback;
  sockinfo.io_info.eOperation = IO_OP_WRITE;

  auto& sbuffer = sockinfo.write_buffer;

  WSABUF buffSend;
  DWORD dwSendNumBytes = 0;
  DWORD dwFlags = 0;
  buffSend.buf = sockinfo.write_buffer.data();
  buffSend.len = write_size;

  int nRet = WSASend(sock, &buffSend, 1, &dwSendNumBytes, dwFlags, &(sockinfo.io_info.overlapped), NULL);
  if (SOCKET_ERROR == nRet && (ERROR_IO_PENDING != WSAGetLastError()))
  {
    printf("WSASend() failed: %d\n", WSAGetLastError());

    //Fire the disconnect handler
    sockinfo.disconnection_handler(*this);
    return false;
  }

  return true;
}

//function used by worker thread(s) used to process io requests
int
io_service::process_io(void)
{
  BOOL bSuccess = FALSE;
  int nRet = 0;
  LPWSAOVERLAPPED     pOverlapped = NULL;
  sock_info*          pPerSocketInfo = NULL;
  
  WSABUF buffRecv;
  WSABUF buffSend;
  DWORD dwRecvNumBytes = 0;
  DWORD dwSendNumBytes = 0;
  DWORD dwFlags = 0;
  DWORD io_size = 0;

  while (!m_should_stop)
  {
    // continually loop to service io completion packets
    pPerSocketInfo = NULL;
    bSuccess = GetQueuedCompletionStatus(m_completion_port, &io_size, (PDWORD_PTR)&pPerSocketInfo, (LPOVERLAPPED *)&pOverlapped, INFINITE);
    if(!bSuccess && m_should_stop)
      return 0;

    //assert(bSuccess==TRUE, "GetQueuedCompletionStatus failed.");
   
    // First check to see if an error has occurred on the socket and if so
    // then close the socket and cleanup the SOCKET_INFORMATION structure
    // associated with the socket
    if (0 == io_size)
    {
      //Fire the disconnect handler. Check if stopped first because of some weird timing issues that may get us here.
      if(bSuccess && false == m_should_stop && pPerSocketInfo)
        pPerSocketInfo->disconnection_handler(*this);
      continue;
    }

    if (!pPerSocketInfo || !pOverlapped)
    {
      // Somebody used PostQueuedCompletionStatus to post an I/O packet with
      // a NULL CompletionKey (or if we get one for any reason).  It is time to exit.
      return 0;
    }

    //Lock our socket structure to prevent multiple access
    std::lock_guard<std::recursive_mutex> socklock(pPerSocketInfo->sock_info_mutex);
    auto& io_info = pPerSocketInfo->io_info;
  
    // determine what type of IO packet has completed by checking the PER_IO_CONTEXT 
    // associated with this socket.  This will determine what action to take.
    switch (io_info.eOperation)
    {
      case IO_OP_READ:
        // a read operation has completed
        printf("WorkerThread %d: Socket(%d) Received %d bytes\n", GetCurrentThreadId(), pPerSocketInfo->hsock, io_size);
        //TODO Resize Read buffer to the size read???
        pPerSocketInfo->read_callback(io_size);
      break;

      case IO_OP_WRITE:
        // a write operation has completed, determine if all the data intended to be
        // sent actually was sent.
        io_info.eOperation = IO_OP_WRITE;
        pPerSocketInfo->sent_bytes += io_size;
        dwFlags = 0;

        auto& buffer = pPerSocketInfo->write_buffer;

        // the previous write operation didn't send all the data,
        // post another send to complete the operation
        if (pPerSocketInfo->sent_bytes < pPerSocketInfo->write_size)
        {
          //Advance pointer past what we sent already and send the remaining.
          size_t size = pPerSocketInfo->write_size;
          buffSend.buf = buffer.data() + pPerSocketInfo->sent_bytes;
          buffSend.len = pPerSocketInfo->write_size - pPerSocketInfo->sent_bytes;

          nRet = WSASend(pPerSocketInfo->hsock, &buffSend, 1, &dwSendNumBytes, dwFlags, &(io_info.overlapped), NULL);

          if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
          {
            printf ("WSASend() failed: %d\n", WSAGetLastError());
            //Fire the disconnect handler
            pPerSocketInfo->disconnection_handler(*this);
          }
          else
          {
            printf("WorkerThread %d: Socket(%d) Partial send %d of %d bytes. Posting another send\n", GetCurrentThreadId(), pPerSocketInfo->hsock, pPerSocketInfo->sent_bytes, size);
            pPerSocketInfo->write_callback(io_size);
          }
        }
        else
        {
          // previous write operation completed for this socket, post another recv
          io_info.eOperation = IO_OP_READ;
          dwRecvNumBytes = 0;
          dwFlags = 0;
          buffRecv.buf = pPerSocketInfo->read_buffer->data();
          buffRecv.len = pPerSocketInfo->read_size;

          int nRet = WSARecv(pPerSocketInfo->hsock, &buffRecv, 1, &dwRecvNumBytes, &dwFlags, &io_info.overlapped, NULL);

          if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
          {
            printf ("WSARecv() failed. Error: %d\n", WSAGetLastError());
            //Fire the disconnect handler
            pPerSocketInfo->disconnection_handler(*this);
          }
          else
          {
            printf("WorkerThread %d: Socket(%d) Sent %d bytes, Posted Recv\n", GetCurrentThreadId(), pPerSocketInfo->hsock, io_size);
            pPerSocketInfo->write_callback(io_size);
          }
        }
        break;
    } //switch
  } //while

  return 0;
}

#else  //End of Windows only code

//Start of Linux code
io_service::io_service()
: m_should_stop(false)
{
#ifdef _MSC_VER
  WSADATA wsaData;
  int nRet = 0;
  if ((nRet = WSAStartup(0x202, &wsaData)) != 0)  //Start winsock before any other socket calls.
    throw cpp_redis::redis_error("Could not init cpp_redis::io_service, WSAStartup() failure");
#endif

  m_notify_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(m_notify_socket < 0)
    throw cpp_redis::redis_error("Could not init cpp_redis::io_service, socket() creation failure");

  u_long ulValue = 1;
  if (0 != ioctlsocket(m_notify_socket, FIONBIO, &ulValue))	//Set sockets to non blocking.
    throw cpp_redis::redis_error("Could not init cpp_redis::io_service, ioctlsocket() failure");

  m_worker = std::thread(&io_service::wait_for_events, this);
}

io_service::~io_service(void) {
  m_should_stop = true;
  notify_select();

  m_worker.join();
}

int
io_service::init_sets(fd_set* rd_set, fd_set* wr_set) {
  SOCKET max_sock = m_notify_socket;

  FD_ZERO(rd_set);
  FD_ZERO(wr_set);
  FD_SET(m_notify_socket, rd_set);

  std::lock_guard<std::recursive_mutex> lock(m_socket_mutex);
  for (const auto& sock : m_sockets) {
    if (sock.second.async_read)
      FD_SET(sock.first, rd_set);

    if (sock.second.async_write)
      FD_SET(sock.first, wr_set);

    if ((sock.second.async_read || sock.second.async_write) && sock.first > max_sock)
      max_sock = sock.first;
  }

  return max_sock;
}

io_service::callback_t
io_service::read_sock(SOCKET sock) {
  std::lock_guard<std::recursive_mutex> lock(m_socket_mutex);

  auto sock_it = m_sockets.find(sock);
  if (sock_it == m_sockets.end())
    return nullptr;

  auto& buffer = *sock_it->second.read_buffer;
  int original_buffer_size = buffer.size();
  buffer.resize(original_buffer_size + sock_it->second.read_size);

  int nb_bytes_read = recv(sock_it->first, buffer.data() + original_buffer_size, sock_it->second.read_size, 0);
  sock_it->second.async_read = false;

  if (nb_bytes_read <= 0) {
    buffer.resize(original_buffer_size);
    sock_it->second.disconnection_handler(*this);
    m_sockets.erase(sock_it);

    return nullptr;
  }
  else {
    buffer.resize(original_buffer_size + nb_bytes_read);

    return std::bind(sock_it->second.read_callback, nb_bytes_read);
  }
}

io_service::callback_t
io_service::write_sock(SOCKET sock) {
  std::lock_guard<std::recursive_mutex> lock(m_socket_mutex);

  auto sock_it = m_sockets.find(sock);
  if (sock_it == m_sockets.end())
    return nullptr;

  int nb_bytes_written = send(sock_it->first, sock_it->second.write_buffer.data(), sock_it->second.write_size, 0);
  sock_it->second.async_write = false;

  if (nb_bytes_written <= 0) {
    sock_it->second.disconnection_handler(*this);
    m_sockets.erase(sock_it);

    return nullptr;
  }
  else
    return std::bind(sock_it->second.write_callback, nb_bytes_written);
}

void
io_service::process_sets(fd_set* rd_set, fd_set* wr_set) {
  std::vector<SOCKET> socks_to_read;
  std::vector<SOCKET> socks_to_write;

  //! Quickly fetch fds that are readable/writeable
  //! This reduce lock time and avoid possible deadlock in callbacks
  {
    std::lock_guard<std::recursive_mutex> lock(m_socket_mutex);

    for (const auto& sock : m_sockets) {
      if (sock.second.async_read && FD_ISSET(sock.first, rd_set))
        socks_to_read.push_back(sock.first);

      if (sock.second.async_write && FD_ISSET(sock.first, wr_set))
        socks_to_write.push_back(sock.first);
    }
  }

  for (SOCKET sock : socks_to_read) {
    auto callback = read_sock(sock);
    if (callback)
      callback();
  }
  for (SOCKET sock : socks_to_write) {
    auto callback = write_sock(sock);
    if (callback)
      callback();
  }

  if (FD_ISSET(m_notify_socket, rd_set)) {
    char buf[1024];
    (void)read(m_notify_socket, buf, 1024);
  }
}

void
io_service::wait_for_events(void) {
  fd_set rd_set;
  fd_set wr_set;

  while (!m_should_stop) {
    int max_fd = init_sets(&rd_set, &wr_set);

    //Wait here until select returns and process any reads or writes that are pending.
    if (select(max_fd + 1, &rd_set, &wr_set, nullptr, nullptr) > 0)
      process_sets(&rd_set, &wr_set);
  }
}

void
io_service::track(SOCKET socket, const disconnection_handler_t& handler) {
  std::lock_guard<std::recursive_mutex> lock(m_socket_mutex);

  auto& info = m_sockets[socket];
  info.async_read = false;
  info.async_write = false;
  info.disconnection_handler = handler;

  notify_select();
}

void
io_service::untrack(SOCKET sock) {
  std::lock_guard<std::recursive_mutex> lock(m_socket_mutex);
  m_sockets.erase(sock);
}

bool
io_service::async_read(SOCKET sock, std::vector<char>& buffer, std::size_t read_size, const read_callback_t& callback) {
  std::lock_guard<std::recursive_mutex> lock(m_socket_mutex);

  auto reg_fd_it = m_sockets.find(sock);
  if (reg_fd_it == m_sockets.end())
    return false;

  auto& reg_fd = reg_fd_it->second;
  bool expected = false;
  if (!reg_fd.async_read.compare_exchange_strong(expected, true))
    return false;

  reg_fd.read_buffer = &buffer;
  reg_fd.read_size = read_size;
  reg_fd.read_callback = callback;

  notify_select();

  return true;
}

bool
io_service::async_write(SOCKET sock, const std::vector<char>& buffer, std::size_t write_size, const write_callback_t& callback) {
  std::lock_guard<std::recursive_mutex> lock(m_socket_mutex);

  auto reg_sock_it = m_sockets.find(sock);
  if (reg_sock_it == m_sockets.end())
    return false;

  auto& reg_sock = reg_sock_it->second;
  bool expected = false;
  if (!reg_sock.async_write.compare_exchange_strong(expected, true))
      return false;

  reg_sock.write_buffer = buffer;
  reg_sock.write_size = write_size;
  reg_sock.write_callback = callback;

  notify_select();

  return true;
}

void
io_service::notify_select(void) {
  (void)send(m_notify_socket, "a", 1, 0);
}
#endif

} //! network
} //! cpp_redis
