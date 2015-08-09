#pragma once

namespace cpp_redis {

class reply {
public:
    virtual ~reply(void) = default;

public:
    //! type of reply
    enum class type {
        array,
        bulk_string,
        error,
        integer,
        simple_string
    };

    virtual type get_type(void) const = 0;
};

} //! cpp_redis
