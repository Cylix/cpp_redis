#include "cpp_redis/builders/array_builder.hpp"
#include "cpp_redis/builders/error_builder.hpp"
#include "cpp_redis/builders/integer_builder.hpp"
#include "cpp_redis/builders/bulk_string_builder.hpp"
#include "cpp_redis/builders/simple_string_builder.hpp"
#include "cpp_redis/redis_error.hpp"

namespace cpp_redis {

namespace builders {

array_builder::array_builder(void)
: m_current_builder(nullptr)
, m_reply_ready(false)
, m_reply(std::make_shared<replies::array_reply>()) {}

builder_iface&
array_builder::operator<<(std::string& buffer) {
    if (m_reply_ready)
        return *this;

    if (not m_int_builder.reply_ready()) {
        m_int_builder << buffer;

        if (not m_int_builder.reply_ready())
            return *this;

        int size = m_int_builder.get_integer();
        if (size < 0)
            throw redis_error("Invalid array size");

        m_array_size = size;
    }

    if (not buffer.size())
        return *this;

    while (not m_reply_ready) {
        if (not m_current_builder) {
            switch (buffer.front()) {
            case '+':
                m_current_builder = std::unique_ptr<simple_string_builder>{ new simple_string_builder() };
                break;
            case '-':
                m_current_builder = std::unique_ptr<error_builder>{ new error_builder() };
                break;
            case ':':
                m_current_builder = std::unique_ptr<integer_builder>{ new integer_builder() };
                break;
            case '$':
                m_current_builder = std::unique_ptr<bulk_string_builder>{ new bulk_string_builder() };
                break;
            case '*':
                m_current_builder = std::unique_ptr<array_builder>{ new array_builder() };
                break;
            default:
                throw redis_error("Invalid data");
            }

            buffer.erase(0, 1);
        }

        *m_current_builder << buffer;

        if (not m_current_builder->reply_ready())
            return *this;

        *m_reply << m_current_builder->get_reply();
        m_current_builder = nullptr;

        if (m_reply->size() == m_array_size)
            m_reply_ready = true;
    }

    return *this;
}

bool
array_builder::reply_ready(void) const {
    return m_reply_ready;
}

std::shared_ptr<reply>
array_builder::get_reply(void) const {
    return m_reply;
}

} //! builders

} //! cpp_redis
