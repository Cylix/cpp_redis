#pragma once

#include <cpp_redis/builders/builder_iface.hpp>
#include <cpp_redis/reply.hpp>

#include <stdint.h>

namespace cpp_redis {

namespace builders {

class integer_builder : public builder_iface {
public:
  //! ctor & dtor
  integer_builder(void);
  ~integer_builder(void) = default;

  //! copy ctor & assignment operator
  integer_builder(const integer_builder&) = delete;
  integer_builder& operator=(const integer_builder&) = delete;

public:
  //! builder_iface impl
  builder_iface& operator<<(std::string&);
  bool reply_ready(void) const;
  reply get_reply(void) const;

  //! getter
  int64_t get_integer(void) const;

private:
  int64_t m_nbr;
  char m_negative_multiplicator;
  bool m_reply_ready;

  reply m_reply;
};

} //! builders

} //! cpp_redis
