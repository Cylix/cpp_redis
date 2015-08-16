#include "cpp_redis/replies/bulk_string_reply.hpp"

namespace cpp_redis {

bulk_string_reply::bulk_string_reply(bool is_null, const std::string& bulk_string)
: m_str(bulk_string)
, m_is_null(is_null) {}

reply::type
bulk_string_reply::get_type(void) const {
    return type::bulk_string;
}

bool
bulk_string_reply::is_null(void) const {
    return m_is_null;
}

const std::string&
bulk_string_reply::get_bulk_string(void) const {
    return m_str;
}

void
bulk_string_reply::set_is_null(bool is_null) {
    m_is_null = is_null;
}

void
bulk_string_reply::set_bulk_string(const std::string& bulk_string) {
    m_str = bulk_string;
}

} //! cpp_redis
