// MIT License
//
// Copyright (c) 2016-2017 Simon Ninon <simon.ninon@gmail.com>
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

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace cpp_redis {

namespace network {

/**
 * interface defining how tcp client should be implemented to be used inside cpp_redis
 *
 */
class tcp_client_iface {
public:
/**
 * ctor
 *
 */
  tcp_client_iface() = default;
/**
 * dtor
 *
 */
  virtual ~tcp_client_iface() = default;

public:
/**
 * start the tcp client
 *
 * @param addr host to be connected to
 * @param port port to be connected to
 * @param timeout_ms max time to connect in ms
 *
 */
  virtual void connect(const std::string& addr, std::uint32_t port, std::uint32_t timeout_ms = 0) = 0;

/**
 * stop the tcp client
 *
 * @param wait_for_removal when sets to true, disconnect blocks until the underlying TCP client has been effectively removed from the io_service and that all the underlying callbacks have completed.
 *
 */
  virtual void disconnect(bool wait_for_removal = false) = 0;

/**
 * @return whether the client is currently connected or not
 *
 */
  virtual bool is_connected() const = 0;

public:
/**
 * structure to store read requests result
 *
 */
  struct read_result {
/**
 * whether the operation succeeded or not
 *
 */
    bool success;

/**
 * read bytes
 *
 */
    std::vector<char> buffer;
  };

/**
 * structure to store write requests result
 *
 */
  struct write_result {
/**
 * whether the operation succeeded or not
 *
 */
    bool success;

/**
 * number of bytes written
 *
 */
    std::size_t size;
  };

public:
/**
 * async read completion callbacks
 * function taking read_result as a parameter
 *
 */
  typedef std::function<void(read_result&)> async_read_callback_t;

/**
 * async write completion callbacks
 * function taking write_result as a parameter
 *
 */
  typedef std::function<void(write_result&)> async_write_callback_t;

public:
/**
 * structure to store read requests information
 *
 */
  struct read_request {
/**
 * number of bytes to read
 *
 */
    std::size_t size;

/**
 * callback to be called on operation completion
 *
 */
    async_read_callback_t async_read_callback;
  };

/**
 * structure to store write requests information
 *
 */
  struct write_request {
/**
 * bytes to write
 *
 */
    std::vector<char> buffer;

/**
 * callback to be called on operation completion
 *
 */
    async_write_callback_t async_write_callback;
  };

public:
/**
 * async read operation
 *
 * @param request information about what should be read and what should be done after completion
 *
 */
  virtual void async_read(read_request& request) = 0;

/**
 * async write operation
 *
 * @param request information about what should be written and what should be done after completion
 *
 */
  virtual void async_write(write_request& request) = 0;

public:
/**
 * disconnection handler
 *
 */
  typedef std::function<void()> disconnection_handler_t;

/**
 * set on disconnection handler
 *
 * @param disconnection_handler handler to be called in case of a disconnection
 *
 */
  virtual void set_on_disconnection_handler(const disconnection_handler_t& disconnection_handler) = 0;
};

} // namespace network

} // namespace cpp_redis
