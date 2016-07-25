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
  redis_client(void) = default;
  ~redis_client(void);

  //! copy ctor & assignment operator
  redis_client(const redis_client&) = delete;
  redis_client& operator=(const redis_client&) = delete;

public:
  //! handle connection
  typedef std::function<void(redis_client&)> disconnection_handler_t;
  void connect(const std::string& host = "127.0.0.1", unsigned int port = 6379,
               const disconnection_handler_t& disconnection_handler = nullptr);
  void disconnect(void);
  bool is_connected(void);

  //! send cmd
  typedef std::function<void(reply&)> reply_callback_t;
  redis_client& send(const std::vector<std::string>& redis_cmd, const reply_callback_t& callback = nullptr);

  //! commit pipelined transaction
  redis_client& commit(void);

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
  std::queue<reply_callback_t> m_callbacks;

  //! user defined disconnection handler
  disconnection_handler_t m_disconnection_handler;

  //! thread safety
  std::mutex m_callbacks_mutex;
  std::mutex m_send_mutex;
};

} //! cpp_redis
