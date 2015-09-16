#pragma once

#include <mutex>
#include <queue>
#include <string>
#include <vector>
#include <functional>

#include "cpp_redis/network/redis_connection.hpp"

namespace cpp_redis {

class redis_client {
public:
    //! ctor & dtor
    redis_client(void);
    ~redis_client(void);

    //! copy ctor & assignment operator
    redis_client(const redis_client&) = delete;
    redis_client& operator=(const redis_client&) = delete;

public:
    //! handle connection
    void connect(const std::string& host = "127.0.0.1", unsigned int port = 6379);
    void disconnect(void);
    bool is_connected(void);

    //! disconnection handler
    typedef std::function<void(redis_client&)> disconnection_handler;
    void set_disconnection_handler(const disconnection_handler& handler);

    //! send cmd
    typedef std::function<void(reply&)> reply_callback;
    void send(const std::vector<std::string>& redis_cmd, const reply_callback& callback = nullptr);

private:
    //! receive & disconnection handlers
    void connection_receive_handler(network::redis_connection&, reply& reply);
    void connection_disconnection_handler(network::redis_connection&);

    void clear_callbacks(void);
    void call_disconnection_handler(void);

private:
    //! tcp client for redis connection
    network::redis_connection m_client;

    //! queue of callback to process
    std::queue<reply_callback> m_callbacks;

    //! user defined disconnection handler
    disconnection_handler m_disconnection_handler;

    //! thread safety
    std::mutex m_disconnection_handler_mutex;
    std::mutex m_callbacks_mutex;
};

} //! cpp_redis
