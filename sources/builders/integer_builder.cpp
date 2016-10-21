#include <cctype>

#include <cpp_redis/builders/integer_builder.hpp>
#include <cpp_redis/logger.hpp>
#include <cpp_redis/redis_error.hpp>

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

} //! builders

} //! cpp_redis
