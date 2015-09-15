#include "cpp_redis/reply.hpp"
#include "cpp_redis/replies/array_reply.hpp"
#include "cpp_redis/replies/bulk_string_reply.hpp"
#include "cpp_redis/replies/error_reply.hpp"
#include "cpp_redis/replies/integer_reply.hpp"
#include "cpp_redis/replies/simple_string_reply.hpp"

namespace cpp_redis {

reply::reply(void)
: m_type(type::null) {}

reply::reply(const replies::array_reply& array)
: m_type(type::array)
{
    build_from_array(array);
}

reply::reply(const replies::bulk_string_reply& string)
: m_type(string.is_null() ? type::null : type::bulk_string)
, m_str(string.str()) {}

reply::reply(const replies::error_reply& string)
: m_type(type::error)
, m_str(string.str()) {}

reply::reply(const replies::integer_reply& integer)
: m_type(type::integer)
, m_int(integer.val()) {}

reply::reply(const replies::simple_string_reply& string)
: m_type(type::simple_string)
, m_str(string.str()) {}

reply&
reply::operator=(const replies::array_reply& array) {
    build_from_array(array);
    return *this;
}

reply&
reply::operator=(const replies::bulk_string_reply& string) {
    m_type = string.is_null() ? type::null : type::bulk_string;
    m_str = string.str();
    return *this;
}

reply&
reply::operator=(const replies::error_reply& string) {
    m_type = type::error;
    m_str = string.str();
    return *this;
}

reply&
reply::operator=(const replies::integer_reply& integer) {
    m_type = type::integer;
    m_str = integer.val();
    return *this;
}

reply&
reply::operator=(const replies::simple_string_reply& string) {
    m_type = type::simple_string;
    m_str = string.str();
    return *this;
}

void
reply::build_from_array(const replies::array_reply& array) {
    for (const auto& reply_item : array.get_rows()) {
        if (reply_item->is_array())
            m_replies.push_back(reply{ reply_item->as_array() });
        else if (reply_item->is_bulk_string())
            m_replies.push_back(reply{ reply_item->as_bulk_string() });
        else if (reply_item->is_error())
            m_replies.push_back(reply{ reply_item->as_error() });
        else if (reply_item->is_integer())
            m_replies.push_back(reply{ reply_item->as_integer() });
        else if (reply_item->is_simple_string())
            m_replies.push_back(reply{ reply_item->as_simple_string() });
    }
}

bool
reply::is_array(void) const {
    return m_type == type::array;
}

bool
reply::is_string(void) const {
    return is_simple_string() or is_bulk_string() or is_error();
}

bool
reply::is_simple_string(void) const {
    return m_type == type::simple_string;
}

bool
reply::is_bulk_string(void) const {
    return m_type == type::bulk_string;
}

bool
reply::is_error(void) const {
    return m_type == type::error;
}

bool
reply::is_integer(void) const {
    return m_type == type::integer;
}

bool
reply::is_null(void) const {
    return m_type == type::null;
}

const std::vector<reply>&
reply::as_array(void) const {
    return m_replies;
}

const std::string&
reply::as_string(void) const {
    return m_str;
}

int
reply::as_integer(void) const {
    return m_int;
}

reply::type
reply::get_type(void) const {
    return m_type;
}

} //! cpp_redis
