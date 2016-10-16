#include <cpp_redis/logger.hpp>
#include <cpp_redis/network/redis_connection.hpp>

namespace cpp_redis {

namespace network {

redis_connection::redis_connection(const std::shared_ptr<io_service>& io_service)
: m_client(io_service)
, m_reply_callback(nullptr)
, m_disconnection_handler(nullptr) {
  __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection created");
}

redis_connection::~redis_connection(void) {
  m_client.disconnect();
  __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection destroyed");
}

void
redis_connection::connect(const std::string& host, std::size_t port,
  const disconnection_handler_t& client_disconnection_handler,
  const reply_callback_t& client_reply_callback) {
  __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection attempts to connect");

  auto disconnection_handler = std::bind(&redis_connection::tcp_client_disconnection_handler, this, std::placeholders::_1);
  auto receive_handler       = std::bind(&redis_connection::tcp_client_receive_handler, this, std::placeholders::_1, std::placeholders::_2);
  m_client.connect(host, port, disconnection_handler, receive_handler);

  __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection connected");

  m_reply_callback        = client_reply_callback;
  m_disconnection_handler = client_disconnection_handler;
}

void
redis_connection::disconnect(void) {
  __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection attempts to disconnect");
  m_client.disconnect();
  __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection disconnected");
}

bool
redis_connection::is_connected(void) {
  return m_client.is_connected();
}

std::string
redis_connection::build_command(const std::vector<std::string>& redis_cmd) {
  std::string cmd = "*" + std::to_string(redis_cmd.size()) + "\r\n";

  for (const auto& cmd_part : redis_cmd)
    cmd += "$" + std::to_string(cmd_part.length()) + "\r\n" + cmd_part + "\r\n";

  return cmd;
}

redis_connection&
redis_connection::send(const std::vector<std::string>& redis_cmd) {
  std::lock_guard<std::mutex> lock(m_buffer_mutex);

  m_buffer += build_command(redis_cmd);
  __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection stored new command in the send buffer");

  return *this;
}

//! commit pipelined transaction
redis_connection&
redis_connection::commit(void) {
  std::lock_guard<std::mutex> lock(m_buffer_mutex);

  //! ensure buffer is cleared
  __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection attempts to send pipelined commands");
  std::string buffer = std::move(m_buffer);
  m_client.send(buffer);
  __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection sent pipelined commands");

  return *this;
}

bool
redis_connection::tcp_client_receive_handler(network::tcp_client&, const std::vector<char>& buffer) {
  try {
    __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection receives packet, attempts to build reply");
    m_builder << std::string(buffer.begin(), buffer.end());
  }
  catch (const redis_error&) {
    __CPP_REDIS_LOG(error, "cpp_redis::network::redis_connection could not build reply (invalid format), disconnecting");

    if (m_disconnection_handler) {
      __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection calls disconnection handler");
      m_disconnection_handler(*this);
    }

    return false;
  }

  while (m_builder.reply_available()) {
    __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection reply fully built");

    auto reply = m_builder.get_front();
    m_builder.pop_front();

    if (m_reply_callback) {
      __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection executes reply callback");
      m_reply_callback(*this, reply);
    }
  }

  return true;
}

void
redis_connection::tcp_client_disconnection_handler(network::tcp_client&) {
  __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection has been disconnected");

  if (m_disconnection_handler) {
    __CPP_REDIS_LOG(info, "cpp_redis::network::redis_connection calls disconnection handler");
    m_disconnection_handler(*this);
  }
}

} //! network

} //! cpp_redis
