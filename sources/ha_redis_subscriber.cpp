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

#include <cpp_redis/ha_redis_subscriber.hpp>
#include <cpp_redis/logger.hpp>
#include <cpp_redis/redis_error.hpp>
#include <cpp_redis/redis_subscriber.hpp>

namespace cpp_redis {

#ifdef __CPP_REDIS_USE_CUSTOM_TCP_CLIENT
ha_redis_subscriber::ha_redis_subscriber(std::int32_t max_reconnects, std::uint32_t reconnect_interval_msecs,
  const std::shared_ptr<network::tcp_client_iface>& tcp_client,
  ha_subscriber_connect_callback_t connect_callback)
: redis_subscriber(tcp_client), m_sentinel(tcp_client), m_connect_callback(connect_callback) {
  m_max_reconnects           = max_reconnects;
  m_reconnect_interval_msecs = reconnect_interval_msecs;
  m_cancel                   = false;
  m_redis_port               = 0;
  __CPP_REDIS_LOG(debug, "cpp_redis::ha_redis_subscriber created");
}
#endif

ha_redis_subscriber::ha_redis_subscriber(std::int32_t max_reconnects, std::uint32_t reconnect_interval_msecs,
  std::uint32_t num_io_workers,
  ha_subscriber_connect_callback_t connect_callback)
: redis_subscriber(num_io_workers), m_sentinel(num_io_workers), m_connect_callback(connect_callback) {
  m_max_reconnects           = max_reconnects;
  m_reconnect_interval_msecs = reconnect_interval_msecs;
  m_cancel                   = false;
  m_redis_port               = 0;
  __CPP_REDIS_LOG(debug, "cpp_redis::ha_redis_subscriber created");
}

ha_redis_subscriber::~ha_redis_subscriber(void) {
  cancel_reconnect();

  //If for some reason sentinel is connected then disconnect now.
  if (m_sentinel.is_connected())
    m_sentinel.disconnect(true);
  __CPP_REDIS_LOG(debug, "cpp_redis::ha_redis_subscriber destroyed");
}

void
ha_redis_subscriber::add_sentinel(const std::string& host, std::size_t port) {
  m_sentinel.add_sentinel(host, port);
}

void
ha_redis_subscriber::cancel_reconnect() {
  m_cancel = true;
}

void
ha_redis_subscriber::connect(std::uint32_t timeout_msecs, const std::string& name) {
  //Save for auto reconnects
  m_master_name           = name;
  m_connect_timeout_msecs = timeout_msecs;

  //We rely on the sentinel to tell use which redis server is currently the master.
  if (m_sentinel.get_master_addr_by_name(name, m_redis_server, m_redis_port, true)) {
    ha_redis_subscriber::internal_connect(m_redis_server, m_redis_port, timeout_msecs);
  }
  else {
    throw redis_error("ha_redis_subscriber::connect() could not find master for name " + name);
  }
}

void
ha_redis_subscriber::internal_connect(const std::string& host, std::size_t port, std::uint32_t timeout_msecs) {

  if (m_connect_callback)
    m_connect_callback(host, port, ha_subscriber_connect_start);

  __CPP_REDIS_LOG(debug, "cpp_redis::ha_redis_subscriber attempts to connect");

  auto disconnect_handler = std::bind(&ha_redis_subscriber::disconnect_handler, this, std::placeholders::_1);
  auto receive_handler    = std::bind(&redis_subscriber::connection_receive_handler, this, std::placeholders::_1, std::placeholders::_2);
  m_client.connect(host, port, disconnect_handler, receive_handler, timeout_msecs);

  if (m_connect_callback)
    m_connect_callback(host, port, ha_subscriber_connect_ok);
  __CPP_REDIS_LOG(info, "cpp_redis::ha_redis_subscriber connected");
}

void
ha_redis_subscriber::disconnect_handler(network::redis_connection&) {

  __CPP_REDIS_LOG(debug, "ha_redis_subscriber has been disconnected. attempting reconnnect in 1/2 second.");

  if (m_connect_callback)
    m_connect_callback(m_redis_server, m_redis_port, ha_subscriber_connect_dropped);

  m_reconnecting = true;

  //We wait for 1/2 second to give the redis server time to recover
  //and also time for sentinels to promote another redis server to master
  //before we attempt to connect to the new master
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  //Lock the callbacks mutex of the base class to prevent more subscriber commands from being issued
  //until our reconnect has completed.
  std::unique_lock<std::mutex> lock_callback(m_subscribed_channels_mutex);

  //Now reconnect automatically to a new master redis server
  std::string host;
  std::size_t port = 0;

  bool forever            = false;
  std::uint32_t attempts  = 0;
  std::int32_t reconnects = m_max_reconnects;
  if (-1 == reconnects)
    forever = true;

  while (!m_cancel && (forever || reconnects > 0)) {
    if (!forever)
      --reconnects;
    ++attempts;

    //We rely on the sentinel to tell use which redis server is currently the master.
    if (m_sentinel.get_master_addr_by_name(m_master_name, host, port, true)) {
      __CPP_REDIS_LOG(warn, std::string("ha_redis_subscriber reconnect attempt #" + std::to_string(attempts) + " to master " + host).c_str());

      //Try catch block because the redis client throws an error if connection cannot be made.
      try {
        internal_connect(host, port, m_connect_timeout_msecs);
      }
      catch (...) {
      }

      if (is_connected()) {
        __CPP_REDIS_LOG(warn, "ha_redis_subscriber reconnected ok");

        //Now re-authorize and select the proper database.
        if (m_password.size() > 0) {
          //Unlock the lock because it cannot be reentered when sending commands as part of auth()
          lock_callback.unlock();

          ha_redis_subscriber::auth(m_password, [&](cpp_redis::reply& reply) {
            if (reply.is_string() && reply.as_string() == "OK") {
              __CPP_REDIS_LOG(warn, "ha_redis_subscriber reconnect authenticated ok");
            }
            else {
              __CPP_REDIS_LOG(warn, std::string("ha_redis_subscriber reconnect failed to authenticate. Redis says: " + reply.as_string()).c_str());
            }
          });
          commit();
        }

        //Unlock the lock because it cannot be reentered when sending commands as part of select()
        if (lock_callback.owns_lock())
          lock_callback.unlock();

        //Now we need to re-subscribe to any channels our user was subscribed to.
        std::string channel;
        std::map<std::string, callback_holder>::iterator it = m_subscribed_channels.begin();
        while (it != m_subscribed_channels.end()) {
          channel = it->first;
          __CPP_REDIS_LOG(info, std::string("ha_redis_subscriber re-subscribed to channel " + channel).c_str());
          m_client.send({"SUBSCRIBE", channel});
          ++it;
        }

        it = m_psubscribed_channels.begin();
        while (it != m_psubscribed_channels.end()) {
          channel = it->first;
          __CPP_REDIS_LOG(info, std::string("ha_redis_subscriber re-subscribed to pchannel " + channel).c_str());
          m_client.send({"PSUBSCRIBE", channel});
          ++it;
        }
        commit();

        break; //Break out of loop. We are good.
      }
      else {
        if (m_connect_callback)
          m_connect_callback(m_redis_server, m_redis_port, ha_subscriber_connect_failed);
      }
    }
    else {
      if (m_connect_callback)
        m_connect_callback(m_redis_server, m_redis_port, ha_subscriber_connect_lookup_failed);
    }

    if (m_reconnect_interval_msecs > 0) {
      if (m_connect_callback)
        m_connect_callback(m_redis_server, m_redis_port, ha_subscriber_connect_sleeping);
      __CPP_REDIS_LOG(warn, "ha_redis_subscriber reconnect failed. sleeping before next attempt");
      std::this_thread::sleep_for(std::chrono::milliseconds(m_reconnect_interval_msecs));
    }
  }

  if (is_connected()) {
    m_reconnecting = false;
    return;
  }

  //We could not get connected!
  m_reconnecting = false;

  //Tell the user we gave up!
  if (m_connect_callback)
    m_connect_callback(m_redis_server, m_redis_port, ha_subscriber_connect_stopped);
}

redis_subscriber&
ha_redis_subscriber::auth(const std::string& password, const reply_callback_t& reply_callback) {
  m_password = password;
  return redis_subscriber::auth(password, reply_callback);
}
}