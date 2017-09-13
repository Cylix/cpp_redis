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

#include <cctype>

#include <cpp_redis/builders/integer_builder.hpp>
#include <cpp_redis/misc/error.hpp>
#include <cpp_redis/misc/logger.hpp>

namespace cpp_redis {

namespace builders {

integer_builder::integer_builder(void)
: m_nbr(0)
, m_negative_multiplicator(1)
, m_reply_ready(false) {}

builder_iface&
integer_builder::operator<<(std::string& buffer) {
  if (m_reply_ready)
    return *this;

  auto end_sequence = buffer.find("\r\n");
  if (end_sequence == std::string::npos)
    return *this;

  std::size_t i;
  for (i = 0; i < end_sequence; i++) {
    //! check for negative numbers
    if (!i && m_negative_multiplicator == 1 && buffer[i] == '-') {
      m_negative_multiplicator = -1;
      continue;
    }
    else if (!std::isdigit(buffer[i])) {
      __CPP_REDIS_LOG(error, "cpp_redis::builders::integer_builder receives invalid digit character");
      throw redis_error("Invalid character for integer redis reply");
    }

    m_nbr *= 10;
    m_nbr += buffer[i] - '0';
  }

  buffer.erase(0, end_sequence + 2);
  m_reply.set(m_negative_multiplicator * m_nbr);
  m_reply_ready = true;

  return *this;
}

bool
integer_builder::reply_ready(void) const {
  return m_reply_ready;
}

reply
integer_builder::get_reply(void) const {
  return reply{m_reply};
}

int64_t
integer_builder::get_integer(void) const {
  return m_negative_multiplicator * m_nbr;
}

} // namespace builders

} // namespace cpp_redis
