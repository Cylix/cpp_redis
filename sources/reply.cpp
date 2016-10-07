#include "cpp_redis/reply.hpp"
#include "cpp_redis/redis_error.hpp"

namespace cpp_redis {

reply::reply(void)
: m_type(type::null) {}

reply::reply(const std::string& value, string_type reply_type)
: m_type(static_cast<type>(reply_type))
, m_strval(value) {}

reply::reply(int value)
: m_type(type::integer)
, m_intval(value) {}

reply::reply(const std::vector<reply>& rows)
: m_type(type::array)
, m_rows(rows) {}

void
reply::set(void) {
  m_type = type::null;
}

void
reply::set(const std::string& value, string_type reply_type) {
  m_type = static_cast<type>(reply_type);
  m_strval = value;
}

void
reply::set(int value) {
  m_type = type::integer;
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
  return is_simple_string() or is_bulk_string() or is_error();
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
  if (not is_array())
    throw cpp_redis::redis_error("Reply is not an array");

  return m_rows;
}

const std::string&
reply::as_string(void) const {
  if (not is_string())
    throw cpp_redis::redis_error("Reply is not a string");

  return m_strval;
}

int
reply::as_integer(void) const {
  if (not is_integer())
    throw cpp_redis::redis_error("Reply is not an integer");

  return m_intval;
}

reply::type
reply::get_type(void) const {
  return m_type;
}

} //! cpp_redis
