#include "cpp_redis/redis_client.hpp"
#include "cpp_redis/redis_error.hpp"

#include <iostream>

namespace cpp_redis {

redis_client::redis_client(void) {
    auto disconnection_handler = std::bind(&redis_client::tcp_client_disconnection_handler, this, std::placeholders::_1);
    m_client.set_disconnection_handler(disconnection_handler);

    auto receive_handler = std::bind(&redis_client::tcp_client_receive_handler, this, std::placeholders::_1, std::placeholders::_2);
    m_client.set_receive_handler(receive_handler);
}

redis_client::~redis_client(void) {
    if (is_connected())
        disconnect();
}

void
redis_client::connect(const std::string& host, unsigned int port) {
    try {
        m_client.connect(host, port);
    }
    catch (const network::tcp_client::tcp_client_error& e) {
        throw redis_error(e.what());
    }
}

void
redis_client::disconnect(void) {
    try {
        m_client.disconnect();
    }
    catch (const network::tcp_client::tcp_client_error& e) {
        throw redis_error(e.what());
    }
}

bool
redis_client::is_connected(void) {
    return m_client.is_connected();
}

std::string
redis_client::build_commad(const std::vector<std::string>& redis_cmd) {
    std::string cmd = "*" + std::to_string(redis_cmd.size()) + "\r\n";

    for (const auto& cmd_part : redis_cmd)
        cmd += "$" + std::to_string(cmd_part.length()) + "\r\n" + cmd_part + "\r\n";

    return cmd;
}

void
redis_client::send(const std::vector<std::string>& redis_cmd, const reply_callback& callback) {
    try {
        std::lock_guard<std::mutex> lock(m_callbacks_mutex);

        m_client.send(build_commad(redis_cmd));
        m_callbacks.push(callback);
    }
    catch (const network::tcp_client::tcp_client_error& e) {
        throw redis_error(e.what());
    }
}

void
redis_client::set_disconnection_handler(const disconnection_handler& handler) {
    std::lock_guard<std::mutex> lock(m_disconnection_handler_mutex);

    m_disconnection_handler = handler;
}

void
redis_client::tcp_client_receive_handler(network::tcp_client&, const std::vector<char>& buffer) {
    std::cout << "recv: " << buffer.data() << std::endl;
}

void
redis_client::clear_callbacks(void) {
    std::lock_guard<std::mutex> lock(m_callbacks_mutex);

    std::queue<reply_callback> empty;
    std::swap(m_callbacks, empty);
}

void
redis_client::call_disconnection_handler(void) {
    std::lock_guard<std::mutex> lock(m_disconnection_handler_mutex);

    if (m_disconnection_handler)
        m_disconnection_handler(*this);
}

void
redis_client::tcp_client_disconnection_handler(network::tcp_client&) {
    clear_callbacks();
    call_disconnection_handler();
}

} //! cpp_redis
