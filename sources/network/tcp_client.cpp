#include <condition_variable>

//! Disable "The POSIX name for this item is deprecated" warnings for gethostbyname()
#ifdef _WIN32
#pragma warning(disable : 4996)
#endif /* _WIN32 */
#include <cstring>

#ifdef _WIN32
#include <WinSock2.h>
#else
#include <netdb.h>
#include <unistd.h>
#endif /* _WIN32 */

#include <cpp_redis/logger.hpp>
#include <cpp_redis/network/tcp_client.hpp>

namespace cpp_redis {

namespace network {

tcp_client::tcp_client(const std::shared_ptr<network::io_service>& io_service)
: m_io_service(io_service ? io_service : io_service::get_global_instance())
, m_sock(-1)
, m_is_connected(false)
, m_receive_handler(nullptr)
, m_disconnection_handler(nullptr) {
  __CPP_REDIS_LOG(debug, "cpp_redis::network::tcp_client created");
}

tcp_client::~tcp_client(void) {
  disconnect();
  __CPP_REDIS_LOG(debug, "cpp_redis::network::tcp_client destroyed");
}

void
tcp_client::setup_socket(void) {
#ifdef _WIN32
  //! create the socket
  //! Enable socket for overlapped i/o
  m_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
  if (m_sock < 0) {
    __CPP_REDIS_LOG(error, "cpp_redis::network::tcp_client could not create socket");
    throw redis_error("Can't open a socket");
  }

  //! Instruct the TCP stack to directly perform I/O using the buffer provided in our I/O call.
  //! The advantage is performance because we save a buffer copy between the TCP stack buffer
  //! and our user buffer for each I/O call.
  //! BUT we have to make sure we don't access the buffer once it's submitted for overlapped operation and before the overlapped operation completes!
  int nZero = 0;
  if (setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (char*) &nZero, sizeof(nZero))) {
    __CPP_REDIS_LOG(warn, "cpp_redis::network::tcp_client could not disable buffering");
  }
#else
  //! create the socket
  m_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (m_sock < 0) {
    __CPP_REDIS_LOG(error, "cpp_redis::network::tcp_client could not create socket");
    throw redis_error("Can't open a socket");
  }
#endif /* _WIN32 */
}

void
tcp_client::connect(const std::string& host, std::size_t port,
  const disconnection_handler_t& disconnection_handler,
  const receive_handler_t& receive_handler) {
  __CPP_REDIS_LOG(debug, "cpp_redis::network::tcp_client attempts to connect");

  if (m_is_connected) {
    __CPP_REDIS_LOG(warn, "cpp_redis::network::tcp_client is already connected");
    return throw cpp_redis::redis_error("Client already connected");
  }

  setup_socket();

  //! get the server's DNS entry
  struct hostent* server = gethostbyname(host.c_str());
  if (!server) {
    __CPP_REDIS_LOG(error, "cpp_redis::network::tcp_client could not resolve DNS");
    throw redis_error("No such host: " + host);
  }

  //! build the server's Internet address
  struct sockaddr_in server_addr;
  std::memset(&server_addr, 0, sizeof(server_addr));
  std::memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
  server_addr.sin_port   = htons(port);
  server_addr.sin_family = AF_INET;

  //! create a connection with the server
  if (::connect(m_sock, reinterpret_cast<const struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
    __CPP_REDIS_LOG(error, "cpp_redis::network::tcp_client could not connect");
    throw redis_error("Fail to connect to " + host + ":" + std::to_string(port));
  }

#ifdef _WIN32
  //! Set socket to non blocking.
  //! Must only be done once connected
  u_long ulValue = 1;
  if (ioctlsocket(m_sock, FIONBIO, &ulValue)) {
    __CPP_REDIS_LOG(warn, "cpp_redis::network::tcp_client could not enable non-blocking mode on socket");
  }
#endif /* _WIN32 */

  //! add fd to the io_service and set the disconnection & recv handlers
  m_disconnection_handler = disconnection_handler;
  m_receive_handler       = receive_handler;
  m_io_service->track(m_sock, std::bind(&tcp_client::io_service_disconnection_handler, this, std::placeholders::_1));
  m_is_connected = true;

  __CPP_REDIS_LOG(debug, "cpp_redis::network::tcp_client connected");

  //! start async read
  async_read();
}

void
tcp_client::disconnect(void) {
  __CPP_REDIS_LOG(debug, "cpp_redis::network::tcp_client attemps to disconnect");

  if (!m_is_connected) {
    __CPP_REDIS_LOG(debug, "cpp_redis::network::tcp_client already disconnected");
    return;
  }

  m_io_service->untrack(m_sock);
  reset_state();

  __CPP_REDIS_LOG(debug, "cpp_redis::network::tcp_client disconnected");
}

void
tcp_client::send(const std::string& buffer) {
  send(std::vector<char>{buffer.begin(), buffer.end()});
}

void
tcp_client::send(const std::vector<char>& buffer) {
  __CPP_REDIS_LOG(debug, "cpp_redis::network::tcp_client attemps to send data");

  if (!m_is_connected) {
    __CPP_REDIS_LOG(error, "cpp_redis::network::tcp_client is not connected");
    throw redis_error("Not connected");
  }

  if (!buffer.size()) {
    __CPP_REDIS_LOG(warn, "cpp_redis::network::tcp_client has nothing to send");
    return;
  }

  std::lock_guard<std::mutex> lock(m_write_buffer_mutex);

  //! concat buffer
  m_write_buffer.push_back(buffer);

  //! if there were already bytes in buffer, simply return
  //! otherwise async_write calls itself recursively until the write buffer is empty
  if (m_write_buffer.size() > 1) {
    __CPP_REDIS_LOG(debug, "cpp_redis::network::tcp_client is already processing an async_write");
    return;
  }

  async_write();
}

void
tcp_client::async_read(void) {
  __CPP_REDIS_LOG(debug, "cpp_redis::network::tcp_client starts async_read");

  m_io_service->async_read(m_sock, m_read_buffer, __CPP_REDIS_READ_SIZE,
    [&](std::size_t length) {
      __CPP_REDIS_LOG(debug, "cpp_redis::network::tcp_client received data");

      if (m_receive_handler) {
        __CPP_REDIS_LOG(debug, "cpp_redis::network::tcp_client calls receive_handler");

        if (!m_receive_handler(*this, {m_read_buffer.begin(), m_read_buffer.begin() + length})) {
          __CPP_REDIS_LOG(warn, "cpp_redis::network::tcp_client has been asked for disconnection by receive_handler");
          disconnect();
          return;
        }
      }

      //! clear read buffer keep waiting for incoming bytes
      m_read_buffer.clear();

      if (m_is_connected)
        async_read();
    });
}

void
tcp_client::async_write(void) {
  __CPP_REDIS_LOG(debug, "cpp_redis::network::tcp_client starts async_write");

  m_io_service->async_write(m_sock, m_write_buffer.front(), m_write_buffer.front().size(),
    [&](std::size_t length) {
      __CPP_REDIS_LOG(debug, "cpp_redis::network::tcp_client wrote data and cleans write_buffer");
      std::lock_guard<std::mutex> lock(m_write_buffer_mutex);

      //! Remove what has already been sent and see if we have any more to send
      if (length >= m_write_buffer.front().size())
        m_write_buffer.pop_front();
      else
        m_write_buffer.front().erase(m_write_buffer.front().begin(), m_write_buffer.front().begin() + length);

      //! If we still have data to write the call ourselves recursivly until the buffer is completely sent
      if (m_is_connected && m_write_buffer.size())
        async_write();
    });
}

bool
tcp_client::is_connected(void) {
  return m_is_connected;
}

void
tcp_client::io_service_disconnection_handler(network::io_service&) {
  __CPP_REDIS_LOG(debug, "cpp_redis::network::tcp_client has been disconnected");

  reset_state();

  if (m_disconnection_handler) {
    __CPP_REDIS_LOG(debug, "cpp_redis::network::tcp_client calls disconnection handler");
    m_disconnection_handler(*this);
  }
}

void
tcp_client::reset_state(void) {
  m_is_connected = false;

  if (m_sock != INVALID_SOCKET) {
#ifdef _WIN32
    closesocket(m_sock);
#else
    close(m_sock);
#endif /* _WIN32 */
    m_sock = INVALID_SOCKET;
  }

  clear_buffer();
}

void
tcp_client::clear_buffer(void) {
  std::lock_guard<std::mutex> lock(m_write_buffer_mutex);
  m_write_buffer.clear();
  m_read_buffer.clear();
}

} //! network

} //! cpp_redis
