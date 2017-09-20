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
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <cpp_redis/builders/reply_builder.hpp>
#include <cpp_redis/network/tcp_client_iface.hpp>

#ifndef __CPP_REDIS_READ_SIZE
#define __CPP_REDIS_READ_SIZE 4096
#endif /* __CPP_REDIS_READ_SIZE */

namespace cpp_redis {

namespace network {

//!
//! tcp connection wrapper handling redis protocol
//!
class redis_connection {
public:
#ifndef __CPP_REDIS_USE_CUSTOM_TCP_CLIENT
  //! ctor
  redis_connection(void);
#endif /* __CPP_REDIS_USE_CUSTOM_TCP_CLIENT */

  //!
  //! ctor allowing to specify custom tcp client (default ctor uses the default tacopie tcp client)
  //!
  //! \param tcp_client tcp client to be used for network communications
  //!
  explicit redis_connection(const std::shared_ptr<tcp_client_iface>& tcp_client);

  //! dtor
  ~redis_connection(void);

  //! copy ctor
  redis_connection(const redis_connection&) = delete;
  //! assignment operator
  redis_connection& operator=(const redis_connection&) = delete;

public:
  //!
  //! disconnection handler takes as parameter the instance of the redis_connection
  //!
  typedef std::function<void(redis_connection&)> disconnection_handler_t;

  //!
  //! reply handler takes as parameter the instance of the redis_connection and the built reply
  //!
  typedef std::function<void(redis_connection&, reply&)> reply_callback_t;

  //!
  //! connect to the given host and port, and set both disconnection and reply callbacks
  //!
  //! \param host host to be connected to
  //! \param port port to be connected to
  //! \param disconnection_handler handler to be called in case of disconnection
  //! \param reply_callback handler to be called once a reply is ready
  //! \param timeout_msecs max time to connect (in ms)
  //!
  void connect(
    const std::string& host                              = "127.0.0.1",
    std::size_t port                                     = 6379,
    const disconnection_handler_t& disconnection_handler = nullptr,
    const reply_callback_t& reply_callback               = nullptr,
    std::uint32_t timeout_msecs                          = 0);

  //!
  //! disconnect from redis server
  //!
  //! \param wait_for_removal when sets to true, disconnect blocks until the underlying TCP client has been effectively removed from the io_service and that all the underlying callbacks have completed.
  //!
  void disconnect(bool wait_for_removal = false);

  //!
  //! \return whether we are connected to the redis server or not
  //!
  bool is_connected(void) const;

  //!
  //! send the given command
  //! the command is actually pipelined and only buffered, so nothing is sent to the network
  //! please call commit() to flush the buffer
  //!
  //! \param redis_cmd command to be sent
  //! \return current instance
  //!
  redis_connection& send(const std::vector<std::string>& redis_cmd);

  //!
  //! commit pipelined transaction
  //! that is, send to the network all commands pipelined by calling send()
  //!
  //! \return current instance
  //!
  redis_connection& commit(void);

private:
  //!
  //! tcp_client receive handler
  //! called by the tcp_client whenever a read has completed
  //!
  //! \param result read result
  //!
  void tcp_client_receive_handler(const tcp_client_iface::read_result& result);

  //!
  //! tcp_client disconnection handler
  //! called by the tcp_client whenever a disconnection occured
  //!
  void tcp_client_disconnection_handler(void);

  //!
  //! transform a user command to a redis command using the redis protocol format
  //! for example, transform {"GET", "HELLO"} to something like "*2\r\n+GET\r\n+HELLO\r\n"
  //!
  std::string build_command(const std::vector<std::string>& redis_cmd);

private:
  //!
  //! simply call the disconnection handler (does nothing if disconnection handler is set to null)
  //!
  void call_disconnection_handler(void);

private:
  //!
  //! tcp client for redis connection
  //!
  std::shared_ptr<cpp_redis::network::tcp_client_iface> m_client;

  //!
  //! reply callback called whenever a reply has been read
  //!
  reply_callback_t m_reply_callback;

  //!
  //! disconnection handler whenever a disconnection occured
  //!
  disconnection_handler_t m_disconnection_handler;

  //!
  //! reply builder used to build replies
  //!
  builders::reply_builder m_builder;

  //!
  //! internal buffer used for pipelining (commands are buffered here and flushed to the tcp client when commit is called)
  //!
  std::string m_buffer;

  //!
  //! protect internal buffer against race conditions
  //!
  std::mutex m_buffer_mutex;
};

} // namespace network

} // namespace cpp_redis
