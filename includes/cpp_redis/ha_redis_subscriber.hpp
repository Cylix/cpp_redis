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
#include <cpp_redis/redis_subscriber.hpp>
#include <cpp_redis/sentinel.hpp>

namespace cpp_redis {

typedef enum { ha_subscriber_connect_dropped, ha_subscriber_connect_start,
               ha_subscriber_connect_sleeping, ha_subscriber_connect_ok, 
               ha_subscriber_connect_failed, ha_subscriber_connect_lookup_failed,
               ha_subscriber_connect_stopped
} en_ha_subscriber_connect;

typedef std::function<void(const std::string& host, std::size_t port, en_ha_subscriber_connect status)> ha_subscriber_connect_callback_t;

class ha_redis_subscriber : public redis_subscriber {
public:
   //! ctor & dtor
   ha_redis_subscriber(std::int32_t max_reconnects, std::uint32_t reconnect_interval_msecs,
                       std::uint32_t num_io_workers=2,
                       ha_subscriber_connect_callback_t connect_callpack = nullptr);
   ha_redis_subscriber(std::int32_t max_reconnects, std::uint32_t reconnect_interval_msecs, 
                       const std::shared_ptr<network::tcp_client_iface>& tcp_client,
                       ha_subscriber_connect_callback_t connect_callpack = nullptr);
   virtual ~ha_redis_subscriber(void);

   //! copy ctor & assignment operator
   ha_redis_subscriber(const ha_redis_subscriber&) = delete;
   ha_redis_subscriber& operator=(const ha_redis_subscriber&) = delete;

   //! add a sentinel
   virtual void add_sentinel(const std::string& host, std::size_t port);

   //! handle connection to named master. msecs must come 1st to disambiguate overloads.
   virtual void connect(std::uint32_t timeout_msecs, const std::string& name);

   //! override to capture the auth password to use in reconnects
   //! ability to authenticate on the redis server if necessary
   //! this method should not be called repeatedly as the storage of reply_callback is NOT threadsafe
   //! calling repeatedly auth() is undefined concerning the execution of the associated callbacks
   typedef std::function<void(reply&)> reply_callback_t;
   virtual redis_subscriber& auth(const std::string& password, const reply_callback_t& reply_callback = nullptr);

   //! indicates if the client is currently reconnected to a new master server
   bool is_reconnecting() { return m_reconnecting; }

   //! used to cancel a reconnect that may be going on in order to shut down the thread.
   void cancel_reconnect();

private:
   //! receive & disconnection handlers
   void disconnect_handler(network::redis_connection&);

   //The name of our redis master we want to connect to.
   //Save it so auto reconnect logic knows what to ask sentinels about
   std::string  m_master_name;
   std::string  m_password;

   //Callback to give status to user regarding connections.
   ha_subscriber_connect_callback_t m_connect_callback;

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
   using redis_subscriber::connect;

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
