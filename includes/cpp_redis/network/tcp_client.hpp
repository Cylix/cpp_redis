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

#include <cpp_redis/misc/error.hpp>
#include <cpp_redis/network/tcp_client_iface.hpp>

#include <tacopie/tacopie>

namespace cpp_redis {

namespace network {

/**
 * implementation of the tcp_client_iface based on tacopie networking library
 *
 */
class tcp_client : public tcp_client_iface {
public:
/**
 * ctor
 *
 */
  tcp_client() = default;
/**
 * dtor
 *
 */
  ~tcp_client() override = default;

public:
/**
 * start the tcp client
 *
 * @param addr host to be connected to
 * @param port port to be connected to
 * @param timeout_ms max time to connect in ms
 *
 */
  void connect(const std::string& addr, std::uint32_t port, std::uint32_t timeout_ms) override;

/**
 * stop the tcp client
 *
 * @param wait_for_removal when sets to true, disconnect blocks until the underlying TCP client has been effectively removed from the io_service and that all the underlying callbacks have completed.
 *
 */
  void disconnect(bool wait_for_removal = false) override;

/**
 * @return whether the client is currently connected or not
 *
 */
  bool is_connected() const override;

/**
 * set number of io service workers for the io service monitoring this tcp connection
 *
 * @param nb_threads number of threads to be assigned
 *
 */
  void set_nb_workers(std::size_t nb_threads);

public:
/**
 * async read operation
 *
 * @param request information about what should be read and what should be done after completion
 *
 */
  void async_read(read_request& request) override;

/**
 * async write operation
 *
 * @param request information about what should be written and what should be done after completion
 *
 */
  void async_write(write_request& request) override;

public:
/**
 * set on disconnection handler
 *
 * @param disconnection_handler handler to be called in case of a disconnection
 *
 */
  void set_on_disconnection_handler(const disconnection_handler_t& disconnection_handler) override;

private:
/**
 * tcp client for redis connection
 *
 */
  tacopie::tcp_client m_client;
};

/**
 * set the number of workers to be assigned for the default io service
 *
 * @param nb_threads the number of threads to be assigned
 *
 */
void set_default_nb_workers(std::size_t nb_threads);

} // namespace network

} // namespace cpp_redis
