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

#include <cpp_redis/builders/bulk_string_builder.hpp>
#include <cpp_redis/misc/error.hpp>
#include <cpp_redis/misc/logger.hpp>

namespace cpp_redis {

namespace builders {

bulk_string_builder::bulk_string_builder(void)
: m_str_size(0)
, m_str("")
, m_is_null(false)
, m_reply_ready(false) {}

void
bulk_string_builder::build_reply(void) {
  if (m_is_null)
    m_reply.set();
  else
    m_reply.set(m_str, reply::string_type::bulk_string);

  m_reply_ready = true;
}

bool
bulk_string_builder::fetch_size(std::string& buffer) {
  if (m_int_builder.reply_ready())
    return true;

  m_int_builder << buffer;
  if (!m_int_builder.reply_ready())
    return false;

  m_str_size = (int) m_int_builder.get_integer();
  if (m_str_size == -1) {
    m_is_null = true;
    build_reply();
  }

  return true;
}

void
bulk_string_builder::fetch_str(std::string& buffer) {
  if (buffer.size() < static_cast<std::size_t>(m_str_size) + 2) // also wait for end sequence
    return;

  if (buffer[m_str_size] != '\r' || buffer[m_str_size + 1] != '\n') {
    __CPP_REDIS_LOG(error, "cpp_redis::builders::bulk_string_builder receives invalid ending sequence");
    throw redis_error("Wrong ending sequence");
  }

  m_str = buffer.substr(0, m_str_size);
  buffer.erase(0, m_str_size + 2);
  build_reply();
}

builder_iface&
bulk_string_builder::operator<<(std::string& buffer) {
  if (m_reply_ready)
    return *this;

  //! if we don't have the size, try to get it with the current buffer
  if (!fetch_size(buffer) || m_reply_ready)
    return *this;

  fetch_str(buffer);

  return *this;
}

bool
bulk_string_builder::reply_ready(void) const {
  return m_reply_ready;
}

reply
bulk_string_builder::get_reply(void) const {
  return reply{m_reply};
}

const std::string&
bulk_string_builder::get_bulk_string(void) const {
  return m_str;
}

bool
bulk_string_builder::is_null(void) const {
  return m_is_null;
}

} // namespace builders

} // namespace cpp_redis
