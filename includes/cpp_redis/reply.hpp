#pragma once

#include <iostream>
#include <string>
#include <vector>

#include <stdint.h>

namespace cpp_redis {

class reply {
public:
//! type of reply
#define __CPP_REDIS_REPLY_ERR 0
#define __CPP_REDIS_REPLY_BULK 1
#define __CPP_REDIS_REPLY_SIMPLE 2
#define __CPP_REDIS_REPLY_NULL 3
#define __CPP_REDIS_REPLY_INT 4
#define __CPP_REDIS_REPLY_ARRAY 5

  enum class type {
    error         = __CPP_REDIS_REPLY_ERR,
    bulk_string   = __CPP_REDIS_REPLY_BULK,
    simple_string = __CPP_REDIS_REPLY_SIMPLE,
    null          = __CPP_REDIS_REPLY_NULL,
    integer       = __CPP_REDIS_REPLY_INT,
    array         = __CPP_REDIS_REPLY_ARRAY
  };

  enum class string_type {
    error         = __CPP_REDIS_REPLY_ERR,
    bulk_string   = __CPP_REDIS_REPLY_BULK,
    simple_string = __CPP_REDIS_REPLY_SIMPLE
  };

public:
  //! ctors
  reply(void);
  reply(const std::string& value, string_type reply_type);
  reply(int64_t value);
  reply(const std::vector<reply>& rows);

  //! dtors & copy ctor & assignment operator
  ~reply(void)        = default;
  reply(const reply&) = default;
  reply& operator=(const reply&) = default;

public:
  //! type info getters
  bool is_array(void) const;
  bool is_string(void) const;
  bool is_simple_string(void) const;
  bool is_bulk_string(void) const;
  bool is_error(void) const;
  bool is_integer(void) const;
  bool is_null(void) const;

  //! convenience function for error handling
  bool ok(void) const;
  bool ko(void) const;
  const std::string& error(void) const;

  //! convenience implicit conversion, same as !is_null()
  operator bool(void) const;

  //! Value getters
  const std::vector<reply>& as_array(void) const;
  const std::string& as_string(void) const;
  int64_t as_integer(void) const;

  //! Value setters
  void set(void);
  void set(const std::string& value, string_type reply_type);
  void set(int64_t value);
  void set(const std::vector<reply>& rows);
  reply& operator<<(const reply& reply);

  //! type getter
  type get_type(void) const;

private:
  type m_type;
  std::vector<cpp_redis::reply> m_rows;
  std::string m_strval;
  int64_t m_intval;
};

} //! cpp_redis

//! support for output
std::ostream& operator<<(std::ostream& os, const cpp_redis::reply& reply);
