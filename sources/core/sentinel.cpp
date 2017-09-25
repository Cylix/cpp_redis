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

#include <cpp_redis/core/sentinel.hpp>
#include <cpp_redis/misc/error.hpp>
#include <cpp_redis/misc/logger.hpp>
#include <cpp_redis/network/redis_connection.hpp>
#include <tacopie/network/io_service.hpp>

namespace cpp_redis {

#ifndef __CPP_REDIS_USE_CUSTOM_TCP_CLIENT
sentinel::sentinel(void)
: m_callbacks_running(0) {
  __CPP_REDIS_LOG(debug, "cpp_redis::sentinel created");
}
#endif /* __CPP_REDIS_USE_CUSTOM_TCP_CLIENT */

sentinel::sentinel(const std::shared_ptr<network::tcp_client_iface>& tcp_client)
: m_client(tcp_client)
, m_callbacks_running(0) {
  __CPP_REDIS_LOG(debug, "cpp_redis::sentinel created");
}

sentinel::~sentinel(void) {
  m_sentinels.clear();
  if (m_client.is_connected())
    m_client.disconnect(true);
  __CPP_REDIS_LOG(debug, "cpp_redis::sentinel destroyed");
}

sentinel&
sentinel::add_sentinel(const std::string& host, std::size_t port) {
  m_sentinels.push_back({host, port});
  return *this;
}

void
sentinel::clear_sentinels() {
  m_sentinels.clear();
}

bool
sentinel::get_master_addr_by_name(const std::string& name, std::string& host, std::size_t& port, bool autoconnect) {
  //! reset connection settings
  host.clear();
  port                          = 0;
  std::uint32_t connect_timeout = 2000;

  //! we must have some sentinels to connect to if we are in autoconnect mode
  if (autoconnect && m_sentinels.size() == 0) {
    throw redis_error("No sentinels available. Call add_sentinel() before get_master_addr_by_name()");
  }

  //! if we are not connected and we are not in autoconnect mode, we can't go further in the process
  if (!autoconnect && !is_connected()) {
    throw redis_error("No sentinel connected. Call connect() first or enable autoconnect.");
  }

  if (autoconnect) {
    try {
      //! Will round robin all attached sentinels until it finds one that is online.
      connect_sentinel(connect_timeout, nullptr);
    }
    catch (const redis_error&) {
    }

    //! we failed to connect
    if (!is_connected()) {
      return false;
    }
  }

  //! By now we have a connection to a redis sentinel.
  //! Ask it who the master is.
  send({"SENTINEL", "get-master-addr-by-name", name}, [&](cpp_redis::reply& reply) {
    if (reply.is_array()) {
      auto arr = reply.as_array();
      host     = arr[0].as_string();                         //host
      port     = std::stoi(arr[1].as_string(), nullptr, 10); //port
    }
  });
  sync_commit();

  //! We always close any open connection in auto connect mode
  //! since the sentinel may not be around next time we ask who the master is.
  if (autoconnect) {
    disconnect(true);
  }

  return port != 0;
}

void
sentinel::connect_sentinel(std::uint32_t timeout_msecs, const sentinel_disconnect_handler_t& sentinel_disconnect_handler) {
  if (m_sentinels.size() == 0) {
    throw redis_error("No sentinels available. Call add_sentinel() before connect_sentinel()");
  }

  auto disconnect_handler = std::bind(&sentinel::connection_disconnect_handler, this, std::placeholders::_1);
  auto receive_handler    = std::bind(&sentinel::connection_receive_handler, this, std::placeholders::_1, std::placeholders::_2);

  //! Now try to connect to first sentinel
  std::vector<sentinel_def>::iterator it = m_sentinels.begin();
  bool not_connected                     = true;

  while (not_connected && it != m_sentinels.end()) {
    try {
      __CPP_REDIS_LOG(debug, std::string("cpp_redis::sentinel attempting to connect to host ") + it->get_host());
      m_client.connect(it->get_host(), it->get_port(), disconnect_handler, receive_handler, timeout_msecs);
    }
    catch (const redis_error&) {
      not_connected = true; //Connection failed.
      __CPP_REDIS_LOG(info, std::string("cpp_redis::sentinel unable to connect to sentinel host ") + it->get_host());
    }

    if (is_connected()) {
      not_connected = false;
      __CPP_REDIS_LOG(info, std::string("cpp_redis::sentinel connected ok to host ") + it->get_host());
    }
    else {
      //! Make sure its closed.
      disconnect(true);
      //! Could not connect.  Try the next sentinel.
      ++it;
    }
  }

  if (not_connected) {
    throw redis_error("Unable to connect to any sentinels");
  }

  m_disconnect_handler = sentinel_disconnect_handler;
}

void
sentinel::connect(const std::string& host, std::size_t port,
  const sentinel_disconnect_handler_t& sentinel_disconnect_handler,
  std::uint32_t timeout_msecs) {
  __CPP_REDIS_LOG(debug, "cpp_redis::sentinel attempts to connect");

  auto disconnect_handler = std::bind(&sentinel::connection_disconnect_handler, this, std::placeholders::_1);
  auto receive_handler    = std::bind(&sentinel::connection_receive_handler, this, std::placeholders::_1, std::placeholders::_2);

  m_client.connect(host, port, disconnect_handler, receive_handler, timeout_msecs);

  __CPP_REDIS_LOG(info, "cpp_redis::sentinel connected");

  m_disconnect_handler = sentinel_disconnect_handler;
}

void
sentinel::connection_receive_handler(network::redis_connection&, reply& reply) {
  reply_callback_t callback = nullptr;

  __CPP_REDIS_LOG(info, "cpp_redis::sentinel received reply");
  {
    std::lock_guard<std::mutex> lock(m_callbacks_mutex);
    m_callbacks_running += 1;

    if (m_callbacks.size()) {
      callback = m_callbacks.front();
      m_callbacks.pop();
    }
  }

  if (callback) {
    __CPP_REDIS_LOG(debug, "cpp_redis::sentinel executes reply callback");
    callback(reply);
  }

  {
    std::lock_guard<std::mutex> lock(m_callbacks_mutex);
    m_callbacks_running -= 1;
    m_sync_condvar.notify_all();
  }
}

void
sentinel::clear_callbacks(void) {
  std::lock_guard<std::mutex> lock(m_callbacks_mutex);

  std::queue<reply_callback_t> empty;
  std::swap(m_callbacks, empty);

  m_sync_condvar.notify_all();
}

void
sentinel::call_disconnect_handler(void) {
  if (m_disconnect_handler) {
    __CPP_REDIS_LOG(info, "cpp_redis::sentinel calls disconnect handler");
    m_disconnect_handler(*this);
  }
}

void
sentinel::connection_disconnect_handler(network::redis_connection&) {
  __CPP_REDIS_LOG(warn, "cpp_redis::sentinel has been disconnected");
  clear_callbacks();
  call_disconnect_handler();
}

void
sentinel::disconnect(bool wait_for_removal) {
  __CPP_REDIS_LOG(debug, "cpp_redis::sentinel attempts to disconnect");
  m_client.disconnect(wait_for_removal);
  __CPP_REDIS_LOG(info, "cpp_redis::sentinel disconnected");
}

bool
sentinel::is_connected(void) {
  return m_client.is_connected();
}

sentinel&
sentinel::send(const std::vector<std::string>& redis_cmd, const reply_callback_t& callback) {
  std::lock_guard<std::mutex> lock_callback(m_callbacks_mutex);

  __CPP_REDIS_LOG(info, "cpp_redis::sentinel attempts to store new command in the send buffer");
  m_client.send(redis_cmd);
  m_callbacks.push(callback);
  __CPP_REDIS_LOG(info, "cpp_redis::sentinel stored new command in the send buffer");

  return *this;
}

//! commit pipelined transaction
sentinel&
sentinel::commit(void) {
  try_commit();

  return *this;
}

sentinel&
sentinel::sync_commit() {
  try_commit();
  std::unique_lock<std::mutex> lock_callback(m_callbacks_mutex);
  __CPP_REDIS_LOG(debug, "cpp_redis::sentinel waiting for callbacks to complete");
  m_sync_condvar.wait(lock_callback, [=] { return m_callbacks_running == 0 && m_callbacks.empty(); });
  __CPP_REDIS_LOG(debug, "cpp_redis::sentinel finished waiting for callback completion");
  return *this;
}

void
sentinel::try_commit(void) {
  try {
    __CPP_REDIS_LOG(debug, "cpp_redis::sentinel attempts to send pipelined commands");
    m_client.commit();
    __CPP_REDIS_LOG(info, "cpp_redis::sentinel sent pipelined commands");
  }
  catch (const cpp_redis::redis_error& e) {
    __CPP_REDIS_LOG(error, "cpp_redis::sentinel could not send pipelined commands");
    clear_callbacks();
    throw e;
  }
}

sentinel&
sentinel::ping(const reply_callback_t& reply_callback) {
  send({"PING"}, reply_callback);
  return *this;
}

sentinel&
sentinel::masters(const reply_callback_t& reply_callback) {
  send({"SENTINEL", "MASTERS"}, reply_callback);
  return *this;
}

sentinel&
sentinel::master(const std::string& name, const reply_callback_t& reply_callback) {
  send({"SENTINEL", "MASTER", name}, reply_callback);
  return *this;
}

sentinel&
sentinel::slaves(const std::string& name, const reply_callback_t& reply_callback) {
  send({"SENTINEL", "SLAVES", name}, reply_callback);
  return *this;
}

sentinel&
sentinel::sentinels(const std::string& name, const reply_callback_t& reply_callback) {
  send({"SENTINEL", "SENTINELS", name}, reply_callback);
  return *this;
}

sentinel&
sentinel::ckquorum(const std::string& name, const reply_callback_t& reply_callback) {
  send({"SENTINEL", "CKQUORUM", name}, reply_callback);
  return *this;
}

sentinel&
sentinel::failover(const std::string& name, const reply_callback_t& reply_callback) {
  send({"SENTINEL", "FAILOVER", name}, reply_callback);
  return *this;
}

sentinel&
sentinel::reset(const std::string& pattern, const reply_callback_t& reply_callback) {
  send({"SENTINEL", "RESET", pattern}, reply_callback);
  return *this;
}

sentinel&
sentinel::flushconfig(const reply_callback_t& reply_callback) {
  send({"SENTINEL", "FLUSHCONFIG"}, reply_callback);
  return *this;
}

sentinel&
sentinel::monitor(const std::string& name, const std::string& ip, std::size_t port, std::size_t quorum,
  const reply_callback_t& reply_callback) {
  send({"SENTINEL", "MONITOR", name, ip, std::to_string(port), std::to_string(quorum)}, reply_callback);
  return *this;
}

sentinel&
sentinel::remove(const std::string& name, const reply_callback_t& reply_callback) {
  send({"SENTINEL", "REMOVE", name}, reply_callback);
  return *this;
}

sentinel&
sentinel::set(const std::string& name, const std::string& option, const std::string& value,
  const reply_callback_t& reply_callback) {
  send({"SENTINEL", "SET", name, option, value}, reply_callback);
  return *this;
}

} //! cpp_redis
