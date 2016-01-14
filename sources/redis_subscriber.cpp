#include "cpp_redis/redis_subscriber.hpp"
#include "cpp_redis/redis_error.hpp"
#include "cpp_redis/replies/bulk_string_reply.hpp"

namespace cpp_redis {

redis_subscriber::redis_subscriber(void)
: m_disconnection_handler(nullptr) {
    auto disconnection_handler = std::bind(&redis_subscriber::connection_disconnection_handler, this, std::placeholders::_1);
    m_client.set_disconnection_handler(disconnection_handler);

    auto receive_handler = std::bind(&redis_subscriber::connection_receive_handler, this, std::placeholders::_1, std::placeholders::_2);
    m_client.set_reply_callback(receive_handler);
}

void
redis_subscriber::connect(const std::string& host, unsigned int port) {
    m_client.connect(host, port);
}

void
redis_subscriber::disconnect(void) {
    m_client.disconnect();
}

bool
redis_subscriber::is_connected(void) {
    return m_client.is_connected();
}

void
redis_subscriber::set_disconnection_handler(const disconnection_handler& handler) {
    std::lock_guard<std::mutex> lock(m_disconnection_handler_mutex);

    m_disconnection_handler = handler;
}

void
redis_subscriber::subscribe(const std::string& channel, const subscribe_callback& callback) {
    std::lock_guard<std::mutex> lock(m_subscribed_channels_mutex);

    m_subscribed_channels[channel] = callback;
    m_client.send({ "SUBSCRIBE", channel });
}

void
redis_subscriber::psubscribe(const std::string& pattern, const subscribe_callback& callback) {
    std::lock_guard<std::mutex> lock(m_psubscribed_channels_mutex);

    m_psubscribed_channels[pattern] = callback;
    m_client.send({ "PSUBSCRIBE", pattern });
}

void
redis_subscriber::unsubscribe(const std::string& channel) {
    std::lock_guard<std::mutex> lock(m_subscribed_channels_mutex);

    auto it = m_subscribed_channels.find(channel);
    if (it == m_subscribed_channels.end())
      return ;

    m_client.send({ "UNSUBSCRIBE", channel });
    m_subscribed_channels.erase(it);
}

void
redis_subscriber::punsubscribe(const std::string& pattern) {
    std::lock_guard<std::mutex> lock(m_psubscribed_channels_mutex);

    auto it = m_psubscribed_channels.find(pattern);
    if (it == m_psubscribed_channels.end())
      return ;

    m_client.send({ "PUNSUBSCRIBE", pattern });
    m_psubscribed_channels.erase(it);
}

void
redis_subscriber::handle_subscribe_reply(const std::vector<reply>& reply) {
    if (reply.size() != 3)
        return ;

    const auto& title = reply[0];
    const auto& channel = reply[1];
    const auto& message = reply[2];

    if (not title.is_string()
        or not channel.is_string()
        or not message.is_string())
        return ;

    if (title.as_string() != "message")
        return ;

    std::lock_guard<std::mutex> lock(m_subscribed_channels_mutex);

    auto it = m_subscribed_channels.find(channel.as_string());
    if (it == m_subscribed_channels.end())
        return ;

    it->second(channel.as_string(), message.as_string());
}

void
redis_subscriber::handle_psubscribe_reply(const std::vector<reply>& reply) {
    if (reply.size() != 4)
        return ;

    const auto& title = reply[0];
    const auto& pchannel = reply[1];
    const auto& channel = reply[2];
    const auto& message = reply[3];

    if (not title.is_string()
        or not pchannel.is_string()
        or not channel.is_string()
        or not message.is_string())
        return ;

    if (title.as_string() != "pmessage")
        return ;

    std::lock_guard<std::mutex> lock(m_psubscribed_channels_mutex);

    auto it = m_psubscribed_channels.find(pchannel.as_string());
    if (it == m_psubscribed_channels.end())
        return ;

    it->second(channel.as_string(), message.as_string());
}

void
redis_subscriber::connection_receive_handler(network::redis_connection&, reply& reply) {
    //! alaway return an array
    if (not reply.is_array())
        return ;

    auto& array = reply.as_array();

    //! Array size of 3 -> SUBSCRIBE
    //! Array size of 4 -> PSUBSCRIBE
    //! Otherwise -> unexepcted reply
    if (array.size() == 3)
        handle_subscribe_reply(array);
    else if (array.size() == 4)
        handle_psubscribe_reply(array);
}

void
redis_subscriber::connection_disconnection_handler(network::redis_connection&) {
    std::lock_guard<std::mutex> lock(m_disconnection_handler_mutex);

    if (m_disconnection_handler)
        m_disconnection_handler(*this);
}

} //! cpp_redis
