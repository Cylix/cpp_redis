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

#include <cpp_redis/builders/array_builder.hpp>
#include <cpp_redis/builders/builders_factory.hpp>
#include <cpp_redis/misc/error.hpp>
#include <cpp_redis/misc/logger.hpp>

namespace cpp_redis {

namespace builders {

array_builder::array_builder(void)
: m_current_builder(nullptr)
, m_reply_ready(false)
, m_reply(std::vector<reply>{}) {}

bool
array_builder::fetch_array_size(std::string& buffer) {
  if (m_int_builder.reply_ready())
    return true;

  m_int_builder << buffer;
  if (!m_int_builder.reply_ready())
    return false;

  int64_t size = m_int_builder.get_integer();
  if (size < 0) {
    m_reply.set();
    m_reply_ready = true;
  }
  else if (size == 0) {
    m_reply_ready = true;
  }

  m_array_size = size;

  return true;
}

bool
array_builder::build_row(std::string& buffer) {
  if (!m_current_builder) {
    m_current_builder = create_builder(buffer.front());
    buffer.erase(0, 1);
  }

  *m_current_builder << buffer;
  if (!m_current_builder->reply_ready())
    return false;

  m_reply << m_current_builder->get_reply();
  m_current_builder = nullptr;

  if (m_reply.as_array().size() == m_array_size)
    m_reply_ready = true;

  return true;
}

builder_iface&
array_builder::operator<<(std::string& buffer) {
  if (m_reply_ready)
    return *this;

  if (!fetch_array_size(buffer))
    return *this;

  while (buffer.size() && !m_reply_ready)
    if (!build_row(buffer))
      return *this;

  return *this;
}

bool
array_builder::reply_ready(void) const {
  return m_reply_ready;
}

reply
array_builder::get_reply(void) const {
  return reply{m_reply};
}

} // namespace builders

} // namespace cpp_redis
