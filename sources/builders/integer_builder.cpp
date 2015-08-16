#include "cpp_redis/builders/integer_builder.hpp"
#include "cpp_redis/redis_error.hpp"

namespace cpp_redis {

namespace builders {

integer_builder::integer_builder(void)
: m_nbr(0), m_negative_multiplicator(1), m_reply_ready(false), m_reply(nullptr) {}

void
integer_builder::build_reply(void) {
    m_reply = std::make_shared<integer_reply>(m_nbr);
    m_reply_ready = true;
}

builder_iface&
integer_builder::operator<<(std::string& buffer) {
    if (m_reply_ready)
        return *this;

    unsigned int i;
    for (i = 0; i < buffer.size(); i++) {
        //! check for \r\n ending sequence
        if (buffer[i] == '\r') {
            if (i != buffer.size() and buffer[i + 1] == '\n')
                build_reply();
            else if (i != buffer.size() and buffer[i + 1] != '\n')
                throw redis_error("Invalid character for integer redis reply");

            buffer.erase(i, 2);
            break;
        }

        //! check for negative numbers
        if (not i and not m_nbr and m_negative_multiplicator == 1 and buffer[i] == '-') {
            m_negative_multiplicator = -1;
            continue;
        }
        else if (not std::isdigit(buffer[i]))
            throw redis_error("Invalid character for integer redis reply");

        m_nbr *= 10;
        m_nbr += buffer[i] - '0';
    }

    buffer.erase(0, i);

    return *this;
}

bool
integer_builder::reply_ready(void) const {
    return m_reply_ready;
}

std::shared_ptr<reply>
integer_builder::get_reply(void) const {
    return m_reply;
}

int
integer_builder::get_integer(void) const {
    return m_negative_multiplicator * m_nbr;
}

} //! builders

} //! cpp_redis
