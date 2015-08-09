#include "cpp_redis/builders/error_builder.hpp"
#include "cpp_redis/replies/error_reply.hpp"

namespace cpp_redis {

namespace builders {

error_builder::error_builder(void)
: m_reply(nullptr) {}

void
error_builder::build_reply(void) {
    m_reply = std::make_shared<replies::error_reply>(m_string_builder.get_simple_string());
}

builder_iface&
error_builder::operator<<(std::string& buffer) {
    m_string_builder << buffer;

    if (m_string_builder.reply_ready())
        build_reply();

    return *this;
}

bool
error_builder::reply_ready(void) const {
    return m_string_builder.reply_ready();
}

std::shared_ptr<reply>
error_builder::get_reply(void) const {
    return m_reply;
}

const std::string&
error_builder::get_error(void) const {
    return m_string_builder.get_simple_string();
}

} //! builders

} //! cpp_redis
