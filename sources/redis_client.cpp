#include "cpp_redis/redis_client.hpp"
#include "cpp_redis/redis_error.hpp"

namespace cpp_redis {

redis_client::redis_client(void) {
    auto disconnection_handler = std::bind(&redis_client::connection_disconnection_handler, this, std::placeholders::_1);
    m_client.set_disconnection_handler(disconnection_handler);

    auto receive_handler = std::bind(&redis_client::connection_receive_handler, this, std::placeholders::_1, std::placeholders::_2);
    m_client.set_reply_callback(receive_handler);
}

redis_client::~redis_client(void) {
    if (is_connected())
        disconnect();
}

void
redis_client::connect(const std::string& host, unsigned int port) {
    m_client.connect(host, port);
}

void
redis_client::disconnect(void) {
    m_client.disconnect();
}

bool
redis_client::is_connected(void) {
    return m_client.is_connected();
}

void
redis_client::send(const std::vector<std::string>& redis_cmd, const reply_callback& callback) {
    std::lock_guard<std::mutex> lock(m_callbacks_mutex);

    m_client.send(redis_cmd);
    m_callbacks.push(callback);
}

void
redis_client::set_disconnection_handler(const disconnection_handler& handler) {
    std::lock_guard<std::mutex> lock(m_disconnection_handler_mutex);

    m_disconnection_handler = handler;
}

void
redis_client::connection_receive_handler(network::redis_connection&, reply& reply) {
    std::lock_guard<std::mutex> lock(m_callbacks_mutex);

    if (not m_callbacks.size())
        return ;

    if (m_callbacks.front())
        m_callbacks.front()(reply);

    m_callbacks.pop();
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
redis_client::connection_disconnection_handler(network::redis_connection&) {
    clear_callbacks();
    call_disconnection_handler();
}

} //! cpp_redis
