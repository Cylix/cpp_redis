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

#include <cpp_redis/network/io_service.hpp>

#ifdef _WIN32
#include <cpp_redis/network/windows_impl/io_service.hpp>
#else
#include <cpp_redis/network/unix_impl/io_service.hpp>
#endif /* _WIN32 */

namespace cpp_redis {

namespace network {

static std::shared_ptr<network::io_service> global_instance = nullptr;

const std::shared_ptr<network::io_service>&
io_service::get_global_instance(void) {
  if (!global_instance)
    global_instance = create_io_service();

  return global_instance;
}

void
io_service::set_global_instance(const std::shared_ptr<network::io_service>& io_service) {
  global_instance = io_service;
}

io_service::io_service(std::size_t nb_workers)
: m_nb_workers(nb_workers) {}

std::size_t
io_service::get_nb_workers(void) const {
  return m_nb_workers;
}

std::shared_ptr<network::io_service>
create_io_service(std::size_t nb_workers) {
#ifdef _WIN32
  return std::make_shared<windows_impl::io_service>(nb_workers);
#else
  return std::make_shared<unix_impl::io_service>(nb_workers);
#endif /* _WIN32 */
}

} //! network

} //! cpp_redis
