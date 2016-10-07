#pragma once

#include <string>
#include <vector>

namespace cpp_redis {

class reply {
public:
  //! type of reply
  #define CPP_REDIS_REPLY_ERR     0
  #define CPP_REDIS_REPLY_BULK    1
  #define CPP_REDIS_REPLY_SIMPLE  2
  #define CPP_REDIS_REPLY_NULL    3
  #define CPP_REDIS_REPLY_INT     4
  #define CPP_REDIS_REPLY_ARRAY   5

  enum class type {
    error         = CPP_REDIS_REPLY_ERR,
    bulk_string   = CPP_REDIS_REPLY_BULK,
    simple_string = CPP_REDIS_REPLY_SIMPLE,
    null          = CPP_REDIS_REPLY_NULL,
    integer       = CPP_REDIS_REPLY_INT,
    array         = CPP_REDIS_REPLY_ARRAY
  };

  enum class string_type {
    error         = CPP_REDIS_REPLY_ERR,
    bulk_string   = CPP_REDIS_REPLY_BULK,
    simple_string = CPP_REDIS_REPLY_SIMPLE
  };

public:
  //! ctors
  reply(void);
  reply(const std::string& value, string_type reply_type);
  reply(int value);
  reply(const std::vector<reply>& rows);

  //! dtors & copy ctor & assignment operator
  ~reply(void) = default;
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

  //! Value getters
  const std::vector<reply>& as_array(void) const;
  const std::string& as_string(void) const;
  int as_integer(void) const;

  //! Value setters
  void set(void);
  void set(const std::string& value, string_type reply_type);
  void set(int value);
  void set(const std::vector<reply>& rows);
  reply& operator<<(const reply& reply);

  //! type getter
  type get_type(void) const;

private:
  type m_type;
  std::vector<cpp_redis::reply> m_rows;
  std::string m_strval;
  int m_intval;
};

} //! cpp_redis
