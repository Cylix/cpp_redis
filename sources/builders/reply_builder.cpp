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

#include <cpp_redis/builders/builders_factory.hpp>
#include <cpp_redis/builders/reply_builder.hpp>
#include <cpp_redis/misc/error.hpp>

namespace cpp_redis {

namespace builders {

reply_builder::reply_builder(void)
: m_builder(nullptr) {}

reply_builder&
reply_builder::operator<<(const std::string& data) {
  m_buffer += data;

  while (build_reply())
    ;

  return *this;
}

bool
reply_builder::build_reply(void) {
  if (!m_buffer.size())
    return false;

  if (!m_builder) {
    m_builder = create_builder(m_buffer.front());
    m_buffer.erase(0, 1);
  }

  *m_builder << m_buffer;

  if (m_builder->reply_ready()) {
    m_available_replies.push_back(m_builder->get_reply());
    m_builder = nullptr;

    return true;
  }

  return false;
}

void
reply_builder::operator>>(reply& reply) {
  reply = get_front();
}

const reply&
reply_builder::get_front(void) const {
  if (!reply_available())
    throw redis_error("No available reply");

  return m_available_replies.front();
}

void
reply_builder::pop_front(void) {
  if (!reply_available())
    throw redis_error("No available reply");

  m_available_replies.pop_front();
}

bool
reply_builder::reply_available(void) const {
  return m_available_replies.size() > 0;
}

} // namespace builders

} // namespace cpp_redis
