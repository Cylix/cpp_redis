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

#include <atomic>
#include <list>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <cpp_redis/network/io_service.hpp>
#include <cpp_redis/network/socket.hpp>
#include <cpp_redis/redis_error.hpp>

#ifndef __CPP_REDIS_READ_SIZE
#define __CPP_REDIS_READ_SIZE 4096
#endif /* __CPP_REDIS_READ_SIZE */

namespace cpp_redis {

namespace network {

//! tcp_client
//! async tcp client based on boost asio
class tcp_client {
public:
  //! ctor & dtor
  tcp_client(const std::shared_ptr<io_service>& IO = nullptr);
  ~tcp_client(void);

  //! assignment operator & copy ctor
  tcp_client(const tcp_client&) = delete;
  tcp_client& operator=(const tcp_client&) = delete;

  //! returns whether the client is connected or not
  bool is_connected(void);

  //! handle connection & disconnection
  typedef std::function<void(tcp_client&)> disconnection_handler_t;
  typedef std::function<bool(tcp_client&, const std::vector<char>& buffer)> receive_handler_t;
  void connect(const std::string& host, std::size_t port,
    const disconnection_handler_t& disconnection_handler = nullptr,
    const receive_handler_t& receive_handler             = nullptr);
  void disconnect(void);

  //! send data
  void send(const std::string& buffer);
  void send(const std::vector<char>& buffer);

private:
  //! make boost asio async read and write operations
  void async_read(void);
  void async_write(void);

  //! io service callback
  void io_service_disconnection_handler(io_service&);

  void reset_state(void);
  void clear_buffer(void);

  void setup_socket(bool is_unix_socket);

private:
  //! io service instance
  std::shared_ptr<io_service> m_io_service;

  //! socket fd
  _sock_t m_sock;

  //! is connected
  std::atomic_bool m_is_connected;

  //! buffers
  std::vector<char> m_read_buffer;
  std::list<std::vector<char>> m_write_buffer;

  //! handlers
  receive_handler_t m_receive_handler;
  disconnection_handler_t m_disconnection_handler;

  //! thread safety
  std::mutex m_write_buffer_mutex;
};

} //! network

} //! cpp_redis
