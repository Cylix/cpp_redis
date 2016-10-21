#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <string>

#include <cpp_redis/network/redis_connection.hpp>

namespace cpp_redis {

class redis_subscriber {
public:
  //! ctor & dtor
  redis_subscriber(const std::shared_ptr<network::io_service>& IO = nullptr);
  ~redis_subscriber(void);

  //! copy ctor & assignment operator
  redis_subscriber(const redis_subscriber&) = delete;
  redis_subscriber& operator=(const redis_subscriber&) = delete;

public:
  //! handle connection
  typedef std::function<void(redis_subscriber&)> disconnection_handler_t;
  void connect(const std::string& host = "127.0.0.1", std::size_t port = 6379,
    const disconnection_handler_t& disconnection_handler = nullptr);
  void disconnect(void);
  bool is_connected(void);

  //! subscribe - unsubscribe
  typedef std::function<void(const std::string&, const std::string&)> subscribe_callback_t;
  typedef std::function<void(int64_t)> acknowledgement_callback_t;
  redis_subscriber& subscribe(const std::string& channel, const subscribe_callback_t& callback, const acknowledgement_callback_t& acknowledgement_callback = nullptr);
  redis_subscriber& psubscribe(const std::string& pattern, const subscribe_callback_t& callback, const acknowledgement_callback_t& acknowledgement_callback = nullptr);
  redis_subscriber& unsubscribe(const std::string& channel);
  redis_subscriber& punsubscribe(const std::string& pattern);

  //! commit pipelined transaction
  redis_subscriber& commit(void);

private:
  struct callback_holder {
    subscribe_callback_t subscribe_callback;
    acknowledgement_callback_t acknowledgement_callback;
  };

private:
  void connection_receive_handler(network::redis_connection&, reply& reply);
  void connection_disconnection_handler(network::redis_connection&);

  void handle_acknowledgement_reply(const std::vector<reply>& reply);
  void handle_subscribe_reply(const std::vector<reply>& reply);
  void handle_psubscribe_reply(const std::vector<reply>& reply);

  void call_acknowledgement_callback(const std::string& channel, const std::map<std::string, callback_holder>& channels, std::mutex& channels_mtx, int64_t nb_chans);

private:
  //! redis connection
  network::redis_connection m_client;

  //! (p)subscribed channels and their associated channels
  std::map<std::string, callback_holder> m_subscribed_channels;
  std::map<std::string, callback_holder> m_psubscribed_channels;

  //! disconnection handler
  disconnection_handler_t m_disconnection_handler;

  //! thread safety
  std::mutex m_psubscribed_channels_mutex;
  std::mutex m_subscribed_channels_mutex;
};

} //! cpp_redis
