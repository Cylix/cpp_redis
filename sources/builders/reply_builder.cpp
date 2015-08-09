#include "cpp_redis/builders/reply_builder.hpp"
#include "cpp_redis/builders/simple_string_builder.hpp"
#include "cpp_redis/builders/error_builder.hpp"
#include "cpp_redis/builders/integer_builder.hpp"
#include "cpp_redis/builders/bulk_string_builder.hpp"
#include "cpp_redis/builders/array_builder.hpp"
#include "cpp_redis/redis_error.hpp"

namespace cpp_redis {

namespace builders {

reply_builder::reply_builder(void)
: m_builder(nullptr) {}

reply_builder&
reply_builder::operator<<(const std::string& data) {
    m_buffer += data;

    while (build_reply());

    return *this;
}

bool
reply_builder::build_reply(void) {
    if (not m_buffer.size())
        return false;

    if (not m_builder) {
        switch (m_buffer[0]) {
        case '+':
            m_builder = std::unique_ptr<simple_string_builder>{ new simple_string_builder() };
            break;
        case '-':
            m_builder = std::unique_ptr<error_builder>{ new error_builder() };
            break;
        case ':':
            m_builder = std::unique_ptr<integer_builder>{ new integer_builder() };
            break;
        case '$':
            m_builder = std::unique_ptr<bulk_string_builder>{ new bulk_string_builder() };
            break;
        case '*':
            m_builder = std::unique_ptr<array_builder>{ new array_builder() };
            break;
        }

        m_buffer.erase(0);
    }

    *m_builder << m_buffer;

    if (m_builder->reply_ready()) {
        m_available_replies.push_back(m_builder->get_reply());
        m_builder = nullptr;

        return true;
    }

    return false;
}

void
reply_builder::operator>>(std::shared_ptr<reply>& reply) {
    reply = get_reply();
}

std::shared_ptr<reply>
reply_builder::get_reply(void) {
    if (not reply_available())
        throw redis_error("No available reply");

    auto reply = m_available_replies.front();
    m_available_replies.pop_front();
    return reply;
}

bool
reply_builder::reply_available(void) {
    return m_available_replies.size() > 0;
}

} //! builders

} //! cpp_redis
