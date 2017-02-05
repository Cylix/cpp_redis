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

#include <cpp_redis/network/redis_connection.hpp>

namespace cpp_redis {

class redis_subscriber {
public:
  //! ctor & dtor
  redis_subscriber(void);
  ~redis_subscriber(void);

  //! copy ctor & assignment operator
  redis_subscriber(const redis_subscriber&) = delete;
  redis_subscriber& operator=(const redis_subscriber&) = delete;

public:
  //! handle connection
  typedef std::function<void(redis_subscriber&)> disconnection_handler_t;
  void connect(const std::string& host = "127.0.0.1", std::size_t port = 6379, const disconnection_handler_t& disconnection_handler = nullptr);
  void disconnect(void);
  bool is_connected(void);

  //! ability to authenticate on the redis server if necessary
  //! this method should not be called repeatedly as the storage of reply_callback is NOT threadsafe
  //! calling repeatedly auth() is undefined concerning the execution of the associated callbacks
  typedef std::function<void(reply&)> reply_callback_t;
  redis_subscriber& auth(const std::string& password, const reply_callback_t& reply_callback = nullptr);

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

  //! auth reply callback
  reply_callback_t m_auth_reply_callback;
};

} //! cpp_redis
