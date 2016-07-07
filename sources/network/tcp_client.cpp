#include <condition_variable>

#include "cpp_redis/network/tcp_client.hpp"

namespace cpp_redis {

namespace network {

tcp_client::tcp_client(void) {}

tcp_client::~tcp_client(void) {}

void
tcp_client::connect(const std::string& host, unsigned int port) {}

void
tcp_client::disconnect(void) {}

void
tcp_client::async_read(void) {}

void
tcp_client::send(const std::string& buffer) {
  send(std::vector<char>{ buffer.begin(), buffer.end() });
}

void
tcp_client::send(const std::vector<char>& buffer) {}

void
tcp_client::async_write(void) {}

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
