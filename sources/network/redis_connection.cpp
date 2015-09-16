#include "cpp_redis/network/redis_connection.hpp"

namespace cpp_redis {

namespace network {

redis_connection::redis_connection(void)
: m_reply_callback(nullptr)
, m_disconnection_handler(nullptr)
{
    auto disconnection_handler = std::bind(&redis_connection::tcp_client_disconnection_handler, this, std::placeholders::_1);
    m_client.set_disconnection_handler(disconnection_handler);

    auto receive_handler = std::bind(&redis_connection::tcp_client_receive_handler, this, std::placeholders::_1, std::placeholders::_2);
    m_client.set_receive_handler(receive_handler);
}

redis_connection::~redis_connection(void) {
    if (is_connected())
        disconnect();
}

void
redis_connection::connect(const std::string& host, unsigned int port) {
    m_client.connect(host, port);
}

void
redis_connection::disconnect(void) {
    m_client.disconnect();
}

bool
redis_connection::is_connected(void) {
    return m_client.is_connected();
}

std::string
redis_connection::build_command(const std::vector<std::string>& redis_cmd) {
    std::string cmd = "*" + std::to_string(redis_cmd.size()) + "\r\n";

    for (const auto& cmd_part : redis_cmd)
        cmd += "$" + std::to_string(cmd_part.length()) + "\r\n" + cmd_part + "\r\n";

    return cmd;
}

void
redis_connection::send(const std::vector<std::string>& redis_cmd) {
    m_client.send(build_command(redis_cmd));
}

void
redis_connection::set_disconnection_handler(const disconnection_handler& handler) {
    std::lock_guard<std::mutex> lock(m_disconnection_handler_mutex);

    m_disconnection_handler = handler;
}

void
redis_connection::set_reply_callback(const reply_callback& handler) {
    std::lock_guard<std::mutex> lock(m_reply_callback_mutex);

    m_reply_callback = handler;
}

bool
redis_connection::tcp_client_receive_handler(network::tcp_client&, const std::vector<char>& buffer) {
    try {
        m_builder << std::string(buffer.begin(), buffer.end());
    }
    catch (const redis_error& e) {
        return false;
    }

    while (m_builder.reply_available()) {
        std::lock_guard<std::mutex> lock(m_reply_callback_mutex);

        auto reply = m_builder.get_front();
        m_builder.pop_front();

        if (m_reply_callback)
            m_reply_callback(*this, reply);
    }

    return true;
}

void
redis_connection::tcp_client_disconnection_handler(network::tcp_client&) {
    std::lock_guard<std::mutex> lock(m_disconnection_handler_mutex);

    if (m_disconnection_handler)
        m_disconnection_handler(*this);
}

} //! network

} //! cpp_redis
