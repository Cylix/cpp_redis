#pragma once

#include <mutex>
#include <queue>
#include <string>
#include <vector>
#include <functional>

#include "cpp_redis/network/tcp_client.hpp"
#include "cpp_redis/builders/reply_builder.hpp"

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
    typedef std::function<void(const std::shared_ptr<reply>&)> reply_callback;
    void send(const std::vector<std::string>& redis_cmd, const reply_callback& callback);

private:
    //! receive & disconnection handlers
    void tcp_client_receive_handler(network::tcp_client&, const std::vector<char>& buffer);
    void tcp_client_disconnection_handler(network::tcp_client&);

    void clear_callbacks(void);
    void call_disconnection_handler(void);
    std::string build_commad(const std::vector<std::string>& redis_cmd);

private:
    //! tcp client for redis connection
    network::tcp_client m_client;

    //! queue of callback to process
    std::queue<reply_callback> m_callbacks;

    //! user defined disconnection handler
    disconnection_handler m_disconnection_handler;

    //! reply builder
    builders::reply_builder m_builder;

    //! thread safety
    std::mutex m_disconnection_handler_mutex;
    std::mutex m_callbacks_mutex;
};

} //! cpp_redis
