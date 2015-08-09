#pragma once

#include "cpp_redis/builders/builder_iface.hpp"
#include "cpp_redis/builders/integer_builder.hpp"
#include "cpp_redis/replies/array_reply.hpp"

namespace cpp_redis {

namespace builders {

class array_builder : public builder_iface {
public:
    //! ctor & dtor
    array_builder(void);
    ~array_builder(void) = default;

    //! copy ctor & assignment operator
    array_builder(const array_builder&) = delete;
    array_builder& operator=(const array_builder&) = delete;

public:
    //! builder_iface impl
    builder_iface& operator<<(std::string&);
    bool reply_ready(void) const;
    const std::shared_ptr<reply>& get_reply(void) const;

private:
    integer_builder m_int_builder;
    int m_array_size;

    std::unique_ptr<builder_iface> m_current_builder;

    bool m_reply_ready;
    std::shared_ptr<replies::array_reply> m_reply;
};

} //! builders

} //! cpp_redis
