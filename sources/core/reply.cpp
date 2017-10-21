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

#include <cpp_redis/core/reply.hpp>
#include <cpp_redis/misc/error.hpp>

namespace cpp_redis {

reply::reply(void)
: m_type(type::null) {}

reply::reply(const std::string& value, string_type reply_type)
: m_type(static_cast<type>(reply_type))
, m_strval(value) {}

reply::reply(int64_t value)
: m_type(type::integer)
, m_intval(value) {}

reply::reply(const std::vector<reply>& rows)
: m_type(type::array)
, m_rows(rows) {}

bool
reply::ok(void) const {
  return !is_error();
}

bool
reply::ko(void) const {
  return !ok();
}

const std::string&
reply::error(void) const {
  if (!is_error())
    throw cpp_redis::redis_error("Reply is not an error");

  return as_string();
}

reply::operator bool(void) const {
  return !is_error() && !is_null();
}

void
reply::set(void) {
  m_type = type::null;
}

void
reply::set(const std::string& value, string_type reply_type) {
  m_type   = static_cast<type>(reply_type);
  m_strval = value;
}

void
reply::set(int64_t value) {
  m_type   = type::integer;
  m_intval = value;
}

void
reply::set(const std::vector<reply>& rows) {
  m_type = type::array;
  m_rows = rows;
}

reply&
reply::operator<<(const reply& reply) {
  m_type = type::array;
  m_rows.push_back(reply);

  return *this;
}

bool
reply::is_array(void) const {
  return m_type == type::array;
}

bool
reply::is_string(void) const {
  return is_simple_string() || is_bulk_string() || is_error();
}

bool
reply::is_simple_string(void) const {
  return m_type == type::simple_string;
}

bool
reply::is_bulk_string(void) const {
  return m_type == type::bulk_string;
}

bool
reply::is_error(void) const {
  return m_type == type::error;
}

bool
reply::is_integer(void) const {
  return m_type == type::integer;
}

bool
reply::is_null(void) const {
  return m_type == type::null;
}

const std::vector<reply>&
reply::as_array(void) const {
  if (!is_array())
    throw cpp_redis::redis_error("Reply is not an array");

  return m_rows;
}

const std::string&
reply::as_string(void) const {
  if (!is_string())
    throw cpp_redis::redis_error("Reply is not a string");

  return m_strval;
}

int64_t
reply::as_integer(void) const {
  if (!is_integer())
    throw cpp_redis::redis_error("Reply is not an integer");

  return m_intval;
}

reply::type
reply::get_type(void) const {
  return m_type;
}

} // namespace cpp_redis

std::ostream&
operator<<(std::ostream& os, const cpp_redis::reply& reply) {
  switch (reply.get_type()) {
  case cpp_redis::reply::type::error:
    os << reply.error();
    break;
  case cpp_redis::reply::type::bulk_string:
    os << reply.as_string();
    break;
  case cpp_redis::reply::type::simple_string:
    os << reply.as_string();
    break;
  case cpp_redis::reply::type::null:
    os << std::string("(nil)");
    break;
  case cpp_redis::reply::type::integer:
    os << reply.as_integer();
    break;
  case cpp_redis::reply::type::array:
    for (const auto& item : reply.as_array())
      os << item;
    break;
  }

  return os;
}
