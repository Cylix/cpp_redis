#include "cpp_redis/replies/simple_string_reply.hpp"

namespace cpp_redis {

simple_string_reply::simple_string_reply(const std::string& simple_string)
: m_str(simple_string) {}

reply::type
simple_string_reply::get_type(void) const {
    return type::simple_string;
}

const std::string&
simple_string_reply::get_simple_string(void) const {
    return m_str;
}

void
simple_string_reply::set_simple_string(const std::string& simple_string) {
    m_str = simple_string;
}

} //! cpp_redis
