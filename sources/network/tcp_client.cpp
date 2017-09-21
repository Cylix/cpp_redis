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

#include <cpp_redis/network/tcp_client.hpp>

namespace cpp_redis {

namespace network {

void
tcp_client::connect(const std::string& addr, std::uint32_t port, std::uint32_t timeout_msecs) {
  m_client.connect(addr, port, timeout_msecs);
}


void
tcp_client::disconnect(bool wait_for_removal) {
  m_client.disconnect(wait_for_removal);
}

bool
tcp_client::is_connected(void) const {
  return m_client.is_connected();
}

void
tcp_client::set_nb_workers(std::size_t nb_threads) {
  m_client.get_io_service()->set_nb_workers(nb_threads);
}

void
tcp_client::async_read(read_request& request) {
  auto callback = std::move(request.async_read_callback);

  m_client.async_read({request.size, [=](tacopie::tcp_client::read_result& result) {
                         if (!callback) {
                           return;
                         }

                         read_result converted_result = {result.success, std::move(result.buffer)};
                         callback(converted_result);
                       }});
}

void
tcp_client::async_write(write_request& request) {
  auto callback = std::move(request.async_write_callback);

  m_client.async_write({std::move(request.buffer), [=](tacopie::tcp_client::write_result& result) {
                          if (!callback) {
                            return;
                          }

                          write_result converted_result = {result.success, result.size};
                          callback(converted_result);
                        }});
}

void
tcp_client::set_on_disconnection_handler(const disconnection_handler_t& disconnection_handler) {
  m_client.set_on_disconnection_handler(disconnection_handler);
}

void
set_default_nb_workers(std::size_t nb_threads) {
  tacopie::get_default_io_service(nb_threads);
}

} // namespace network

} // namespace cpp_redis
