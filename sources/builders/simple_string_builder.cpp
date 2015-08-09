#include "cpp_redis/builders/simple_string_builder.hpp"

namespace cpp_redis {

namespace builders {

simple_string_builder::simple_string_builder(void)
: m_str(""), m_reply_ready(false), m_reply(nullptr) {}

void
simple_string_builder::build_reply(void) {
    m_reply = std::make_shared<replies::simple_string_reply>(m_str);
    m_reply_ready = true;
}

builder_iface&
simple_string_builder::operator<<(std::string& str) {
    if (m_reply_ready)
        return *this;

    unsigned int nb_bytes_to_transfer = 0;

    //! ending sequence is composed of 2 chars, \r\n
    //! it is possible that these 2 chars are splitted into 2 buffers
    //! so, we check if we have received the first char, \r:
    //!  * if it is alone, we don't consume it and wait to receive the \n
    //!  * otherwise, we consume the ending sequence and mark the reply as built
    auto found = str.find('\r');
    if (found == std::string::npos)
        nb_bytes_to_transfer = str.size();
    else {
        auto last_char_pos = str.size() - 1;

        if (found == last_char_pos)
            nb_bytes_to_transfer = found == 0 ? 0 : found - 1; //! wait
        else if (found != last_char_pos and str[found + 1] == '\n') {
            nb_bytes_to_transfer = found + 1; //! consume
            build_reply();
        }
        else  //! in the case we have something else than \r\n
            nb_bytes_to_transfer = found; //! consume
    }

    //! if ending sequence has been found, copy everything except this sequence
    m_str += str.substr(0, m_reply_ready ? nb_bytes_to_transfer - 2 : nb_bytes_to_transfer);
    str.erase(0, nb_bytes_to_transfer);

    return *this;
}

bool
simple_string_builder::reply_ready(void) const {
    return m_reply_ready;
}

const std::shared_ptr<reply>&
simple_string_builder::get_reply(void) const {
    return m_reply;
}

const std::string&
simple_string_builder::get_simple_string(void) const {
    return m_str;
}

} //! builders

} //! cpp_redis
