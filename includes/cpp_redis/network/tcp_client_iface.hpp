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

#include <functional>
#include <string>
#include <vector>
#include <cstdint>

namespace cpp_redis {

namespace network {

class tcp_client_iface {
public:
  //! ctor & dtor
  tcp_client_iface(void)          = default;
  virtual ~tcp_client_iface(void) = default;

public:
  //! start & stop the tcp client
  virtual void connect(const std::string& addr, std::uint32_t port) = 0;
  virtual void disconnect(bool wait_for_removal = false) = 0;

  //! returns whether the client is currently connected or not
  virtual bool is_connected(void) const = 0;

public:
  //! structure to store read requests result
  struct read_result {
    bool success;
    std::vector<char> buffer;
  };

  //! structure to store write requests result
  struct write_result {
    bool success;
    std::size_t size;
  };

public:
  //! async read & write completion callbacks
  typedef std::function<void(read_result&)> async_read_callback_t;
  typedef std::function<void(write_result&)> async_write_callback_t;

public:
  //! structure to store read requests information
  struct read_request {
    std::size_t size;
    async_read_callback_t async_read_callback;
  };

  //! structure to store write requests information
  struct write_request {
    std::vector<char> buffer;
    async_write_callback_t async_write_callback;
  };

public:
  //! async read & write operations
  virtual void async_read(read_request& request)   = 0;
  virtual void async_write(write_request& request) = 0;

public:
  //! disconnection handle
  typedef std::function<void()> disconnection_handler_t;

  //! set on disconnection handler
  virtual void set_on_disconnection_handler(const disconnection_handler_t& disconnection_handler) = 0;
};

} //! network

} //! cpp_redis
