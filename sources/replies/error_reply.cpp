#include "cpp_redis/replies/error_reply.hpp"

namespace cpp_redis {

error_reply::error_reply(const std::string& error)
: m_error(error) {}

reply::type
error_reply::get_type(void) const {
    return type::error;
}

const std::string&
error_reply::get_error(void) const {
    return m_error;
}

void
error_reply::set_error(const std::string& error) {
    m_error = error;
}

} //! cpp_redis
