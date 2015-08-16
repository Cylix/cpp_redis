#pragma once

namespace cpp_redis {

class array_reply;
class bulk_string_reply;
class error_reply;
class integer_reply;
class simple_string_reply;

class reply {
public:
    //! type of reply
    enum class type {
        array,
        bulk_string,
        error,
        integer,
        simple_string
    };

public:
    //! ctor & dtor
    reply(type reply_type);
    virtual ~reply(void) = default;

    type get_type(void) const;

    bool is_array(void) const;
    bool is_bulk_string(void) const;
    bool is_error(void) const;
    bool is_integer(void) const;
    bool is_simple_string(void) const;

    array_reply& as_array(void);
    bulk_string_reply& as_bulk_string(void);
    error_reply& as_error(void);
    integer_reply& as_integer(void);
    simple_string_reply& as_simple_string(void);

private:
    type m_type;
};

} //! cpp_redis
