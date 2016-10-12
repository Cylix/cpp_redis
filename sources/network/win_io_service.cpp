#include "cpp_redis/network/win_io_service.hpp"
#include "cpp_redis/redis_error.hpp"

namespace cpp_redis {

namespace network {

const std::shared_ptr<io_service>&
   io_service::get_instance(void) {
   static std::shared_ptr<io_service> instance = std::shared_ptr<io_service>{ new io_service };
   return instance;
}

io_service::io_service(size_t max_worker_threads)
  : m_should_stop(false)
{
   //Determine the size of the thread pool dynamically.
   //2 * number of processors in the system is our rule here.
   SYSTEM_INFO info;
   ::GetSystemInfo(&info);
   m_worker_thread_pool_size = (info.dwNumberOfProcessors * 2);
  
  if (m_worker_thread_pool_size > max_worker_threads)
    m_worker_thread_pool_size = max_worker_threads;

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
  shutdown();
}

void 
io_service::shutdown()
{
  m_should_stop = true;
  
  //Iterate all of our sockets and shutdown any IO worker threads by posting a issuing a special
  //message to the thread to tell them to wake up and shut down.
  io_context_info* pInfo = NULL;

  auto sock_it = m_sockets.begin();
  while (sock_it != m_sockets.end())
  {
    auto& info = sock_it->second;
    //Post for each of our worker threads.
    int workers = m_worker_threads.size();
    for(int i=0; i<workers; i++)
      PostQueuedCompletionStatus(m_completion_port, 0, NULL, NULL); //Use NULL for the completion key to wake them up.
    sock_it++;
  }

  // Wait for the threads to finish
  for (auto& t : m_worker_threads)
    t.join();

  //close the completion port otherwise the worker threads will all be waiting on GetQueuedCompletionStatus()
  if (m_completion_port)
  {
    CloseHandle(m_completion_port);
    m_completion_port = NULL;
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
  
  info.hsock = sock;
  info.disconnection_handler = handler;

  //Associate the socket with our io completion port.
  if( NULL == CreateIoCompletionPort((HANDLE)sock, m_completion_port, (DWORD_PTR)&m_sockets[sock], 0))
  {
    throw cpp_redis::redis_error("Track() failed to create CreateIoCompletionPort");
  }
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

  //Resize the buffer to the correct size if we need it to be to hold the incoming data.
  if (buffer.size() < read_size)
    buffer.resize(read_size);

  auto& sockinfo = sock_it->second;
  sockinfo.read_buffer = &buffer;
  sockinfo.read_callback = callback;

  DWORD dwRecvNumBytes = 0;
  DWORD dwFlags = 0;
  WSABUF buffRecv;

  buffRecv.buf = sockinfo.read_buffer->data();
  buffRecv.len = read_size;
  
  //We need a new overlapped struct for EACH overlapped operation.
  //we reuse them over and over.
  io_context_info* p_io_info = sockinfo.get_pool_io_context();

  int nRet = WSARecv(sock, &buffRecv, 1, &dwRecvNumBytes, &dwFlags, &(p_io_info->overlapped), NULL);

  if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
  {
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
  sockinfo.write_buffer = buffer;
  sockinfo.write_size = write_size;
  sockinfo.write_callback = callback;

  WSABUF buffSend;
  DWORD dwSendNumBytes = 0;
  DWORD dwFlags = 0;
  buffSend.buf = sockinfo.write_buffer.data();
  buffSend.len = write_size;

  //We need a new overlapped struct for EACH overlapped operation.
  //we reuse them over and over.
  io_context_info* p_io_info = sockinfo.get_pool_io_context();
  p_io_info->eOperation = IO_OP_WRITE;

  int nRet = WSASend(sock, &buffSend, 1, &dwSendNumBytes, dwFlags, &(p_io_info->overlapped), NULL);
  if (SOCKET_ERROR == nRet)
  {
    int error = WSAGetLastError();
    if (error != ERROR_IO_PENDING)
    {
      //Fire the disconnect handler
      sockinfo.disconnection_handler(*this);
      return false;
    }
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
  sock_info*          psock_info = NULL;
  io_context_info*    pio_info = NULL;

  DWORD dwRecvNumBytes = 0;
  DWORD dwSendNumBytes = 0;
  DWORD dwFlags = 0;
  DWORD io_size = 0;
  enIoOperation e_op;
  int sock_error = 0;

  while (!m_should_stop)
  {
    // continually loop to service io completion packets
    psock_info = NULL;
    pOverlapped = NULL;

    bSuccess = GetQueuedCompletionStatus(m_completion_port, &io_size, (PDWORD_PTR)&psock_info, (LPOVERLAPPED *)&pOverlapped, INFINITE);
    if (!bSuccess)
    {
      //client socket must have died. continue...
      if (0 == io_size)
      {
        sock_error = WSAGetLastError();
        //Fire the disconnect handler if we can.
        if(psock_info && sock_error != ERROR_CONNECTION_ABORTED)  //Seems the psock_info structure is crap when ERROR_CONNECTION_ABORTED is captured.
          psock_info->disconnection_handler(*this);
        continue;
      }
      if (m_should_stop)
        return 0;
    }

    //get the base address of the struct holding lpOverlapped (the io_context_info) pointer.
    if (pOverlapped)
      pio_info = CONTAINING_RECORD(pOverlapped, io_context_info, overlapped);
    else
      pio_info = NULL;

    // Somebody used PostQueuedCompletionStatus to post an I/O packet with
    // a NULL CompletionKey (or if we get one for any reason).  It is time to exit.
    if (!psock_info || !pOverlapped)
      return 0;

    e_op = pio_info->eOperation;

    //Push it to the pool so another IO operation can use it again later
    if(pio_info)
      psock_info->return_pool_io_context(pio_info);
    pio_info = NULL;

    // First check to see if an error has occurred on the socket and if so
    // then close the socket and cleanup the SOCKET_INFORMATION structure associated with the socket
    if(0 == io_size)
    {
      //Fire the disconnect handler. Check if stopped first because of some weird timing issues that may get us here.
      if(bSuccess && false == m_should_stop && psock_info)
        psock_info->disconnection_handler(*this);
      continue;
    }
  
    // determine what type of IO packet has completed by checking the io_context_info 
    // associated with this io operation. This will determine what action to take.
    switch (e_op)
    {
      case IO_OP_READ:        // a read operation has completed
        //TODO Resize Read buffer to the size that we read?
        psock_info->read_callback(io_size);
      break;

      case IO_OP_WRITE:       // a write operation has completed
        //Lock our socket structure before updating the write_size
        {
          std::lock_guard<std::recursive_mutex> socklock(psock_info->sock_info_mutex);
          psock_info->write_size -= io_size;
        }
        psock_info->write_callback(io_size);
      break;
    } //switch
  } //while

  return 0;
}

} //! network
} //! cpp_redis
