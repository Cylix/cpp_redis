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

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include <cpp_redis/redis_client.hpp>
#include <cpp_redis/sentinel.hpp>
#include <cpp_redis/logger.hpp>
#include <cpp_redis/network/redis_connection.hpp>
#include <tacopie/network/io_service.hpp>

namespace cpp_redis {

typedef enum { ha_client_connect_dropped, ha_client_connect_start,
               ha_client_connect_sleeping, ha_client_connect_ok, 
               ha_client_connect_failed, ha_client_connect_lookup_failed, ha_client_connect_stopped
} en_ha_client_connect;

typedef std::function<void(const std::string& host, std::size_t port, en_ha_client_connect status)> ha_connect_callback_t;

class ha_redis_client : public redis_client {
public:
      //! ctor & dtor
      //! max_reconnects indicates how many times the high availability client will
      //! will attempt to find a master in the cluster before failing.
      //! -1 will retry forever.
      //! reconnect_interval_msecs indicates how much many milliseconds to wait
      //! between reconnect attempts.
      //! connect_callback is called whenever connection to redis drops and during the 
      //! automatic reconnect process to give caller feedback.
#ifdef __CPP_REDIS_USE_CUSTOM_TCP_CLIENT
      ha_redis_client(std::int32_t max_reconnects, 
                      std::uint32_t reconnect_interval_msecs,
                      const std::shared_ptr<network::tcp_client_iface>& tcp_client,
                      ha_connect_callback_t connect_callback = nullptr);
#else
      ha_redis_client(std::int32_t max_reconnects,
                      std::uint32_t reconnect_interval_msecs,
                      std::uint32_t num_io_workers = 2,
                      ha_connect_callback_t connect_callback = nullptr);
#endif
      virtual ~ha_redis_client(void);

      //! remove copy ctor & assignment operator
      ha_redis_client(const ha_redis_client&) = delete;
      ha_redis_client& operator=(const ha_redis_client&) = delete;

      //! add a sentinel
      virtual void add_sentinel(const std::string& host, std::size_t port);

      //! clear all sentinels
      virtual void clear_sentinels();

      //! handle connection. timeout is 1st argument is disambiguate from other connect()
      virtual void connect(std::uint32_t timeout_msecs, const std::string& name);

      //! override to capture the auth password to use in reconnects
      virtual redis_client& auth(const std::string& password, const reply_callback_t& reply_callback = nullptr);

      //! override to capture the database index to use in reconnects
      virtual redis_client& select(int index, const reply_callback_t& reply_callback = nullptr);

      //! indicates if the client is currently reconnected to a new master server
      bool is_reconnecting() {return m_reconnecting;}
      
      //! used to cancel a reconnect that may be going on in order to shut down the thread.
      void cancel_reconnect();

      virtual redis_client& send(const std::vector<std::string>& redis_cmd, const reply_callback_t& callback);

private:
      //! receive & disconnection handlers
      void receive_handler(network::redis_connection&, reply& reply);
      void disconnect_handler(network::redis_connection&);

      //The name of our redis master we want to connect to.
      //Save it so auto reconnect logic knows what name to ask sentinels about
      std::string  m_master_name;
      std::string  m_password;
      int          m_database_index;
      
      //Callback to give status to user regarding connections.
      ha_connect_callback_t m_connect_callback;

      //Holds information regarding the server and port we are connected to (can change)
      std::string  m_redis_server;
      std::size_t  m_redis_port;

      //Timeout used on the original connect() call. Needed for reconnects.
      std::uint32_t m_connect_timeout_msecs;

      //! maximum number of times to reconnect once we detect our redis server is down.
      std::uint32_t m_max_reconnects;

      //! amount of time between reconnects.
      std::uint32_t m_reconnect_interval_msecs;

      cpp_redis::sentinel m_sentinel;
      
      //! remove base class methods by making them private
      using redis_client::connect;
      using redis_client::call_disconnection_handler;

      typedef std::function<void(network::redis_connection&)> disconnect_handler_t;
      virtual void internal_connect(const std::string& host, std::size_t port,
                                    std::uint32_t timeout_msecs = 0);

      //! used to tell users we are in the middle of reconnecting.
      std::atomic<bool> m_reconnecting;

      std::atomic<bool> m_cancel;

      //! queue of commands that are outstanding.
      //! length must ALWAYS match m_callbacks queue of base class.
      std::queue<std::vector<std::string>> m_commands;
};

} //! cpp_redis
