#include "cpp_redis/replies/array_reply.hpp"

namespace cpp_redis {

namespace replies {

array_reply::array_reply(const std::list<std::shared_ptr<reply>>& rows)
: m_rows(rows) {}

reply::type
array_reply::get_type(void) const {
    return type::array;
}

unsigned int
array_reply::size(void) const {
    return m_rows.size();
}

const std::list<std::shared_ptr<reply>>&
array_reply::get_rows(void) const {
    return m_rows;
}

void
array_reply::set_rows(const std::list<std::shared_ptr<reply>>& rows) {
    m_rows = rows;
}

void
array_reply::add_row(const std::shared_ptr<reply>& row) {
    m_rows.push_back(row);
}

void
array_reply::operator<<(const std::shared_ptr<reply>& row) {
    add_row(row);
}

} //! replies

} //! cpp_redis
