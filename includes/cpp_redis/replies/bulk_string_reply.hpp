#pragma once

#include <string>

#include "cpp_redis/replies/reply.hpp"

namespace cpp_redis {

class bulk_string_reply : public reply {
public:
    //! ctor & dtor
    bulk_string_reply(bool is_null = false, const std::string& bulk_string = "");
    ~bulk_string_reply(void) = default;

    //! copy ctor & assignment operator
    bulk_string_reply(const bulk_string_reply&) = default;
    bulk_string_reply& operator=(const bulk_string_reply&) = default;

public:
    //! impl
    type get_type(void) const;

    //! getters
    bool is_null(void) const;
    const std::string& get_bulk_string(void) const;

    //! setters
    void set_is_null(bool is_null);
    void set_bulk_string(const std::string& bulk_string);

private:
    std::string m_str;
    bool m_is_null;
};

} //! cpp_redis
