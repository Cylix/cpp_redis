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

#include <queue>
#include <vector>

#include <cpp_redis/logger.hpp>
#include <cpp_redis/network/redis_connection.hpp>
#include <tacopie/network/io_service.hpp>

namespace cpp_redis {

class sentinel {

private:
  class sentinel_def {
  public:
    //! ctor & dtor
    sentinel_def(const std::string& host, std::size_t port) {
      m_host = host;
      m_port = port;
    }
    virtual ~sentinel_def(){};

  private:
    std::string m_host;
    std::size_t m_port;

  public:
    const std::string&
    get_host() { return m_host; }
    size_t
    get_port() { return m_port; }
  };

  //! A pool of 1 or more sentinels we ask to determine which redis server is the master.
  std::vector<sentinel_def> m_sentinels;

public:
//! ctor & dtor
#ifdef __CPP_REDIS_USE_CUSTOM_TCP_CLIENT
  sentinel(const std::shared_ptr<network::tcp_client_iface>& tcp_client);
#else
  sentinel(void);
#endif

  virtual ~sentinel(void);

  //! remove copy ctor & assignment operator
  sentinel(const sentinel&) = delete;
  sentinel& operator=(const sentinel&) = delete;

  //! send cmd
  typedef std::function<void(reply&)> reply_callback_t;
  virtual sentinel& send(const std::vector<std::string>& sentinel_cmd, const reply_callback_t& callback = nullptr);

  //! commit pipelined transaction
  sentinel& commit(void);
  sentinel& sync_commit();

  template <class Rep, class Period>
  sentinel&
  sync_commit(const std::chrono::duration<Rep, Period>& timeout) {
    try_commit();

    std::unique_lock<std::mutex> lock_callback(m_callbacks_mutex);
    __CPP_REDIS_LOG(debug, "cpp_redis::sentinel waiting for callbacks to complete");
    if (!m_sync_condvar.wait_for(lock_callback, timeout, [=] {
          return m_callbacks_running == 0 && m_callbacks.empty();
        })) {
      __CPP_REDIS_LOG(debug, "cpp_redis::sentinel finished waiting for callback");
    }
    else {
      __CPP_REDIS_LOG(debug, "cpp_redis::sentinel timed out waiting for callback");
    }
    return *this;
  }

  virtual void disconnect(bool wait_for_removal = false);
  bool is_connected(void);

  //! add a sentinel definition. Required for connect() or get_master_addr_by_name()
  //! when autoconnect is enabled.
  sentinel& add_sentinel(const std::string& host, std::size_t port);

  // !clear all existing sentinels.
  void clear_sentinels();

  //! handle connection
  typedef std::function<void(sentinel&)> sentinel_disconnect_handler_t;
  //! Connect to 1st active sentinel we find. Requires add_sentinel() to be called first
  virtual void connect_sentinel(std::uint32_t timeout_msecs = 0,
    const sentinel_disconnect_handler_t& disconnect_handler = nullptr);
  //! Connect to named sentinel
  virtual void connect(const std::string& host, std::size_t port,
    const sentinel_disconnect_handler_t& disconnect_handler = nullptr,
    std::uint32_t timeout_msecs                             = 0);


  //! Used to find the current redis master by asking one or more sentinels. Used high availablity.
  //! Handles connect() and disconnect() automatically when autoconnect=true
  //! This method is synchronous. No need to call sync_commit() or process a reply callback.
  //! Returns true if a master was found and fills in host and port output parameters.
  //! Returns false if no master could be found or no connection could be made.
  //! Call add_sentinel() before using when autoconnect==true
  bool get_master_addr_by_name(const std::string& name, std::string& host,
    std::size_t& port, bool autoconnect = true);

  sentinel& ping(const reply_callback_t& reply_callback = nullptr);
  sentinel& master(const std::string& name, const reply_callback_t& reply_callback = nullptr);
  sentinel& masters(const reply_callback_t& reply_callback = nullptr);
  sentinel& slaves(const std::string& name, const reply_callback_t& reply_callback = nullptr);
  sentinel& sentinels(const std::string& name, const reply_callback_t& reply_callback = nullptr);
  sentinel& ckquorum(const std::string& name, const reply_callback_t& reply_callback = nullptr);
  sentinel& reset(const std::string& pattern, const reply_callback_t& reply_callback = nullptr);
  sentinel& flushconfig(const reply_callback_t& reply_callback = nullptr);
  sentinel& failover(const std::string& name, const reply_callback_t& reply_callback = nullptr);
  sentinel& monitor(const std::string& name, const std::string& ip, std::size_t port, std::size_t quorum,
    const reply_callback_t& reply_callback = nullptr);
  sentinel& remove(const std::string& name, const reply_callback_t& reply_callback = nullptr);
  sentinel& set(const std::string& name, const std::string& option, const std::string& value,
    const reply_callback_t& reply_callback = nullptr);

private:
  //! tcp client for redis sentinel connection
  network::redis_connection m_client;

  //! queue of callback to process
  std::queue<reply_callback_t> m_callbacks;

  //! user defined disconnection handler
  sentinel_disconnect_handler_t m_disconnect_handler;

  //! receive & disconnection handlers
  void connection_receive_handler(network::redis_connection&, reply& reply);
  void connection_disconnect_handler(network::redis_connection&);

  void clear_callbacks(void);
  void call_disconnect_handler(void);

  void try_commit(void);

  //! thread safety
  std::mutex m_callbacks_mutex;
  std::mutex m_send_mutex;
  std::condition_variable m_sync_condvar;
  std::atomic<unsigned int> m_callbacks_running = ATOMIC_VAR_INIT(0);
};

}; // namespace cpp_redis
