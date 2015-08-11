#include "cpp_redis/builders/reply_builder.hpp"
#include "cpp_redis/builders/builders_factory.hpp"
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
        m_builder = create_builder(m_buffer.front());
        m_buffer.erase(0, 1);
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
