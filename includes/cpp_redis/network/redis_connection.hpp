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

#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include <cpp_redis/builders/reply_builder.hpp>

#include <tacopie/tacopie>

#ifndef __CPP_REDIS_READ_SIZE
#define __CPP_REDIS_READ_SIZE 4096
#endif /* __CPP_REDIS_READ_SIZE */

namespace cpp_redis {

namespace network {

class redis_connection {
public:
  //! ctor & dtor
  redis_connection(void);
  ~redis_connection(void);

  //! copy ctor & assignment operator
  redis_connection(const redis_connection&) = delete;
  redis_connection& operator=(const redis_connection&) = delete;

public:
  //! handle connection
  typedef std::function<void(redis_connection&)> disconnection_handler_t;
  typedef std::function<void(redis_connection&, reply&)> reply_callback_t;
  void connect(const std::string& host = "127.0.0.1", std::size_t port = 6379,
    const disconnection_handler_t& disconnection_handler = nullptr,
    const reply_callback_t& reply_callback               = nullptr);
  void disconnect(bool wait_for_removal = false);
  bool is_connected(void);

  //! send cmd
  redis_connection& send(const std::vector<std::string>& redis_cmd);

  //! commit pipelined transaction
  redis_connection& commit(void);

private:
  //! receive & disconnection handlers
  void tcp_client_receive_handler(const tacopie::tcp_client::read_result& result);
  void tcp_client_disconnection_handler(void);

  std::string build_command(const std::vector<std::string>& redis_cmd);

private:
  void call_disconnection_handler(void);

private:
  //! tcp client for redis connection
  tacopie::tcp_client m_client;

  //! reply callback
  reply_callback_t m_reply_callback;

  //! user defined disconnection handler
  disconnection_handler_t m_disconnection_handler;

  //! reply builder
  builders::reply_builder m_builder;

  //! internal buffer used for pipelining
  std::string m_buffer;

  //! protect internal buffer against race conditions
  std::mutex m_buffer_mutex;
};

} //! network

} //! cpp_redis
