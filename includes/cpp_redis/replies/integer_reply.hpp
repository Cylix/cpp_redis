#pragma once

#include "cpp_redis/replies/reply.hpp"

namespace cpp_redis {

namespace replies {

class integer_reply : public reply {
public:
    //! ctor & dtor
    integer_reply(int nbr = 0);
    ~integer_reply(void) = default;

    //! copy ctor & assignment operator
    integer_reply(const integer_reply&) = default;
    integer_reply& operator=(const integer_reply&) = default;

public:
    //! impl
    type get_type(void) const;

    //! getter
    int get_nbr(void) const;

    //! setter
    void set_nbr(int nbr);

private:
    int m_nbr;
};

} //! replies

} //! cpp_redis
