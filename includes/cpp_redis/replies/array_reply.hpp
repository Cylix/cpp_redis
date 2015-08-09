#pragma once

#include "cpp_redis/replies/reply.hpp"

namespace cpp_redis {

namespace replies {

class array_reply : public reply {
public:
    //! ctor & dtor
    array_reply(void) = default;
    ~array_reply(void) = default;

    //! copy ctor & assignment operator
    array_reply(const array_reply&) = default;
    array_reply& operator=(const array_reply&) = default;

public:
    //! impl
    type get_type(void) const;
};

} //! replies

} //! cpp_redis
