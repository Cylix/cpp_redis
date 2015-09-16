#pragma once

#include <map>
#include <string>
#include <functional>
#include <mutex>

#include "cpp_redis/network/redis_connection.hpp"
#include "cpp_redis/replies/array_reply.hpp"

namespace cpp_redis {

class redis_subscriber {
public:
    //! ctor & dtor
    redis_subscriber(void);
    ~redis_subscriber(void) = default;

    //! copy ctor & assignment operator
    redis_subscriber(const redis_subscriber&) = delete;
    redis_subscriber& operator=(const redis_subscriber&) = delete;

public:
    //! handle connection
    void connect(const std::string& host = "127.0.0.1", unsigned int port = 6379);
    void disconnect(void);
    bool is_connected(void);

    //! disconnection handler
    typedef std::function<void(redis_subscriber&)> disconnection_handler;
    void set_disconnection_handler(const disconnection_handler& handler);

    //! subscribe - unsubscribe
    typedef std::function<void(const std::string&, const std::string&)> subscribe_callback;
    void subscribe(const std::string& channel, const subscribe_callback& callback);
    void psubscribe(const std::string& pattern, const subscribe_callback& callback);
    void unsubscribe(const std::string& channel);
    void punsubscribe(const std::string& pattern);

private:
    void connection_receive_handler(network::redis_connection&, reply& reply);
    void connection_disconnection_handler(network::redis_connection&);

    void handle_subscribe_reply(const std::vector<reply>& reply);
    void handle_psubscribe_reply(const std::vector<reply>& reply);

private:
    //! redis connection
    network::redis_connection m_client;

    //! (p)subscribed channels and their associated channels
    std::map<std::string, subscribe_callback> m_subscribed_channels;
    std::map<std::string, subscribe_callback> m_psubscribed_channels;

    //! disconnection handler
    disconnection_handler m_disconnection_handler;

    //! thread safety
    std::mutex m_disconnection_handler_mutex;
    std::mutex m_psubscribed_channels_mutex;
    std::mutex m_subscribed_channels_mutex;
};

} //! cpp_redis
