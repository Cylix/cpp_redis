// The MIT License (MIT)
//
// Copyright (c) 2015-2017 Simon Ninon <simon.ninon@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <string>

#include <cpp_redis/core/sentinel.hpp>
#include <cpp_redis/network/redis_connection.hpp>
#include <cpp_redis/network/tcp_client_iface.hpp>

namespace cpp_redis {

class subscriber {
public:
//! ctor & dtor
#ifndef __CPP_REDIS_USE_CUSTOM_TCP_CLIENT
  subscriber(void);
#endif /* __CPP_REDIS_USE_CUSTOM_TCP_CLIENT */
  explicit subscriber(const std::shared_ptr<network::tcp_client_iface>& tcp_client);
  ~subscriber(void);

  //! copy ctor & assignment operator
  subscriber(const subscriber&) = delete;
  subscriber& operator=(const subscriber&) = delete;

public:
  //! high availability reconnection state
  enum class connect_state {
    dropped,
    start,
    sleeping,
    ok,
    failed,
    lookup_failed,
    stopped
  };

public:
  //! default connect, based on host + port
  typedef std::function<void(const std::string& host, std::size_t port, connect_state status)> connect_callback_t;
  void connect(
    const std::string& host                    = "127.0.0.1",
    std::size_t port                           = 6379,
    const connect_callback_t& connect_callback = nullptr,
    std::uint32_t timeout_msecs                = 0,
    std::int32_t max_reconnects                = 0,
    std::uint32_t reconnect_interval_msecs     = 0);

  //! same as connect, but based on sentinels
  void connect(
    const std::string& name,
    const connect_callback_t& connect_callback = nullptr,
    std::uint32_t timeout_msecs                = 0,
    std::int32_t max_reconnects                = 0,
    std::uint32_t reconnect_interval_msecs     = 0);

  bool is_connected(void) const;
  void disconnect(bool wait_for_removal = false);

  bool is_reconnecting(void) const;
  void cancel_reconnect(void);

  //! ability to authenticate on the redis server if necessary
  //! this method should not be called repeatedly as the storage of reply_callback is NOT threadsafe
  //! calling repeatedly auth() is undefined concerning the execution of the associated callbacks
  typedef std::function<void(reply&)> reply_callback_t;
  subscriber& auth(const std::string& password, const reply_callback_t& reply_callback = nullptr);

  //! subscribe - unsubscribe
  typedef std::function<void(const std::string&, const std::string&)> subscribe_callback_t;
  typedef std::function<void(int64_t)> acknowledgement_callback_t;
  subscriber& subscribe(const std::string& channel, const subscribe_callback_t& callback, const acknowledgement_callback_t& acknowledgement_callback = nullptr);
  subscriber& psubscribe(const std::string& pattern, const subscribe_callback_t& callback, const acknowledgement_callback_t& acknowledgement_callback = nullptr);
  subscriber& unsubscribe(const std::string& channel);
  subscriber& punsubscribe(const std::string& pattern);

  //! commit pipelined transaction
  subscriber& commit(void);

public:
  //! sentinels
  void add_sentinel(const std::string& host, std::size_t port);
  void clear_sentinels(void);

private:
  struct callback_holder {
    subscribe_callback_t subscribe_callback;
    acknowledgement_callback_t acknowledgement_callback;
  };

private:
  void connection_receive_handler(network::redis_connection&, reply& reply);
  void connection_disconnection_handler(network::redis_connection& connection);

  void handle_acknowledgement_reply(const std::vector<reply>& reply);
  void handle_subscribe_reply(const std::vector<reply>& reply);
  void handle_psubscribe_reply(const std::vector<reply>& reply);

  void call_acknowledgement_callback(const std::string& channel, const std::map<std::string, callback_holder>& channels, std::mutex& channels_mtx, int64_t nb_chans);

private:
  //! reconnection handling
  void reconnect(void);
  void re_auth(void);
  void re_subscribe(void);
  bool should_reconnect(void) const;
  void sleep_before_next_reconnect_attempt(void);
  void clear_subscriptions(void);

private:
  //! unprotected
  void unprotected_subscribe(const std::string& channel, const subscribe_callback_t& callback, const acknowledgement_callback_t& acknowledgement_callback);
  void unprotected_psubscribe(const std::string& pattern, const subscribe_callback_t& callback, const acknowledgement_callback_t& acknowledgement_callback);

private:
  //! Holds information regarding the server and port we are connected to
  //! Save it so auto reconnect logic knows what name to ask sentinels about
  std::string m_redis_server;
  std::size_t m_redis_port = 0;
  std::string m_master_name;
  std::string m_password;

  //! tcp client for redis connection
  network::redis_connection m_client;

  //! redis sentinel
  cpp_redis::sentinel m_sentinel;

  //! (re)connect settings
  std::uint32_t m_connect_timeout_msecs    = 0;
  std::int32_t m_max_reconnects            = 0;
  std::uint32_t m_reconnect_interval_msecs = 0;

  //! reconnection status
  std::atomic_bool m_reconnecting = ATOMIC_VAR_INIT(false);
  //! to force cancel reconnection
  std::atomic_bool m_cancel = ATOMIC_VAR_INIT(false);

  //! (p)subscribed channels and their associated channels
  std::map<std::string, callback_holder> m_subscribed_channels;
  std::map<std::string, callback_holder> m_psubscribed_channels;

  //! connect handler
  connect_callback_t m_connect_callback;

  //! thread safety
  std::mutex m_psubscribed_channels_mutex;
  std::mutex m_subscribed_channels_mutex;

  //! auth reply callback
  reply_callback_t m_auth_reply_callback;
};

} // namespace cpp_redis
