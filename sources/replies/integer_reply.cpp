#include "cpp_redis/replies/integer_reply.hpp"

namespace cpp_redis {

integer_reply::integer_reply(int nbr)
: m_nbr(nbr) {}

reply::type
integer_reply::get_type(void) const {
    return type::integer;
}

int
integer_reply::get_nbr(void) const {
    return m_nbr;
}

void
integer_reply::set_nbr(int nbr) {
    m_nbr = nbr;
}

} //! cpp_redis
