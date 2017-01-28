// The MIT License (MIT)
//
// Copyright (c) 2015-2017 Simon Ninon <simon.ninon@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cpp_redis/logger.hpp>
#include <cpp_redis/network/redis_connection.hpp>
#include <cpp_redis/redis_error.hpp>

namespace cpp_redis {

namespace network {

redis_connection::redis_connection(void)
: m_reply_callback(nullptr)
, m_disconnection_handler(nullptr) {
  __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection created");
}

redis_connection::~redis_connection(void) {
  m_client.disconnect(true);
  __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection destroyed");
}

void
redis_connection::connect(const std::string& host, std::size_t port,
  const disconnection_handler_t& client_disconnection_handler,
  const reply_callback_t& client_reply_callback) {
  try {
    __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection attempts to connect");

    //! connect client and start to read asynchronously
    m_client.connect(host, port);
    m_client.async_read({__CPP_REDIS_READ_SIZE, std::bind(&redis_connection::tcp_client_receive_handler, this, std::placeholders::_1)});
    m_client.set_on_disconnection_handler(std::bind(&redis_connection::tcp_client_disconnection_handler, this));

    __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection connected");
  }
  catch (const tacopie::tacopie_error& e) {
    __CPP_REDIS_LOG(error, std::string("cpp_redis::network::redis_connection ") + e.what());
    throw redis_error(e.what());
  }

  m_reply_callback        = client_reply_callback;
  m_disconnection_handler = client_disconnection_handler;
}

void
redis_connection::disconnect(bool wait_for_removal) {
  __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection attempts to disconnect");
  m_client.disconnect(wait_for_removal);
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

  try {
    m_client.async_write({std::vector<char>{buffer.begin(), buffer.end()}, nullptr});
  }
  catch (const tacopie::tacopie_error& e) {
    __CPP_REDIS_LOG(error, std::string("cpp_redis::network::redis_connection ") + e.what());
    throw redis_error(e.what());
  }

  __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection sent pipelined commands");

  return *this;
}

void
redis_connection::call_disconnection_handler(void) {
  if (m_disconnection_handler) {
    __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection calls disconnection handler");
    m_disconnection_handler(*this);
  }
}

void
redis_connection::tcp_client_receive_handler(const tacopie::tcp_client::read_result& result) {
  if (!result.success) { return; }

  try {
    __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection receives packet, attempts to build reply");
    m_builder << std::string(result.buffer.begin(), result.buffer.end());
  }
  catch (const redis_error&) {
    __CPP_REDIS_LOG(error, "cpp_redis::network::redis_connection could not build reply (invalid format), disconnecting");
    call_disconnection_handler();
    return;
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

  try {
    m_client.async_read({__CPP_REDIS_READ_SIZE, std::bind(&redis_connection::tcp_client_receive_handler, this, std::placeholders::_1)});
  }
  catch (const tacopie::tacopie_error&) {
    //! Client disconnected in the meantime
  }
}

void
redis_connection::tcp_client_disconnection_handler(void) {
  __CPP_REDIS_LOG(debug, "cpp_redis::network::redis_connection has been disconnected");

  if (m_disconnection_handler) {
    __CPP_REDIS_LOG(info, "cpp_redis::network::redis_connection calls disconnection handler");
    m_disconnection_handler(*this);
  }
}

} //! network

} //! cpp_redis
