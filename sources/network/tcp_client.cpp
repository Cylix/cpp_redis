#include <condition_variable>
#include <netdb.h>
#include <cstring>
#include <iostream>

#include "cpp_redis/network/tcp_client.hpp"

namespace cpp_redis {

namespace network {

tcp_client::tcp_client(void)
: m_fd(-1)
, m_is_connected(false)
, m_receive_handler(nullptr)
, m_disconnection_handler(nullptr)
{}

tcp_client::~tcp_client(void) {
  if (m_is_connected)
    io_service::get_instance().untrack(m_fd);
}

void
tcp_client::connect(const std::string& host, unsigned int port) {
  if (m_is_connected)
    return ;

  //! create the socket
  m_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (m_fd < 0)
      throw redis_error("Can't open a socket");

  //! get the server's DNS entry
  struct hostent *server = gethostbyname(host.c_str());
  if (not server)
    throw redis_error("No such host: " + host);

  //! build the server's Internet address
  struct sockaddr_in server_addr;
  std::memset(&server_addr, 0, sizeof(server_addr));
  std::memcpy(server->h_addr, &server_addr.sin_addr.s_addr, server->h_length);
  server_addr.sin_port = htons(port);
  server_addr.sin_family = AF_INET;

  //! create a connection with the server
  if (::connect(m_fd, reinterpret_cast<const struct sockaddr *>(&server_addr), sizeof(server_addr)) < 0)
    throw redis_error("Fail to connect to " + host + ":" + std::to_string(port));

  m_is_connected = true;
  io_service::get_instance().track(m_fd);
  async_read();
}

void
tcp_client::disconnect(void) {
  if (not m_is_connected)
    return ;

  m_is_connected = false;
  io_service::get_instance().untrack(m_fd);
  close(m_fd);
}

void
tcp_client::send(const std::string& buffer) {
  send(std::vector<char>{ buffer.begin(), buffer.end() });
}

void
tcp_client::send(const std::vector<char>& buffer) {
  if (not m_is_connected)
    throw redis_error("Not connected");

  if (not buffer.size())
    return ;

  std::lock_guard<std::mutex> lock(m_write_buffer_mutex);

  bool bytes_in_buffer = m_write_buffer.size() > 0;

  //! concat buffer
  m_write_buffer.insert(m_write_buffer.end(), buffer.begin(), buffer.end());

  //! if there were already bytes in buffer, simply return
  //! async_write callback will process the new buffer
  if (bytes_in_buffer)
    return;

  async_write();
}

void
tcp_client::async_read(void) {
  io_service::get_instance().async_read(m_fd, m_read_buffer, READ_SIZE,
    [=](bool success, std::size_t length) {
      if (not success) {
        disconnect();
        return ;
      }

      std::lock_guard<std::mutex> lock(m_receive_handler_mutex);
      if (m_receive_handler)
        if (not m_receive_handler(*this, { m_read_buffer.begin(), m_read_buffer.begin() + length })) {
          disconnect();
          return ;
        }

      //! clear read buffer keep waiting for incoming bytes
      m_read_buffer.clear();
      async_read();
    });
}

void
tcp_client::async_write(void) {
  io_service::get_instance().async_write(m_fd, m_write_buffer, m_write_buffer.size(),
    [this](bool success, std::size_t length) {
      if (not success) {
        disconnect();
        return ;
      }

      std::lock_guard<std::mutex> lock(m_write_buffer_mutex);
      m_write_buffer.erase(m_write_buffer.begin(), m_write_buffer.begin() + length);

      if (m_write_buffer.size())
        async_write();
    });
}

void
tcp_client::set_receive_handler(const receive_handler& handler) {
  std::lock_guard<std::mutex> lock(m_receive_handler_mutex);

  m_receive_handler = handler;
}

void
tcp_client::set_disconnection_handler(const disconnection_handler& handler) {
  std::lock_guard<std::mutex> lock(m_disconnection_handler_mutex);

  m_disconnection_handler = handler;
}

bool
tcp_client::is_connected(void) {
  return m_is_connected;
}

} //! network

} //! cpp_redis
