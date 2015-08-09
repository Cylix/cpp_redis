#include "cpp_redis/replies/array_reply.hpp"

namespace cpp_redis {

namespace replies {

reply::type
array_reply::get_type(void) const {
    return type::array;
}

} //! replies

} //! cpp_redis
