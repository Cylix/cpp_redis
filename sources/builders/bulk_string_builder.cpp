#include "cpp_redis/builders/bulk_string_builder.hpp"
#include "cpp_redis/redis_error.hpp"

namespace cpp_redis {

namespace builders {

bulk_string_builder::bulk_string_builder(void)
: m_str_size(0), m_str(""), m_is_null(false), m_reply_ready(false), m_reply(nullptr) {}

void
bulk_string_builder::build_reply(void) {
    m_reply = std::make_shared<replies::bulk_string_reply>(m_is_null, m_str);
    m_reply_ready = true;
}

bool
bulk_string_builder::fetch_size(std::string& buffer) {
    if (m_int_builder.reply_ready())
        return true;

    m_int_builder << buffer;
    if (not m_int_builder.reply_ready())
        return false;

    m_str_size = m_int_builder.get_integer();
    if (m_str_size == -1) {
        m_is_null = true;
        build_reply();
    }

    return true;
}

void
bulk_string_builder::fetch_str(std::string& buffer) {
    //! if bytes are missing, fetch them from the buffer
    unsigned int nb_bytes_missing = m_str_size - m_str.size();
    if (nb_bytes_missing) {
        unsigned int nb_bytes_to_transfer = buffer.size() < nb_bytes_missing ? buffer.size() : nb_bytes_missing;
        nb_bytes_missing -= nb_bytes_to_transfer;

        m_str.insert(m_str.end(), buffer.begin(), buffer.begin() + nb_bytes_to_transfer);
        buffer.erase(0, nb_bytes_to_transfer);
    }

    //! if after fetching content in the buffer, there are no more missing bytes, check for ending sequence
    //! always wait there are the two chars \r\n before consuming them
    if (not nb_bytes_missing) {
        if (buffer.size() < 2)
            return;

        if (buffer[0] != '\r' or buffer[1] != '\n')
            throw redis_error("Wrong ending sequence");

        buffer.erase(0, 2);
        build_reply();
    }
}

builder_iface&
bulk_string_builder::operator<<(std::string& buffer) {
    if (m_reply_ready)
        return *this;

    //! if we don't have the size, try to get it with the current buffer
    if (not fetch_size(buffer) or m_reply_ready)
        return *this;

    fetch_str(buffer);

    return *this;
}

bool
bulk_string_builder::reply_ready(void) const {
    return m_reply_ready;
}

std::shared_ptr<reply>
bulk_string_builder::get_reply(void) const {
    return m_reply;
}

const std::string&
bulk_string_builder::get_bulk_string(void) const {
    return m_str;
}

bool
bulk_string_builder::is_null(void) const {
    return m_is_null;
}

} //! builders

} //! cpp_redis
