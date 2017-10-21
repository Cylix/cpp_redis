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

#include <cpp_redis/core/client.hpp>
#include <cpp_redis/misc/error.hpp>

namespace cpp_redis {

#ifndef __CPP_REDIS_USE_CUSTOM_TCP_CLIENT
client::client(void)
: m_reconnecting(false)
, m_cancel(false)
, m_callbacks_running(0) {
  __CPP_REDIS_LOG(debug, "cpp_redis::client created");
}
#endif /* __CPP_REDIS_USE_CUSTOM_TCP_CLIENT */

client::client(const std::shared_ptr<network::tcp_client_iface>& tcp_client)
: m_client(tcp_client)
, m_reconnecting(false)
, m_cancel(false)
, m_callbacks_running(0) {
  __CPP_REDIS_LOG(debug, "cpp_redis::client created");
}

client::~client(void) {
  //! ensure we stopped reconnection attemps
  if (!m_cancel) {
    cancel_reconnect();
  }

  //! If for some reason sentinel is connected then disconnect now.
  if (m_sentinel.is_connected()) {
    m_sentinel.disconnect(true);
  }

  //! disconnect underlying tcp socket
  if (m_client.is_connected()) {
    m_client.disconnect(true);
  }

  __CPP_REDIS_LOG(debug, "cpp_redis::client destroyed");
}

void
client::connect(
  const std::string& name,
  const connect_callback_t& connect_callback,
  std::uint32_t timeout_msecs,
  std::int32_t max_reconnects,
  std::uint32_t reconnect_interval_msecs) {
  //! Save for auto reconnects
  m_master_name = name;

  //! We rely on the sentinel to tell us which redis server is currently the master.
  if (m_sentinel.get_master_addr_by_name(name, m_redis_server, m_redis_port, true)) {
    connect(m_redis_server, m_redis_port, connect_callback, timeout_msecs, max_reconnects, reconnect_interval_msecs);
  }
  else {
    throw redis_error("cpp_redis::client::connect() could not find master for name " + name);
  }
}


void
client::connect(
  const std::string& host, std::size_t port,
  const connect_callback_t& connect_callback,
  std::uint32_t timeout_msecs,
  std::int32_t max_reconnects,
  std::uint32_t reconnect_interval_msecs) {
  __CPP_REDIS_LOG(debug, "cpp_redis::client attempts to connect");

  //! Save for auto reconnects
  m_redis_server             = host;
  m_redis_port               = port;
  m_connect_callback         = connect_callback;
  m_max_reconnects           = max_reconnects;
  m_reconnect_interval_msecs = reconnect_interval_msecs;

  //! notify start
  if (m_connect_callback) {
    m_connect_callback(host, port, connect_state::start);
  }

  auto disconnection_handler = std::bind(&client::connection_disconnection_handler, this, std::placeholders::_1);
  auto receive_handler       = std::bind(&client::connection_receive_handler, this, std::placeholders::_1, std::placeholders::_2);
  m_client.connect(host, port, disconnection_handler, receive_handler, timeout_msecs);

  __CPP_REDIS_LOG(info, "cpp_redis::client connected");

  //! notify end
  if (m_connect_callback) {
    m_connect_callback(m_redis_server, m_redis_port, connect_state::ok);
  }
}

void
client::disconnect(bool wait_for_removal) {
  __CPP_REDIS_LOG(debug, "cpp_redis::client attempts to disconnect");

  //! close connection
  m_client.disconnect(wait_for_removal);

  //! make sure we clear buffer of unsent commands
  clear_callbacks();

  __CPP_REDIS_LOG(info, "cpp_redis::client disconnected");
}

bool
client::is_connected(void) const {
  return m_client.is_connected();
}

void
client::cancel_reconnect(void) {
  m_cancel = true;
}

bool
client::is_reconnecting(void) const {
  return m_reconnecting;
}

void
client::add_sentinel(const std::string& host, std::size_t port) {
  m_sentinel.add_sentinel(host, port);
}

const sentinel&
client::get_sentinel(void) const {
  return m_sentinel;
}

sentinel&
client::get_sentinel(void) {
  return m_sentinel;
}

void
client::clear_sentinels(void) {
  m_sentinel.clear_sentinels();
}

client&
client::send(const std::vector<std::string>& redis_cmd, const reply_callback_t& callback) {
  std::lock_guard<std::mutex> lock_callback(m_callbacks_mutex);

  __CPP_REDIS_LOG(info, "cpp_redis::client attemps to store new command in the send buffer");
  unprotected_send(redis_cmd, callback);
  __CPP_REDIS_LOG(info, "cpp_redis::client stored new command in the send buffer");

  return *this;
}

void
client::unprotected_send(const std::vector<std::string>& redis_cmd, const reply_callback_t& callback) {
  m_client.send(redis_cmd);
  m_commands.push({redis_cmd, callback});
}

//! commit pipelined transaction
client&
client::commit(void) {
  //! no need to call commit in case of reconnection
  //! the reconnection flow will do it for us
  if (!is_reconnecting()) {
    try_commit();
  }

  return *this;
}

client&
client::sync_commit(void) {
  //! no need to call commit in case of reconnection
  //! the reconnection flow will do it for us
  if (!is_reconnecting()) {
    try_commit();
  }

  std::unique_lock<std::mutex> lock_callback(m_callbacks_mutex);
  __CPP_REDIS_LOG(debug, "cpp_redis::client waiting for callbacks to complete");
  m_sync_condvar.wait(lock_callback, [=] { return m_callbacks_running == 0 && m_commands.empty(); });
  __CPP_REDIS_LOG(debug, "cpp_redis::client finished waiting for callback completion");
  return *this;
}

void
client::try_commit(void) {
  try {
    __CPP_REDIS_LOG(debug, "cpp_redis::client attempts to send pipelined commands");
    m_client.commit();
    __CPP_REDIS_LOG(info, "cpp_redis::client sent pipelined commands");
  }
  catch (const cpp_redis::redis_error& e) {
    __CPP_REDIS_LOG(error, "cpp_redis::client could not send pipelined commands");
    //! ensure commands are flushed
    clear_callbacks();
    throw e;
  }
}

void
client::connection_receive_handler(network::redis_connection&, reply& reply) {
  reply_callback_t callback = nullptr;

  __CPP_REDIS_LOG(info, "cpp_redis::client received reply");
  {
    std::lock_guard<std::mutex> lock(m_callbacks_mutex);
    m_callbacks_running += 1;

    if (m_commands.size()) {
      callback = m_commands.front().callback;
      m_commands.pop();
    }
  }

  if (callback) {
    __CPP_REDIS_LOG(debug, "cpp_redis::client executes reply callback");
    callback(reply);
  }

  {
    std::lock_guard<std::mutex> lock(m_callbacks_mutex);
    m_callbacks_running -= 1;
    m_sync_condvar.notify_all();
  }
}

void
client::clear_callbacks(void) {
  if (m_commands.empty()) {
    return;
  }

  //! dequeue commands and move them to a local variable
  std::queue<command_request> commands = std::move(m_commands);

  m_callbacks_running += commands.size();

  std::thread t([=]() mutable {
    while (!commands.empty()) {
      const auto& callback = commands.front().callback;

      if (callback) {
        reply r = {"network failure", reply::string_type::error};
        callback(r);
      }

      --m_callbacks_running;
      commands.pop();
    }

    m_sync_condvar.notify_all();
  });
  t.detach();
}

void
client::resend_failed_commands(void) {
  if (m_commands.empty()) {
    return;
  }

  //! dequeue commands and move them to a local variable
  std::queue<command_request> commands = std::move(m_commands);

  while (m_commands.size() > 0) {
    //! Reissue the pending command and its callback.
    unprotected_send(commands.front().command, commands.front().callback);

    commands.pop();
  }
}

void
client::connection_disconnection_handler(network::redis_connection&) {
  //! leave right now if we are already dealing with reconnection
  if (is_reconnecting()) {
    return;
  }

  //! initiate reconnection process
  m_reconnecting               = true;
  m_current_reconnect_attempts = 0;

  __CPP_REDIS_LOG(warn, "cpp_redis::client has been disconnected");

  if (m_connect_callback) {
    m_connect_callback(m_redis_server, m_redis_port, connect_state::dropped);
  }

  //! Lock the callbacks mutex of the base class to prevent more client commands from being issued until our reconnect has completed.
  std::lock_guard<std::mutex> lock_callback(m_callbacks_mutex);

  while (should_reconnect()) {
    sleep_before_next_reconnect_attempt();
    reconnect();
  }

  if (!is_connected()) {
    clear_callbacks();

    //! Tell the user we gave up!
    if (m_connect_callback) {
      m_connect_callback(m_redis_server, m_redis_port, connect_state::stopped);
    }
  }

  //! terminate reconnection
  m_reconnecting = false;
}

void
client::sleep_before_next_reconnect_attempt(void) {
  if (m_reconnect_interval_msecs <= 0) {
    return;
  }

  if (m_connect_callback) {
    m_connect_callback(m_redis_server, m_redis_port, connect_state::sleeping);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(m_reconnect_interval_msecs));
}

bool
client::should_reconnect(void) const {
  return !is_connected() && !m_cancel && (m_max_reconnects == -1 || m_current_reconnect_attempts < m_max_reconnects);
}

void
client::re_auth(void) {
  if (m_password.empty()) {
    return;
  }

  unprotected_auth(m_password, [&](cpp_redis::reply& reply) {
    if (reply.is_string() && reply.as_string() == "OK") {
      __CPP_REDIS_LOG(warn, "client successfully re-authenticated");
    }
    else {
      __CPP_REDIS_LOG(warn, std::string("client failed to re-authenticate: " + reply.as_string()).c_str());
    }
  });
}

void
client::re_select(void) {
  if (m_database_index <= 0) {
    return;
  }

  unprotected_select(m_database_index, [&](cpp_redis::reply& reply) {
    if (reply.is_string() && reply.as_string() == "OK") {
      __CPP_REDIS_LOG(warn, "client successfully re-selected redis database");
    }
    else {
      __CPP_REDIS_LOG(warn, std::string("client failed to re-select database: " + reply.as_string()).c_str());
    }
  });
}

void
client::reconnect(void) {
  //! increase the number of attemps to reconnect
  ++m_current_reconnect_attempts;


  //! We rely on the sentinel to tell us which redis server is currently the master.
  if (!m_master_name.empty() && !m_sentinel.get_master_addr_by_name(m_master_name, m_redis_server, m_redis_port, true)) {
    if (m_connect_callback) {
      m_connect_callback(m_redis_server, m_redis_port, connect_state::lookup_failed);
    }
    return;
  }

  //! Try catch block because the redis client throws an error if connection cannot be made.
  try {
    connect(m_redis_server, m_redis_port, m_connect_callback, m_connect_timeout_msecs, m_max_reconnects, m_reconnect_interval_msecs);
  }
  catch (...) {
  }

  if (!is_connected()) {
    if (m_connect_callback) {
      m_connect_callback(m_redis_server, m_redis_port, connect_state::failed);
    }
    return;
  }

  //! notify end
  if (m_connect_callback) {
    m_connect_callback(m_redis_server, m_redis_port, connect_state::ok);
  }

  __CPP_REDIS_LOG(info, "client reconnected ok");

  re_auth();
  re_select();
  resend_failed_commands();
  try_commit();
}

std::string
client::aggregate_method_to_string(aggregate_method method) const {
  switch (method) {
  case aggregate_method::sum: return "SUM";
  case aggregate_method::min: return "MIN";
  case aggregate_method::max: return "MAX";
  default: return "";
  }
}

std::string
client::geo_unit_to_string(geo_unit unit) const {
  switch (unit) {
  case geo_unit::m: return "m";
  case geo_unit::km: return "km";
  case geo_unit::ft: return "ft";
  case geo_unit::mi: return "mi";
  default: return "";
  }
}

std::string
client::bitfield_operation_type_to_string(bitfield_operation_type operation) const {
  switch (operation) {
  case bitfield_operation_type::get: return "GET";
  case bitfield_operation_type::set: return "SET";
  case bitfield_operation_type::incrby: return "INCRBY";
  default: return "";
  }
}

std::string
client::overflow_type_to_string(overflow_type type) const {
  switch (type) {
  case overflow_type::wrap: return "WRAP";
  case overflow_type::sat: return "SAT";
  case overflow_type::fail: return "FAIL";
  default: return "";
  }
}

client::bitfield_operation
client::bitfield_operation::get(const std::string& type, int offset, overflow_type overflow) {
  return {bitfield_operation_type::get, type, offset, 0, overflow};
}

client::bitfield_operation
client::bitfield_operation::set(const std::string& type, int offset, int value, overflow_type overflow) {
  return {bitfield_operation_type::set, type, offset, value, overflow};
}

client::bitfield_operation
client::bitfield_operation::incrby(const std::string& type, int offset, int increment, overflow_type overflow) {
  return {bitfield_operation_type::incrby, type, offset, increment, overflow};
}

//!
//! Redis commands
//! Callback-based
//!

client&
client::append(const std::string& key, const std::string& value, const reply_callback_t& reply_callback) {
  send({"APPEND", key, value}, reply_callback);
  return *this;
}

client&
client::auth(const std::string& password, const reply_callback_t& reply_callback) {
  std::lock_guard<std::mutex> lock(m_callbacks_mutex);

  unprotected_auth(password, reply_callback);

  return *this;
}

void
client::unprotected_auth(const std::string& password, const reply_callback_t& reply_callback) {
  //! save the password for reconnect attempts.
  m_password = password;
  //! store command in pipeline
  unprotected_send({"AUTH", password}, reply_callback);
}

client&
client::bgrewriteaof(const reply_callback_t& reply_callback) {
  send({"BGREWRITEAOF"}, reply_callback);
  return *this;
}

client&
client::bgsave(const reply_callback_t& reply_callback) {
  send({"BGSAVE"}, reply_callback);
  return *this;
}

client&
client::bitcount(const std::string& key, const reply_callback_t& reply_callback) {
  send({"BITCOUNT", key}, reply_callback);
  return *this;
}

client&
client::bitcount(const std::string& key, int start, int end, const reply_callback_t& reply_callback) {
  send({"BITCOUNT", key, std::to_string(start), std::to_string(end)}, reply_callback);
  return *this;
}

client&
client::bitfield(const std::string& key, const std::vector<bitfield_operation>& operations, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"BITFIELD", key};

  for (const auto& operation : operations) {
    cmd.push_back(bitfield_operation_type_to_string(operation.operation_type));
    cmd.push_back(operation.type);
    cmd.push_back(std::to_string(operation.offset));

    if (operation.operation_type == bitfield_operation_type::set || operation.operation_type == bitfield_operation_type::incrby) {
      cmd.push_back(std::to_string(operation.value));
    }

    if (operation.overflow != overflow_type::server_default) {
      cmd.push_back("OVERFLOW");
      cmd.push_back(overflow_type_to_string(operation.overflow));
    }
  }

  send(cmd, reply_callback);
  return *this;
}

client&
client::bitop(const std::string& operation, const std::string& destkey, const std::vector<std::string>& keys, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"BITOP", operation, destkey};
  cmd.insert(cmd.end(), keys.begin(), keys.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::bitpos(const std::string& key, int bit, const reply_callback_t& reply_callback) {
  send({"BITPOS", key, std::to_string(bit)}, reply_callback);
  return *this;
}

client&
client::bitpos(const std::string& key, int bit, int start, const reply_callback_t& reply_callback) {
  send({"BITPOS", key, std::to_string(bit), std::to_string(start)}, reply_callback);
  return *this;
}

client&
client::bitpos(const std::string& key, int bit, int start, int end, const reply_callback_t& reply_callback) {
  send({"BITPOS", key, std::to_string(bit), std::to_string(start), std::to_string(end)}, reply_callback);
  return *this;
}

client&
client::blpop(const std::vector<std::string>& keys, int timeout, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"BLPOP"};
  cmd.insert(cmd.end(), keys.begin(), keys.end());
  cmd.push_back(std::to_string(timeout));
  send(cmd, reply_callback);
  return *this;
}

client&
client::brpop(const std::vector<std::string>& keys, int timeout, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"BRPOP"};
  cmd.insert(cmd.end(), keys.begin(), keys.end());
  cmd.push_back(std::to_string(timeout));
  send(cmd, reply_callback);
  return *this;
}

client&
client::brpoplpush(const std::string& src, const std::string& dst, int timeout, const reply_callback_t& reply_callback) {
  send({"BRPOPLPUSH", src, dst, std::to_string(timeout)}, reply_callback);
  return *this;
}

client&
client::client_list(const reply_callback_t& reply_callback) {
  send({"CLIENT", "LIST"}, reply_callback);
  return *this;
}

client&
client::client_getname(const reply_callback_t& reply_callback) {
  send({"CLIENT", "GETNAME"}, reply_callback);
  return *this;
}

client&
client::client_pause(int timeout, const reply_callback_t& reply_callback) {
  send({"CLIENT", "PAUSE", std::to_string(timeout)}, reply_callback);
  return *this;
}

client&
client::client_reply(const std::string& mode, const reply_callback_t& reply_callback) {
  send({"CLIENT", "REPLY", mode}, reply_callback);
  return *this;
}

client&
client::client_setname(const std::string& name, const reply_callback_t& reply_callback) {
  send({"CLIENT", "SETNAME", name}, reply_callback);
  return *this;
}

client&
client::cluster_addslots(const std::vector<std::string>& p_slots, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"CLUSTER", "ADDSLOTS"};
  cmd.insert(cmd.end(), p_slots.begin(), p_slots.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::cluster_count_failure_reports(const std::string& node_id, const reply_callback_t& reply_callback) {
  send({"CLUSTER", "COUNT-FAILURE-REPORTS", node_id}, reply_callback);
  return *this;
}

client&
client::cluster_countkeysinslot(const std::string& slot, const reply_callback_t& reply_callback) {
  send({"CLUSTER", "COUNTKEYSINSLOT", slot}, reply_callback);
  return *this;
}

client&
client::cluster_delslots(const std::vector<std::string>& p_slots, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"CLUSTER", "DELSLOTS"};
  cmd.insert(cmd.end(), p_slots.begin(), p_slots.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::cluster_failover(const reply_callback_t& reply_callback) {
  send({"CLUSTER", "FAILOVER"}, reply_callback);
  return *this;
}

client&
client::cluster_failover(const std::string& mode, const reply_callback_t& reply_callback) {
  send({"CLUSTER", "FAILOVER", mode}, reply_callback);
  return *this;
}

client&
client::cluster_forget(const std::string& node_id, const reply_callback_t& reply_callback) {
  send({"CLUSTER", "FORGET", node_id}, reply_callback);
  return *this;
}

client&
client::cluster_getkeysinslot(const std::string& slot, int count, const reply_callback_t& reply_callback) {
  send({"CLUSTER", "GETKEYSINSLOT", slot, std::to_string(count)}, reply_callback);
  return *this;
}

client&
client::cluster_info(const reply_callback_t& reply_callback) {
  send({"CLUSTER", "INFO"}, reply_callback);
  return *this;
}

client&
client::cluster_keyslot(const std::string& key, const reply_callback_t& reply_callback) {
  send({"CLUSTER", "KEYSLOT", key}, reply_callback);
  return *this;
}

client&
client::cluster_meet(const std::string& ip, int port, const reply_callback_t& reply_callback) {
  send({"CLUSTER", "MEET", ip, std::to_string(port)}, reply_callback);
  return *this;
}

client&
client::cluster_nodes(const reply_callback_t& reply_callback) {
  send({"CLUSTER", "NODES"}, reply_callback);
  return *this;
}

client&
client::cluster_replicate(const std::string& node_id, const reply_callback_t& reply_callback) {
  send({"CLUSTER", "REPLICATE", node_id}, reply_callback);
  return *this;
}

client&
client::cluster_reset(const reply_callback_t& reply_callback) {
  send({"CLUSTER", "RESET"}, reply_callback);
  return *this;
}

client&
client::cluster_reset(const std::string& mode, const reply_callback_t& reply_callback) {
  send({"CLUSTER", "RESET", mode}, reply_callback);
  return *this;
}

client&
client::cluster_saveconfig(const reply_callback_t& reply_callback) {
  send({"CLUSTER", "SAVECONFIG"}, reply_callback);
  return *this;
}

client&
client::cluster_set_config_epoch(const std::string& epoch, const reply_callback_t& reply_callback) {
  send({"CLUSTER", "SET-CONFIG-EPOCH", epoch}, reply_callback);
  return *this;
}

client&
client::cluster_setslot(const std::string& slot, const std::string& mode, const reply_callback_t& reply_callback) {
  send({"CLUSTER", "SETSLOT", slot, mode}, reply_callback);
  return *this;
}

client&
client::cluster_setslot(const std::string& slot, const std::string& mode, const std::string& node_id, const reply_callback_t& reply_callback) {
  send({"CLUSTER", "SETSLOT", slot, mode, node_id}, reply_callback);
  return *this;
}

client&
client::cluster_slaves(const std::string& node_id, const reply_callback_t& reply_callback) {
  send({"CLUSTER", "SLAVES", node_id}, reply_callback);
  return *this;
}

client&
client::cluster_slots(const reply_callback_t& reply_callback) {
  send({"CLUSTER", "SLOTS"}, reply_callback);
  return *this;
}

client&
client::command(const reply_callback_t& reply_callback) {
  send({"COMMAND"}, reply_callback);
  return *this;
}

client&
client::command_count(const reply_callback_t& reply_callback) {
  send({"COMMAND", "COUNT"}, reply_callback);
  return *this;
}

client&
client::command_getkeys(const reply_callback_t& reply_callback) {
  send({"COMMAND", "GETKEYS"}, reply_callback);
  return *this;
}

client&
client::command_info(const std::vector<std::string>& command_name, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"COMMAND", "COUNT"};
  cmd.insert(cmd.end(), command_name.begin(), command_name.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::config_get(const std::string& param, const reply_callback_t& reply_callback) {
  send({"CONFIG", "GET", param}, reply_callback);
  return *this;
}

client&
client::config_rewrite(const reply_callback_t& reply_callback) {
  send({"CONFIG", "REWRITE"}, reply_callback);
  return *this;
}

client&
client::config_set(const std::string& param, const std::string& val, const reply_callback_t& reply_callback) {
  send({"CONFIG", "SET", param, val}, reply_callback);
  return *this;
}

client&
client::config_resetstat(const reply_callback_t& reply_callback) {
  send({"CONFIG", "RESETSTAT"}, reply_callback);
  return *this;
}

client&
client::dbsize(const reply_callback_t& reply_callback) {
  send({"DBSIZE"}, reply_callback);
  return *this;
}

client&
client::debug_object(const std::string& key, const reply_callback_t& reply_callback) {
  send({"DEBUG", "OBJECT", key}, reply_callback);
  return *this;
}

client&
client::debug_segfault(const reply_callback_t& reply_callback) {
  send({"DEBUG", "SEGFAULT"}, reply_callback);
  return *this;
}

client&
client::decr(const std::string& key, const reply_callback_t& reply_callback) {
  send({"DECR", key}, reply_callback);
  return *this;
}

client&
client::decrby(const std::string& key, int val, const reply_callback_t& reply_callback) {
  send({"DECRBY", key, std::to_string(val)}, reply_callback);
  return *this;
}

client&
client::del(const std::vector<std::string>& key, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"DEL"};
  cmd.insert(cmd.end(), key.begin(), key.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::discard(const reply_callback_t& reply_callback) {
  send({"DISCARD"}, reply_callback);
  return *this;
}

client&
client::dump(const std::string& key, const reply_callback_t& reply_callback) {
  send({"DUMP", key}, reply_callback);
  return *this;
}

client&
client::echo(const std::string& msg, const reply_callback_t& reply_callback) {
  send({"ECHO", msg}, reply_callback);
  return *this;
}

client&
client::eval(const std::string& script, int numkeys, const std::vector<std::string>& keys, const std::vector<std::string>& args, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"EVAL", script, std::to_string(numkeys)};
  cmd.insert(cmd.end(), keys.begin(), keys.end());
  cmd.insert(cmd.end(), args.begin(), args.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::evalsha(const std::string& sha1, int numkeys, const std::vector<std::string>& keys, const std::vector<std::string>& args, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"EVALSHA", sha1, std::to_string(numkeys)};
  cmd.insert(cmd.end(), keys.begin(), keys.end());
  cmd.insert(cmd.end(), args.begin(), args.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::exec(const reply_callback_t& reply_callback) {
  send({"EXEC"}, reply_callback);
  return *this;
}

client&
client::exists(const std::vector<std::string>& keys, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"EXISTS"};
  cmd.insert(cmd.end(), keys.begin(), keys.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::expire(const std::string& key, int seconds, const reply_callback_t& reply_callback) {
  send({"EXPIRE", key, std::to_string(seconds)}, reply_callback);
  return *this;
}

client&
client::expireat(const std::string& key, int timestamp, const reply_callback_t& reply_callback) {
  send({"EXPIREAT", key, std::to_string(timestamp)}, reply_callback);
  return *this;
}

client&
client::flushall(const reply_callback_t& reply_callback) {
  send({"FLUSHALL"}, reply_callback);
  return *this;
}

client&
client::flushdb(const reply_callback_t& reply_callback) {
  send({"FLUSHDB"}, reply_callback);
  return *this;
}

client&
client::geoadd(const std::string& key, const std::vector<std::tuple<std::string, std::string, std::string>>& long_lat_memb, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"GEOADD", key};
  for (const auto& obj : long_lat_memb) {
    cmd.push_back(std::get<0>(obj));
    cmd.push_back(std::get<1>(obj));
    cmd.push_back(std::get<2>(obj));
  }
  send(cmd, reply_callback);
  return *this;
}

client&
client::geohash(const std::string& key, const std::vector<std::string>& members, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"GEOHASH", key};
  cmd.insert(cmd.end(), members.begin(), members.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::geopos(const std::string& key, const std::vector<std::string>& members, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"GEOPOS", key};
  cmd.insert(cmd.end(), members.begin(), members.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::geodist(const std::string& key, const std::string& member_1, const std::string& member_2, const reply_callback_t& reply_callback) {
  send({"GEODIST", key, member_1, member_2}, reply_callback);
  return *this;
}

client&
client::geodist(const std::string& key, const std::string& member_1, const std::string& member_2, const std::string& unit, const reply_callback_t& reply_callback) {
  send({"GEODIST", key, member_1, member_2, unit}, reply_callback);
  return *this;
}

client&
client::georadius(const std::string& key, double longitude, double latitude, double radius, geo_unit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, const reply_callback_t& reply_callback) {
  return georadius(key, longitude, latitude, radius, unit, with_coord, with_dist, with_hash, asc_order, 0, "", "", reply_callback);
}

client&
client::georadius(const std::string& key, double longitude, double latitude, double radius, geo_unit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, std::size_t count, const reply_callback_t& reply_callback) {
  return georadius(key, longitude, latitude, radius, unit, with_coord, with_dist, with_hash, asc_order, count, "", "", reply_callback);
}

client&
client::georadius(const std::string& key, double longitude, double latitude, double radius, geo_unit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, const std::string& store_key, const reply_callback_t& reply_callback) {
  return georadius(key, longitude, latitude, radius, unit, with_coord, with_dist, with_hash, asc_order, 0, store_key, "", reply_callback);
}

client&
client::georadius(const std::string& key, double longitude, double latitude, double radius, geo_unit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, const std::string& store_key, const std::string& storedist_key, const reply_callback_t& reply_callback) {
  return georadius(key, longitude, latitude, radius, unit, with_coord, with_dist, with_hash, asc_order, 0, store_key, storedist_key, reply_callback);
}

client&
client::georadius(const std::string& key, double longitude, double latitude, double radius, geo_unit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, std::size_t count, const std::string& store_key, const reply_callback_t& reply_callback) {
  return georadius(key, longitude, latitude, radius, unit, with_coord, with_dist, with_hash, asc_order, count, store_key, "", reply_callback);
}

client&
client::georadius(const std::string& key, double longitude, double latitude, double radius, geo_unit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, std::size_t count, const std::string& store_key, const std::string& storedist_key, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"GEORADIUS", key, std::to_string(longitude), std::to_string(latitude), std::to_string(radius), geo_unit_to_string(unit)};

  //! with_coord (optional)
  if (with_coord) {
    cmd.push_back("WITHCOORD");
  }

  //! with_dist (optional)
  if (with_dist) {
    cmd.push_back("WITHDIST");
  }

  //! with_hash (optional)
  if (with_hash) {
    cmd.push_back("WITHHASH");
  }

  //! order (optional)
  cmd.push_back(asc_order ? "ASC" : "DESC");

  //! count (optional)
  if (count > 0) {
    cmd.push_back("COUNT");
    cmd.push_back(std::to_string(count));
  }

  //! store_key (optional)
  if (!store_key.empty()) {
    cmd.push_back("STOREDIST");
    cmd.push_back(storedist_key);
  }

  //! storedist_key (optional)
  if (!storedist_key.empty()) {
    cmd.push_back("STOREDIST");
    cmd.push_back(storedist_key);
  }

  send(cmd, reply_callback);
  return *this;
}

client&
client::georadiusbymember(const std::string& key, const std::string& member, double radius, geo_unit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, const reply_callback_t& reply_callback) {
  return georadiusbymember(key, member, radius, unit, with_coord, with_dist, with_hash, asc_order, 0, "", "", reply_callback);
}

client&
client::georadiusbymember(const std::string& key, const std::string& member, double radius, geo_unit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, std::size_t count, const reply_callback_t& reply_callback) {
  return georadiusbymember(key, member, radius, unit, with_coord, with_dist, with_hash, asc_order, count, "", "", reply_callback);
}

client&
client::georadiusbymember(const std::string& key, const std::string& member, double radius, geo_unit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, const std::string& store_key, const reply_callback_t& reply_callback) {
  return georadiusbymember(key, member, radius, unit, with_coord, with_dist, with_hash, asc_order, 0, store_key, "", reply_callback);
}

client&
client::georadiusbymember(const std::string& key, const std::string& member, double radius, geo_unit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, const std::string& store_key, const std::string& storedist_key, const reply_callback_t& reply_callback) {
  return georadiusbymember(key, member, radius, unit, with_coord, with_dist, with_hash, asc_order, 0, store_key, storedist_key, reply_callback);
}

client&
client::georadiusbymember(const std::string& key, const std::string& member, double radius, geo_unit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, std::size_t count, const std::string& store_key, const reply_callback_t& reply_callback) {
  return georadiusbymember(key, member, radius, unit, with_coord, with_dist, with_hash, asc_order, count, store_key, "", reply_callback);
}

client&
client::georadiusbymember(const std::string& key, const std::string& member, double radius, geo_unit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, std::size_t count, const std::string& store_key, const std::string& storedist_key, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"GEORADIUSBYMEMBER", key, member, std::to_string(radius), geo_unit_to_string(unit)};

  //! with_coord (optional)
  if (with_coord) {
    cmd.push_back("WITHCOORD");
  }

  //! with_dist (optional)
  if (with_dist) {
    cmd.push_back("WITHDIST");
  }

  //! with_hash (optional)
  if (with_hash) {
    cmd.push_back("WITHHASH");
  }

  //! order (optional)
  cmd.push_back(asc_order ? "ASC" : "DESC");

  //! count (optional)
  if (count > 0) {
    cmd.push_back("COUNT");
    cmd.push_back(std::to_string(count));
  }

  //! store_key (optional)
  if (!store_key.empty()) {
    cmd.push_back("STOREDIST");
    cmd.push_back(storedist_key);
  }

  //! storedist_key (optional)
  if (!storedist_key.empty()) {
    cmd.push_back("STOREDIST");
    cmd.push_back(storedist_key);
  }

  send(cmd, reply_callback);
  return *this;
}

client&
client::get(const std::string& key, const reply_callback_t& reply_callback) {
  send({"GET", key}, reply_callback);
  return *this;
}

client&
client::getbit(const std::string& key, int offset, const reply_callback_t& reply_callback) {
  send({"GETBIT", key, std::to_string(offset)}, reply_callback);
  return *this;
}

client&
client::getrange(const std::string& key, int start, int end, const reply_callback_t& reply_callback) {
  send({"GETRANGE", key, std::to_string(start), std::to_string(end)}, reply_callback);
  return *this;
}

client&
client::getset(const std::string& key, const std::string& val, const reply_callback_t& reply_callback) {
  send({"GETSET", key, val}, reply_callback);
  return *this;
}

client&
client::hdel(const std::string& key, const std::vector<std::string>& fields, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"HDEL", key};
  cmd.insert(cmd.end(), fields.begin(), fields.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::hexists(const std::string& key, const std::string& field, const reply_callback_t& reply_callback) {
  send({"HEXISTS", key, field}, reply_callback);
  return *this;
}

client&
client::hget(const std::string& key, const std::string& field, const reply_callback_t& reply_callback) {
  send({"HGET", key, field}, reply_callback);
  return *this;
}

client&
client::hgetall(const std::string& key, const reply_callback_t& reply_callback) {
  send({"HGETALL", key}, reply_callback);
  return *this;
}

client&
client::hincrby(const std::string& key, const std::string& field, int incr, const reply_callback_t& reply_callback) {
  send({"HINCRBY", key, field, std::to_string(incr)}, reply_callback);
  return *this;
}

client&
client::hincrbyfloat(const std::string& key, const std::string& field, float incr, const reply_callback_t& reply_callback) {
  send({"HINCRBYFLOAT", key, field, std::to_string(incr)}, reply_callback);
  return *this;
}

client&
client::hkeys(const std::string& key, const reply_callback_t& reply_callback) {
  send({"HKEYS", key}, reply_callback);
  return *this;
}

client&
client::hlen(const std::string& key, const reply_callback_t& reply_callback) {
  send({"HLEN", key}, reply_callback);
  return *this;
}

client&
client::hmget(const std::string& key, const std::vector<std::string>& fields, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"HMGET", key};
  cmd.insert(cmd.end(), fields.begin(), fields.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::hmset(const std::string& key, const std::vector<std::pair<std::string, std::string>>& field_val, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"HMSET", key};
  for (const auto& obj : field_val) {
    cmd.push_back(obj.first);
    cmd.push_back(obj.second);
  }
  send(cmd, reply_callback);
  return *this;
}

client&
client::hscan(const std::string& key, std::size_t cursor, const reply_callback_t& reply_callback) {
  return hscan(key, cursor, "", 0, reply_callback);
}

client&
client::hscan(const std::string& key, std::size_t cursor, const std::string& pattern, const reply_callback_t& reply_callback) {
  return hscan(key, cursor, pattern, 0, reply_callback);
}

client&
client::hscan(const std::string& key, std::size_t cursor, std::size_t count, const reply_callback_t& reply_callback) {
  return hscan(key, cursor, "", count, reply_callback);
}

client&
client::hscan(const std::string& key, std::size_t cursor, const std::string& pattern, std::size_t count, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"HSCAN", key, std::to_string(cursor)};

  if (!pattern.empty()) {
    cmd.push_back("MATCH");
    cmd.push_back(pattern);
  }

  if (count > 0) {
    cmd.push_back("COUNT");
    cmd.push_back(std::to_string(count));
  }

  send(cmd, reply_callback);
  return *this;
}

client&
client::hset(const std::string& key, const std::string& field, const std::string& value, const reply_callback_t& reply_callback) {
  send({"HSET", key, field, value}, reply_callback);
  return *this;
}

client&
client::hsetnx(const std::string& key, const std::string& field, const std::string& value, const reply_callback_t& reply_callback) {
  send({"HSETNX", key, field, value}, reply_callback);
  return *this;
}

client&
client::hstrlen(const std::string& key, const std::string& field, const reply_callback_t& reply_callback) {
  send({"HSTRLEN", key, field}, reply_callback);
  return *this;
}

client&
client::hvals(const std::string& key, const reply_callback_t& reply_callback) {
  send({"HVALS", key}, reply_callback);
  return *this;
}

client&
client::incr(const std::string& key, const reply_callback_t& reply_callback) {
  send({"INCR", key}, reply_callback);
  return *this;
}

client&
client::incrby(const std::string& key, int incr, const reply_callback_t& reply_callback) {
  send({"INCRBY", key, std::to_string(incr)}, reply_callback);
  return *this;
}

client&
client::incrbyfloat(const std::string& key, float incr, const reply_callback_t& reply_callback) {
  send({"INCRBYFLOAT", key, std::to_string(incr)}, reply_callback);
  return *this;
}

client&
client::info(const reply_callback_t& reply_callback) {
  send({"INFO"}, reply_callback);
  return *this;
}

client&
client::info(const std::string& section, const reply_callback_t& reply_callback) {
  send({"INFO", section}, reply_callback);
  return *this;
}

client&
client::keys(const std::string& pattern, const reply_callback_t& reply_callback) {
  send({"KEYS", pattern}, reply_callback);
  return *this;
}

client&
client::lastsave(const reply_callback_t& reply_callback) {
  send({"LASTSAVE"}, reply_callback);
  return *this;
}

client&
client::lindex(const std::string& key, int index, const reply_callback_t& reply_callback) {
  send({"LINDEX", key, std::to_string(index)}, reply_callback);
  return *this;
}

client&
client::linsert(const std::string& key, const std::string& before_after, const std::string& pivot, const std::string& value, const reply_callback_t& reply_callback) {
  send({"LINSERT", key, before_after, pivot, value}, reply_callback);
  return *this;
}

client&
client::llen(const std::string& key, const reply_callback_t& reply_callback) {
  send({"LLEN", key}, reply_callback);
  return *this;
}

client&
client::lpop(const std::string& key, const reply_callback_t& reply_callback) {
  send({"LPOP", key}, reply_callback);
  return *this;
}

client&
client::lpush(const std::string& key, const std::vector<std::string>& values, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"LPUSH", key};
  cmd.insert(cmd.end(), values.begin(), values.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::lpushx(const std::string& key, const std::string& value, const reply_callback_t& reply_callback) {
  send({"LPUSHX", key, value}, reply_callback);
  return *this;
}

client&
client::lrange(const std::string& key, int start, int stop, const reply_callback_t& reply_callback) {
  send({"LRANGE", key, std::to_string(start), std::to_string(stop)}, reply_callback);
  return *this;
}

client&
client::lrem(const std::string& key, int count, const std::string& value, const reply_callback_t& reply_callback) {
  send({"LREM", key, std::to_string(count), value}, reply_callback);
  return *this;
}

client&
client::lset(const std::string& key, int index, const std::string& value, const reply_callback_t& reply_callback) {
  send({"LSET", key, std::to_string(index), value}, reply_callback);
  return *this;
}

client&
client::ltrim(const std::string& key, int start, int stop, const reply_callback_t& reply_callback) {
  send({"LTRIM", key, std::to_string(start), std::to_string(stop)}, reply_callback);
  return *this;
}

client&
client::mget(const std::vector<std::string>& keys, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"MGET"};
  cmd.insert(cmd.end(), keys.begin(), keys.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::migrate(const std::string& host, int port, const std::string& key, const std::string& dest_db, int timeout, const reply_callback_t& reply_callback) {
  send({"MIGRATE", host, std::to_string(port), key, dest_db, std::to_string(timeout)}, reply_callback);
  return *this;
}

client&
client::migrate(const std::string& host, int port, const std::string& key, const std::string& dest_db, int timeout, bool copy, bool replace, const std::vector<std::string>& keys, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"MIGRATE", host, std::to_string(port), key, dest_db, std::to_string(timeout)};
  if (copy) { cmd.push_back("COPY"); }
  if (replace) { cmd.push_back("REPLACE"); }
  if (keys.size()) {
    cmd.push_back("KEYS");
    cmd.insert(cmd.end(), keys.begin(), keys.end());
  }
  send(cmd, reply_callback);
  return *this;
}

client&
client::monitor(const reply_callback_t& reply_callback) {
  send({"MONITOR"}, reply_callback);
  return *this;
}

client&
client::move(const std::string& key, const std::string& db, const reply_callback_t& reply_callback) {
  send({"MOVE", key, db}, reply_callback);
  return *this;
}

client&
client::mset(const std::vector<std::pair<std::string, std::string>>& key_vals, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"MSET"};
  for (const auto& obj : key_vals) {
    cmd.push_back(obj.first);
    cmd.push_back(obj.second);
  }
  send(cmd, reply_callback);
  return *this;
}

client&
client::msetnx(const std::vector<std::pair<std::string, std::string>>& key_vals, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"MSETNX"};
  for (const auto& obj : key_vals) {
    cmd.push_back(obj.first);
    cmd.push_back(obj.second);
  }
  send(cmd, reply_callback);
  return *this;
}

client&
client::multi(const reply_callback_t& reply_callback) {
  send({"MULTI"}, reply_callback);
  return *this;
}

client&
client::object(const std::string& subcommand, const std::vector<std::string>& args, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"OBJECT", subcommand};
  cmd.insert(cmd.end(), args.begin(), args.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::persist(const std::string& key, const reply_callback_t& reply_callback) {
  send({"PERSIST", key}, reply_callback);
  return *this;
}

client&
client::pexpire(const std::string& key, int milliseconds, const reply_callback_t& reply_callback) {
  send({"PEXPIRE", key, std::to_string(milliseconds)}, reply_callback);
  return *this;
}

client&
client::pexpireat(const std::string& key, int milliseconds_timestamp, const reply_callback_t& reply_callback) {
  send({"PEXPIREAT", key, std::to_string(milliseconds_timestamp)}, reply_callback);
  return *this;
}

client&
client::pfadd(const std::string& key, const std::vector<std::string>& elements, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"PFADD", key};
  cmd.insert(cmd.end(), elements.begin(), elements.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::pfcount(const std::vector<std::string>& keys, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"PFCOUNT"};
  cmd.insert(cmd.end(), keys.begin(), keys.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::pfmerge(const std::string& destkey, const std::vector<std::string>& sourcekeys, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"PFMERGE", destkey};
  cmd.insert(cmd.end(), sourcekeys.begin(), sourcekeys.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::ping(const reply_callback_t& reply_callback) {
  send({"PING"}, reply_callback);
  return *this;
}

client&
client::ping(const std::string& message, const reply_callback_t& reply_callback) {
  send({"PING", message}, reply_callback);
  return *this;
}

client&
client::psetex(const std::string& key, int milliseconds, const std::string& val, const reply_callback_t& reply_callback) {
  send({"PSETEX", key, std::to_string(milliseconds), val}, reply_callback);
  return *this;
}

client&
client::publish(const std::string& channel, const std::string& message, const reply_callback_t& reply_callback) {
  send({"PUBLISH", channel, message}, reply_callback);
  return *this;
}

client&
client::pubsub(const std::string& subcommand, const std::vector<std::string>& args, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"PUBSUB", subcommand};
  cmd.insert(cmd.end(), args.begin(), args.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::pttl(const std::string& key, const reply_callback_t& reply_callback) {
  send({"PTTL", key}, reply_callback);
  return *this;
}

client&
client::quit(const reply_callback_t& reply_callback) {
  send({"QUIT"}, reply_callback);
  return *this;
}

client&
client::randomkey(const reply_callback_t& reply_callback) {
  send({"RANDOMKEY"}, reply_callback);
  return *this;
}

client&
client::readonly(const reply_callback_t& reply_callback) {
  send({"READONLY"}, reply_callback);
  return *this;
}

client&
client::readwrite(const reply_callback_t& reply_callback) {
  send({"READWRITE"}, reply_callback);
  return *this;
}

client&
client::rename(const std::string& key, const std::string& newkey, const reply_callback_t& reply_callback) {
  send({"RENAME", key, newkey}, reply_callback);
  return *this;
}

client&
client::renamenx(const std::string& key, const std::string& newkey, const reply_callback_t& reply_callback) {
  send({"RENAMENX", key, newkey}, reply_callback);
  return *this;
}

client&
client::restore(const std::string& key, int ttl, const std::string& serialized_value, const reply_callback_t& reply_callback) {
  send({"RESTORE", key, std::to_string(ttl), serialized_value}, reply_callback);
  return *this;
}

client&
client::restore(const std::string& key, int ttl, const std::string& serialized_value, const std::string& replace, const reply_callback_t& reply_callback) {
  send({"RESTORE", key, std::to_string(ttl), serialized_value, replace}, reply_callback);
  return *this;
}

client&
client::role(const reply_callback_t& reply_callback) {
  send({"ROLE"}, reply_callback);
  return *this;
}

client&
client::rpop(const std::string& key, const reply_callback_t& reply_callback) {
  send({"RPOP", key}, reply_callback);
  return *this;
}

client&
client::rpoplpush(const std::string& source, const std::string& destination, const reply_callback_t& reply_callback) {
  send({"RPOPLPUSH", source, destination}, reply_callback);
  return *this;
}

client&
client::rpush(const std::string& key, const std::vector<std::string>& values, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"RPUSH", key};
  cmd.insert(cmd.end(), values.begin(), values.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::rpushx(const std::string& key, const std::string& value, const reply_callback_t& reply_callback) {
  send({"RPUSHX", key, value}, reply_callback);
  return *this;
}

client&
client::sadd(const std::string& key, const std::vector<std::string>& members, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"SADD", key};
  cmd.insert(cmd.end(), members.begin(), members.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::save(const reply_callback_t& reply_callback) {
  send({"SAVE"}, reply_callback);
  return *this;
}

client&
client::scan(std::size_t cursor, const reply_callback_t& reply_callback) {
  return scan(cursor, "", 0, reply_callback);
}

client&
client::scan(std::size_t cursor, const std::string& pattern, const reply_callback_t& reply_callback) {
  return scan(cursor, pattern, 0, reply_callback);
}

client&
client::scan(std::size_t cursor, std::size_t count, const reply_callback_t& reply_callback) {
  return scan(cursor, "", count, reply_callback);
}

client&
client::scan(std::size_t cursor, const std::string& pattern, std::size_t count, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"SCAN", std::to_string(cursor)};

  if (!pattern.empty()) {
    cmd.push_back("MATCH");
    cmd.push_back(pattern);
  }

  if (count > 0) {
    cmd.push_back("COUNT");
    cmd.push_back(std::to_string(count));
  }

  send(cmd, reply_callback);
  return *this;
}

client&
client::scard(const std::string& key, const reply_callback_t& reply_callback) {
  send({"SCARD", key}, reply_callback);
  return *this;
}

client&
client::script_debug(const std::string& mode, const reply_callback_t& reply_callback) {
  send({"SCRIPT", "DEBUG", mode}, reply_callback);
  return *this;
}

client&
client::script_exists(const std::vector<std::string>& scripts, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"SCRIPT", "EXISTS"};
  cmd.insert(cmd.end(), scripts.begin(), scripts.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::script_flush(const reply_callback_t& reply_callback) {
  send({"SCRIPT", "FLUSH"}, reply_callback);
  return *this;
}

client&
client::script_kill(const reply_callback_t& reply_callback) {
  send({"SCRIPT", "KILL"}, reply_callback);
  return *this;
}

client&
client::script_load(const std::string& script, const reply_callback_t& reply_callback) {
  send({"SCRIPT", "LOAD", script}, reply_callback);
  return *this;
}

client&
client::sdiff(const std::vector<std::string>& keys, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"SDIFF"};
  cmd.insert(cmd.end(), keys.begin(), keys.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::sdiffstore(const std::string& destination, const std::vector<std::string>& keys, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"SDIFFSTORE", destination};
  cmd.insert(cmd.end(), keys.begin(), keys.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::select(int index, const reply_callback_t& reply_callback) {
  std::lock_guard<std::mutex> lock(m_callbacks_mutex);

  unprotected_select(index, reply_callback);

  return *this;
}

void
client::unprotected_select(int index, const reply_callback_t& reply_callback) {
  //! save the index of the database for reconnect attempts.
  m_database_index = index;
  //! save command in the pipeline
  unprotected_send({"SELECT", std::to_string(index)}, reply_callback);
}

client&
client::set(const std::string& key, const std::string& value, const reply_callback_t& reply_callback) {
  send({"SET", key, value}, reply_callback);
  return *this;
}

client&
client::set_advanced(const std::string& key, const std::string& value, const reply_callback_t& reply_callback) {
  send({"SET", key, value}, reply_callback);
  return *this;
}

client&
client::set_advanced(const std::string& key, const std::string& value, bool ex, int ex_sec, bool px, int px_milli, bool nx, bool xx, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"SET", key, value};
  if (ex) {
    cmd.push_back("EX");
    cmd.push_back(std::to_string(ex_sec));
  }
  if (px) {
    cmd.push_back("PX");
    cmd.push_back(std::to_string(px_milli));
  }
  if (nx) { cmd.push_back("NX"); }
  if (xx) { cmd.push_back("XX"); }
  send(cmd, reply_callback);
  return *this;
}

client&
client::setbit_(const std::string& key, int offset, const std::string& value, const reply_callback_t& reply_callback) {
  send({"SETBIT", key, std::to_string(offset), value}, reply_callback);
  return *this;
}

client&
client::setex(const std::string& key, int seconds, const std::string& value, const reply_callback_t& reply_callback) {
  send({"SETEX", key, std::to_string(seconds), value}, reply_callback);
  return *this;
}

client&
client::setnx(const std::string& key, const std::string& value, const reply_callback_t& reply_callback) {
  send({"SETNX", key, value}, reply_callback);
  return *this;
}

client&
client::setrange(const std::string& key, int offset, const std::string& value, const reply_callback_t& reply_callback) {
  send({"SETRANGE", key, std::to_string(offset), value}, reply_callback);
  return *this;
}

client&
client::shutdown(const reply_callback_t& reply_callback) {
  send({"SHUTDOWN"}, reply_callback);
  return *this;
}

client&
client::shutdown(const std::string& save, const reply_callback_t& reply_callback) {
  send({"SHUTDOWN", save}, reply_callback);
  return *this;
}

client&
client::sinter(const std::vector<std::string>& keys, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"SINTER"};
  cmd.insert(cmd.end(), keys.begin(), keys.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::sinterstore(const std::string& destination, const std::vector<std::string>& keys, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"SINTERSTORE", destination};
  cmd.insert(cmd.end(), keys.begin(), keys.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::sismember(const std::string& key, const std::string& member, const reply_callback_t& reply_callback) {
  send({"SISMEMBER", key, member}, reply_callback);
  return *this;
}

client&
client::slaveof(const std::string& host, int port, const reply_callback_t& reply_callback) {
  send({"SLAVEOF", host, std::to_string(port)}, reply_callback);
  return *this;
}

client&
client::slowlog(const std::string subcommand, const reply_callback_t& reply_callback) {
  send({"SLOWLOG", subcommand}, reply_callback);
  return *this;
}

client&
client::slowlog(const std::string subcommand, const std::string& argument, const reply_callback_t& reply_callback) {
  send({"SLOWLOG", subcommand, argument}, reply_callback);
  return *this;
}

client&
client::smembers(const std::string& key, const reply_callback_t& reply_callback) {
  send({"SMEMBERS", key}, reply_callback);
  return *this;
}

client&
client::smove(const std::string& source, const std::string& destination, const std::string& member, const reply_callback_t& reply_callback) {
  send({"SMOVE", source, destination, member}, reply_callback);
  return *this;
}


client&
client::sort(const std::string& key, const reply_callback_t& reply_callback) {
  send({"SORT", key}, reply_callback);
  return *this;
}

client&
client::sort(const std::string& key, const std::vector<std::string>& get_patterns, bool asc_order, bool alpha, const reply_callback_t& reply_callback) {
  return sort(key, "", false, 0, 0, get_patterns, asc_order, alpha, "", reply_callback);
}

client&
client::sort(const std::string& key, std::size_t offset, std::size_t count, const std::vector<std::string>& get_patterns, bool asc_order, bool alpha, const reply_callback_t& reply_callback) {
  return sort(key, "", true, offset, count, get_patterns, asc_order, alpha, "", reply_callback);
}

client&
client::sort(const std::string& key, const std::string& by_pattern, const std::vector<std::string>& get_patterns, bool asc_order, bool alpha, const reply_callback_t& reply_callback) {
  return sort(key, by_pattern, false, 0, 0, get_patterns, asc_order, alpha, "", reply_callback);
}

client&
client::sort(const std::string& key, const std::vector<std::string>& get_patterns, bool asc_order, bool alpha, const std::string& store_dest, const reply_callback_t& reply_callback) {
  return sort(key, "", false, 0, 0, get_patterns, asc_order, alpha, store_dest, reply_callback);
}

client&
client::sort(const std::string& key, std::size_t offset, std::size_t count, const std::vector<std::string>& get_patterns, bool asc_order, bool alpha, const std::string& store_dest, const reply_callback_t& reply_callback) {
  return sort(key, "", true, offset, count, get_patterns, asc_order, alpha, store_dest, reply_callback);
}

client&
client::sort(const std::string& key, const std::string& by_pattern, const std::vector<std::string>& get_patterns, bool asc_order, bool alpha, const std::string& store_dest, const reply_callback_t& reply_callback) {
  return sort(key, by_pattern, false, 0, 0, get_patterns, asc_order, alpha, store_dest, reply_callback);
}

client&
client::sort(const std::string& key, const std::string& by_pattern, std::size_t offset, std::size_t count, const std::vector<std::string>& get_patterns, bool asc_order, bool alpha, const reply_callback_t& reply_callback) {
  return sort(key, by_pattern, true, offset, count, get_patterns, asc_order, alpha, "", reply_callback);
}

client&
client::sort(const std::string& key, const std::string& by_pattern, std::size_t offset, std::size_t count, const std::vector<std::string>& get_patterns, bool asc_order, bool alpha, const std::string& store_dest, const reply_callback_t& reply_callback) {
  return sort(key, by_pattern, true, offset, count, get_patterns, asc_order, alpha, store_dest, reply_callback);
}

client&
client::sort(const std::string& key, const std::string& by_pattern, bool limit, std::size_t offset, std::size_t count, const std::vector<std::string>& get_patterns, bool asc_order, bool alpha, const std::string& store_dest, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"SORT", key};

  //! add by pattern (optional)
  if (!by_pattern.empty()) {
    cmd.push_back("BY");
    cmd.push_back(by_pattern);
  }

  //! add limit (optional)
  if (limit) {
    cmd.push_back("LIMIT");
    cmd.push_back(std::to_string(offset));
    cmd.push_back(std::to_string(count));
  }

  //! add get pattern (optional)
  for (const auto& get_pattern : get_patterns) {
    if (get_pattern.empty()) {
      continue;
    }

    cmd.push_back("GET");
    cmd.push_back(get_pattern);
  }

  //! add order by (optional)
  cmd.push_back(asc_order ? "ASC" : "DESC");

  //! add alpha (optional)
  if (alpha) {
    cmd.push_back("ALPHA");
  }

  //! add store dest (optional)
  if (!store_dest.empty()) {
    cmd.push_back("STORE");
    cmd.push_back(store_dest);
  }

  send(cmd, reply_callback);
  return *this;
}

client&
client::spop(const std::string& key, const reply_callback_t& reply_callback) {
  send({"SPOP", key}, reply_callback);
  return *this;
}

client&
client::spop(const std::string& key, int count, const reply_callback_t& reply_callback) {
  send({"SPOP", key, std::to_string(count)}, reply_callback);
  return *this;
}

client&
client::srandmember(const std::string& key, const reply_callback_t& reply_callback) {
  send({"SRANDMEMBER", key}, reply_callback);
  return *this;
}

client&
client::srandmember(const std::string& key, int count, const reply_callback_t& reply_callback) {
  send({"SRANDMEMBER", key, std::to_string(count)}, reply_callback);
  return *this;
}

client&
client::srem(const std::string& key, const std::vector<std::string>& members, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"SREM", key};
  cmd.insert(cmd.end(), members.begin(), members.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::sscan(const std::string& key, std::size_t cursor, const reply_callback_t& reply_callback) {
  return sscan(key, cursor, "", 0, reply_callback);
}

client&
client::sscan(const std::string& key, std::size_t cursor, const std::string& pattern, const reply_callback_t& reply_callback) {
  return sscan(key, cursor, pattern, 0, reply_callback);
}

client&
client::sscan(const std::string& key, std::size_t cursor, std::size_t count, const reply_callback_t& reply_callback) {
  return sscan(key, cursor, "", count, reply_callback);
}

client&
client::sscan(const std::string& key, std::size_t cursor, const std::string& pattern, std::size_t count, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"SSCAN", key, std::to_string(cursor)};

  if (!pattern.empty()) {
    cmd.push_back("MATCH");
    cmd.push_back(pattern);
  }

  if (count > 0) {
    cmd.push_back("COUNT");
    cmd.push_back(std::to_string(count));
  }

  send(cmd, reply_callback);
  return *this;
}

client&
client::strlen(const std::string& key, const reply_callback_t& reply_callback) {
  send({"STRLEN", key}, reply_callback);
  return *this;
}

client&
client::sunion(const std::vector<std::string>& keys, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"SUNION"};
  cmd.insert(cmd.end(), keys.begin(), keys.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::sunionstore(const std::string& destination, const std::vector<std::string>& keys, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"SUNIONSTORE", destination};
  cmd.insert(cmd.end(), keys.begin(), keys.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::sync(const reply_callback_t& reply_callback) {
  send({"SYNC"}, reply_callback);
  return *this;
}

client&
client::time(const reply_callback_t& reply_callback) {
  send({"TIME"}, reply_callback);
  return *this;
}

client&
client::ttl(const std::string& key, const reply_callback_t& reply_callback) {
  send({"TTL", key}, reply_callback);
  return *this;
}

client&
client::type(const std::string& key, const reply_callback_t& reply_callback) {
  send({"TYPE", key}, reply_callback);
  return *this;
}

client&
client::unwatch(const reply_callback_t& reply_callback) {
  send({"UNWATCH"}, reply_callback);
  return *this;
}

client&
client::wait(int numslaves, int timeout, const reply_callback_t& reply_callback) {
  send({"WAIT", std::to_string(numslaves), std::to_string(timeout)}, reply_callback);
  return *this;
}

client&
client::watch(const std::vector<std::string>& keys, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"WATCH"};
  cmd.insert(cmd.end(), keys.begin(), keys.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::zadd(const std::string& key, const std::vector<std::string>& options, const std::multimap<std::string, std::string>& score_members, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"ZADD", key};

  //! options
  cmd.insert(cmd.end(), options.begin(), options.end());

  //! score members
  for (auto& sm : score_members) {
    cmd.push_back(sm.first);
    cmd.push_back(sm.second);
  }

  send(cmd, reply_callback);
  return *this;
}

client&
client::zcard(const std::string& key, const reply_callback_t& reply_callback) {
  send({"ZCARD", key}, reply_callback);
  return *this;
}

client&
client::zcount(const std::string& key, int min, int max, const reply_callback_t& reply_callback) {
  send({"ZCOUNT", key, std::to_string(min), std::to_string(max)}, reply_callback);
  return *this;
}

client&
client::zcount(const std::string& key, double min, double max, const reply_callback_t& reply_callback) {
  send({"ZCOUNT", key, std::to_string(min), std::to_string(max)}, reply_callback);
  return *this;
}

client&
client::zcount(const std::string& key, const std::string& min, const std::string& max, const reply_callback_t& reply_callback) {
  send({"ZCOUNT", key, min, max}, reply_callback);
  return *this;
}

client&
client::zincrby(const std::string& key, int incr, const std::string& member, const reply_callback_t& reply_callback) {
  send({"ZINCRBY", key, std::to_string(incr), member}, reply_callback);
  return *this;
}

client&
client::zincrby(const std::string& key, double incr, const std::string& member, const reply_callback_t& reply_callback) {
  send({"ZINCRBY", key, std::to_string(incr), member}, reply_callback);
  return *this;
}

client&
client::zincrby(const std::string& key, const std::string& incr, const std::string& member, const reply_callback_t& reply_callback) {
  send({"ZINCRBY", key, incr, member}, reply_callback);
  return *this;
}

client&
client::zinterstore(const std::string& destination, std::size_t numkeys, const std::vector<std::string>& keys, const std::vector<std::size_t> weights, aggregate_method method, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"ZINTERSTORE", destination, std::to_string(numkeys)};

  //! keys
  for (const auto& key : keys) {
    cmd.push_back(key);
  }

  //! weights (optional)
  if (!weights.empty()) {
    cmd.push_back("WEIGHTS");

    for (auto weight : weights) {
      cmd.push_back(std::to_string(weight));
    }
  }

  //! aggregate method
  if (method != aggregate_method::server_default) {
    cmd.push_back("AGGREGATE");
    cmd.push_back(aggregate_method_to_string(method));
  }

  send(cmd, reply_callback);
  return *this;
}

client&
client::zlexcount(const std::string& key, int min, int max, const reply_callback_t& reply_callback) {
  send({"ZLEXCOUNT", key, std::to_string(min), std::to_string(max)}, reply_callback);
  return *this;
}

client&
client::zlexcount(const std::string& key, double min, double max, const reply_callback_t& reply_callback) {
  send({"ZLEXCOUNT", key, std::to_string(min), std::to_string(max)}, reply_callback);
  return *this;
}

client&
client::zlexcount(const std::string& key, const std::string& min, const std::string& max, const reply_callback_t& reply_callback) {
  send({"ZLEXCOUNT", key, min, max}, reply_callback);
  return *this;
}

client&
client::zrange(const std::string& key, int start, int stop, const reply_callback_t& reply_callback) {
  send({"ZRANGE", key, std::to_string(start), std::to_string(stop)}, reply_callback);
  return *this;
}

client&
client::zrange(const std::string& key, int start, int stop, bool withscores, const reply_callback_t& reply_callback) {
  if (withscores)
    send({"ZRANGE", key, std::to_string(start), std::to_string(stop), "WITHSCORES"}, reply_callback);
  else
    send({"ZRANGE", key, std::to_string(start), std::to_string(stop)}, reply_callback);
  return *this;
}

client&
client::zrange(const std::string& key, double start, double stop, const reply_callback_t& reply_callback) {
  send({"ZRANGE", key, std::to_string(start), std::to_string(stop)}, reply_callback);
  return *this;
}

client&
client::zrange(const std::string& key, double start, double stop, bool withscores, const reply_callback_t& reply_callback) {
  if (withscores)
    send({"ZRANGE", key, std::to_string(start), std::to_string(stop), "WITHSCORES"}, reply_callback);
  else
    send({"ZRANGE", key, std::to_string(start), std::to_string(stop)}, reply_callback);
  return *this;
}

client&
client::zrange(const std::string& key, const std::string& start, const std::string& stop, const reply_callback_t& reply_callback) {
  send({"ZRANGE", key, start, stop}, reply_callback);
  return *this;
}

client&
client::zrange(const std::string& key, const std::string& start, const std::string& stop, bool withscores, const reply_callback_t& reply_callback) {
  if (withscores)
    send({"ZRANGE", key, start, stop, "WITHSCORES"}, reply_callback);
  else
    send({"ZRANGE", key, start, stop}, reply_callback);
  return *this;
}

client&
client::zrangebylex(const std::string& key, int min, int max, const reply_callback_t& reply_callback) {
  return zrangebylex(key, std::to_string(min), std::to_string(max), false, 0, 0, false, reply_callback);
}

client&
client::zrangebylex(const std::string& key, int min, int max, bool withscores, const reply_callback_t& reply_callback) {
  return zrangebylex(key, std::to_string(min), std::to_string(max), false, 0, 0, withscores, reply_callback);
}

client&
client::zrangebylex(const std::string& key, double min, double max, const reply_callback_t& reply_callback) {
  return zrangebylex(key, std::to_string(min), std::to_string(max), false, 0, 0, false, reply_callback);
}

client&
client::zrangebylex(const std::string& key, double min, double max, bool withscores, const reply_callback_t& reply_callback) {
  return zrangebylex(key, std::to_string(min), std::to_string(max), false, 0, 0, withscores, reply_callback);
}

client&
client::zrangebylex(const std::string& key, const std::string& min, const std::string& max, const reply_callback_t& reply_callback) {
  return zrangebylex(key, min, max, false, 0, 0, false, reply_callback);
}

client&
client::zrangebylex(const std::string& key, const std::string& min, const std::string& max, bool withscores, const reply_callback_t& reply_callback) {
  return zrangebylex(key, min, max, false, 0, 0, withscores, reply_callback);
}

client&
client::zrangebylex(const std::string& key, int min, int max, std::size_t offset, std::size_t count, const reply_callback_t& reply_callback) {
  return zrangebylex(key, std::to_string(min), std::to_string(max), true, offset, count, false, reply_callback);
}

client&
client::zrangebylex(const std::string& key, int min, int max, std::size_t offset, std::size_t count, bool withscores, const reply_callback_t& reply_callback) {
  return zrangebylex(key, std::to_string(min), std::to_string(max), true, offset, count, withscores, reply_callback);
}

client&
client::zrangebylex(const std::string& key, double min, double max, std::size_t offset, std::size_t count, const reply_callback_t& reply_callback) {
  return zrangebylex(key, std::to_string(min), std::to_string(max), true, offset, count, false, reply_callback);
}

client&
client::zrangebylex(const std::string& key, double min, double max, std::size_t offset, std::size_t count, bool withscores, const reply_callback_t& reply_callback) {
  return zrangebylex(key, std::to_string(min), std::to_string(max), true, offset, count, withscores, reply_callback);
}

client&
client::zrangebylex(const std::string& key, const std::string& min, const std::string& max, std::size_t offset, std::size_t count, const reply_callback_t& reply_callback) {
  return zrangebylex(key, min, max, true, offset, count, false, reply_callback);
}

client&
client::zrangebylex(const std::string& key, const std::string& min, const std::string& max, std::size_t offset, std::size_t count, bool withscores, const reply_callback_t& reply_callback) {
  return zrangebylex(key, min, max, true, offset, count, withscores, reply_callback);
}

client&
client::zrangebylex(const std::string& key, const std::string& min, const std::string& max, bool limit, std::size_t offset, std::size_t count, bool withscores, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"ZRANGEBYLEX", key, min, max};

  //! withscores (optional)
  if (withscores) {
    cmd.push_back("WITHSCORES");
  }

  //! limit (optional)
  if (limit) {
    cmd.push_back("LIMIT");
    cmd.push_back(std::to_string(offset));
    cmd.push_back(std::to_string(count));
  }

  send(cmd, reply_callback);
  return *this;
}

client&
client::zrangebyscore(const std::string& key, int min, int max, const reply_callback_t& reply_callback) {
  return zrangebyscore(key, std::to_string(min), std::to_string(max), false, 0, 0, false, reply_callback);
}

client&
client::zrangebyscore(const std::string& key, int min, int max, bool withscores, const reply_callback_t& reply_callback) {
  return zrangebyscore(key, std::to_string(min), std::to_string(max), false, 0, 0, withscores, reply_callback);
}

client&
client::zrangebyscore(const std::string& key, double min, double max, const reply_callback_t& reply_callback) {
  return zrangebyscore(key, std::to_string(min), std::to_string(max), false, 0, 0, false, reply_callback);
}

client&
client::zrangebyscore(const std::string& key, double min, double max, bool withscores, const reply_callback_t& reply_callback) {
  return zrangebyscore(key, std::to_string(min), std::to_string(max), false, 0, 0, withscores, reply_callback);
}

client&
client::zrangebyscore(const std::string& key, const std::string& min, const std::string& max, const reply_callback_t& reply_callback) {
  return zrangebyscore(key, min, max, false, 0, 0, false, reply_callback);
}

client&
client::zrangebyscore(const std::string& key, const std::string& min, const std::string& max, bool withscores, const reply_callback_t& reply_callback) {
  return zrangebyscore(key, min, max, false, 0, 0, withscores, reply_callback);
}

client&
client::zrangebyscore(const std::string& key, int min, int max, std::size_t offset, std::size_t count, const reply_callback_t& reply_callback) {
  return zrangebyscore(key, std::to_string(min), std::to_string(max), true, offset, count, false, reply_callback);
}

client&
client::zrangebyscore(const std::string& key, int min, int max, std::size_t offset, std::size_t count, bool withscores, const reply_callback_t& reply_callback) {
  return zrangebyscore(key, std::to_string(min), std::to_string(max), true, offset, count, withscores, reply_callback);
}

client&
client::zrangebyscore(const std::string& key, double min, double max, std::size_t offset, std::size_t count, const reply_callback_t& reply_callback) {
  return zrangebyscore(key, std::to_string(min), std::to_string(max), true, offset, count, false, reply_callback);
}

client&
client::zrangebyscore(const std::string& key, double min, double max, std::size_t offset, std::size_t count, bool withscores, const reply_callback_t& reply_callback) {
  return zrangebyscore(key, std::to_string(min), std::to_string(max), true, offset, count, withscores, reply_callback);
}

client&
client::zrangebyscore(const std::string& key, const std::string& min, const std::string& max, std::size_t offset, std::size_t count, const reply_callback_t& reply_callback) {
  return zrangebyscore(key, min, max, true, offset, count, false, reply_callback);
}

client&
client::zrangebyscore(const std::string& key, const std::string& min, const std::string& max, std::size_t offset, std::size_t count, bool withscores, const reply_callback_t& reply_callback) {
  return zrangebyscore(key, min, max, true, offset, count, withscores, reply_callback);
}

client&
client::zrangebyscore(const std::string& key, const std::string& min, const std::string& max, bool limit, std::size_t offset, std::size_t count, bool withscores, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"ZRANGEBYSCORE", key, min, max};

  //! withscores (optional)
  if (withscores) {
    cmd.push_back("WITHSCORES");
  }

  //! limit (optional)
  if (limit) {
    cmd.push_back("LIMIT");
    cmd.push_back(std::to_string(offset));
    cmd.push_back(std::to_string(count));
  }

  send(cmd, reply_callback);
  return *this;
}

client&
client::zrank(const std::string& key, const std::string& member, const reply_callback_t& reply_callback) {
  send({"ZRANK", key, member}, reply_callback);
  return *this;
}

client&
client::zrem(const std::string& key, const std::vector<std::string>& members, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"ZREM", key};
  cmd.insert(cmd.end(), members.begin(), members.end());
  send(cmd, reply_callback);
  return *this;
}

client&
client::zremrangebylex(const std::string& key, int min, int max, const reply_callback_t& reply_callback) {
  send({"ZREMRANGEBYLEX", key, std::to_string(min), std::to_string(max)}, reply_callback);
  return *this;
}

client&
client::zremrangebylex(const std::string& key, double min, double max, const reply_callback_t& reply_callback) {
  send({"ZREMRANGEBYLEX", key, std::to_string(min), std::to_string(max)}, reply_callback);
  return *this;
}

client&
client::zremrangebylex(const std::string& key, const std::string& min, const std::string& max, const reply_callback_t& reply_callback) {
  send({"ZREMRANGEBYLEX", key, min, max}, reply_callback);
  return *this;
}

client&
client::zremrangebyrank(const std::string& key, int start, int stop, const reply_callback_t& reply_callback) {
  send({"ZREMRANGEBYRANK", key, std::to_string(start), std::to_string(stop)}, reply_callback);
  return *this;
}

client&
client::zremrangebyrank(const std::string& key, double start, double stop, const reply_callback_t& reply_callback) {
  send({"ZREMRANGEBYRANK", key, std::to_string(start), std::to_string(stop)}, reply_callback);
  return *this;
}

client&
client::zremrangebyrank(const std::string& key, const std::string& start, const std::string& stop, const reply_callback_t& reply_callback) {
  send({"ZREMRANGEBYRANK", key, start, stop}, reply_callback);
  return *this;
}

client&
client::zremrangebyscore(const std::string& key, int min, int max, const reply_callback_t& reply_callback) {
  send({"ZREMRANGEBYSCORE", key, std::to_string(min), std::to_string(max)}, reply_callback);
  return *this;
}

client&
client::zremrangebyscore(const std::string& key, double min, double max, const reply_callback_t& reply_callback) {
  send({"ZREMRANGEBYSCORE", key, std::to_string(min), std::to_string(max)}, reply_callback);
  return *this;
}

client&
client::zremrangebyscore(const std::string& key, const std::string& min, const std::string& max, const reply_callback_t& reply_callback) {
  send({"ZREMRANGEBYSCORE", key, min, max}, reply_callback);
  return *this;
}

client&
client::zrevrange(const std::string& key, int start, int stop, const reply_callback_t& reply_callback) {
  send({"ZREVRANGE", key, std::to_string(start), std::to_string(stop)}, reply_callback);
  return *this;
}

client&
client::zrevrange(const std::string& key, int start, int stop, bool withscores, const reply_callback_t& reply_callback) {
  if (withscores)
    send({"ZREVRANGE", key, std::to_string(start), std::to_string(stop), "WITHSCORES"}, reply_callback);
  else
    send({"ZREVRANGE", key, std::to_string(start), std::to_string(stop)}, reply_callback);
  return *this;
}

client&
client::zrevrange(const std::string& key, double start, double stop, const reply_callback_t& reply_callback) {
  send({"ZREVRANGE", key, std::to_string(start), std::to_string(stop)}, reply_callback);
  return *this;
}

client&
client::zrevrange(const std::string& key, double start, double stop, bool withscores, const reply_callback_t& reply_callback) {
  if (withscores)
    send({"ZREVRANGE", key, std::to_string(start), std::to_string(stop), "WITHSCORES"}, reply_callback);
  else
    send({"ZREVRANGE", key, std::to_string(start), std::to_string(stop)}, reply_callback);
  return *this;
}

client&
client::zrevrange(const std::string& key, const std::string& start, const std::string& stop, const reply_callback_t& reply_callback) {
  send({"ZREVRANGE", key, start, stop}, reply_callback);
  return *this;
}

client&
client::zrevrange(const std::string& key, const std::string& start, const std::string& stop, bool withscores, const reply_callback_t& reply_callback) {
  if (withscores)
    send({"ZREVRANGE", key, start, stop, "WITHSCORES"}, reply_callback);
  else
    send({"ZREVRANGE", key, start, stop}, reply_callback);
  return *this;
}

client&
client::zrevrangebylex(const std::string& key, int max, int min, const reply_callback_t& reply_callback) {
  return zrevrangebylex(key, std::to_string(max), std::to_string(min), false, 0, 0, false, reply_callback);
}

client&
client::zrevrangebylex(const std::string& key, int max, int min, bool withscores, const reply_callback_t& reply_callback) {
  return zrevrangebylex(key, std::to_string(max), std::to_string(min), false, 0, 0, withscores, reply_callback);
}

client&
client::zrevrangebylex(const std::string& key, double max, double min, const reply_callback_t& reply_callback) {
  return zrevrangebylex(key, std::to_string(max), std::to_string(min), false, 0, 0, false, reply_callback);
}

client&
client::zrevrangebylex(const std::string& key, double max, double min, bool withscores, const reply_callback_t& reply_callback) {
  return zrevrangebylex(key, std::to_string(max), std::to_string(min), false, 0, 0, withscores, reply_callback);
}

client&
client::zrevrangebylex(const std::string& key, const std::string& max, const std::string& min, const reply_callback_t& reply_callback) {
  return zrevrangebylex(key, max, min, false, 0, 0, false, reply_callback);
}

client&
client::zrevrangebylex(const std::string& key, const std::string& max, const std::string& min, bool withscores, const reply_callback_t& reply_callback) {
  return zrevrangebylex(key, max, min, false, 0, 0, withscores, reply_callback);
}

client&
client::zrevrangebylex(const std::string& key, int max, int min, std::size_t offset, std::size_t count, const reply_callback_t& reply_callback) {
  return zrevrangebylex(key, std::to_string(max), std::to_string(min), true, offset, count, false, reply_callback);
}

client&
client::zrevrangebylex(const std::string& key, int max, int min, std::size_t offset, std::size_t count, bool withscores, const reply_callback_t& reply_callback) {
  return zrevrangebylex(key, std::to_string(max), std::to_string(min), true, offset, count, withscores, reply_callback);
}

client&
client::zrevrangebylex(const std::string& key, double max, double min, std::size_t offset, std::size_t count, const reply_callback_t& reply_callback) {
  return zrevrangebylex(key, std::to_string(max), std::to_string(min), true, offset, count, false, reply_callback);
}

client&
client::zrevrangebylex(const std::string& key, double max, double min, std::size_t offset, std::size_t count, bool withscores, const reply_callback_t& reply_callback) {
  return zrevrangebylex(key, std::to_string(max), std::to_string(min), true, offset, count, withscores, reply_callback);
}

client&
client::zrevrangebylex(const std::string& key, const std::string& max, const std::string& min, std::size_t offset, std::size_t count, const reply_callback_t& reply_callback) {
  return zrevrangebylex(key, max, min, true, offset, count, false, reply_callback);
}

client&
client::zrevrangebylex(const std::string& key, const std::string& max, const std::string& min, std::size_t offset, std::size_t count, bool withscores, const reply_callback_t& reply_callback) {
  return zrevrangebylex(key, max, min, true, offset, count, withscores, reply_callback);
}

client&
client::zrevrangebylex(const std::string& key, const std::string& max, const std::string& min, bool limit, std::size_t offset, std::size_t count, bool withscores, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"ZREVRANGEBYLEX", key, max, min};

  //! withscores (optional)
  if (withscores) {
    cmd.push_back("WITHSCORES");
  }

  //! limit (optional)
  if (limit) {
    cmd.push_back("LIMIT");
    cmd.push_back(std::to_string(offset));
    cmd.push_back(std::to_string(count));
  }

  send(cmd, reply_callback);
  return *this;
}

client&
client::zrevrangebyscore(const std::string& key, int max, int min, const reply_callback_t& reply_callback) {
  return zrevrangebyscore(key, std::to_string(max), std::to_string(min), false, 0, 0, false, reply_callback);
}

client&
client::zrevrangebyscore(const std::string& key, int max, int min, bool withscores, const reply_callback_t& reply_callback) {
  return zrevrangebyscore(key, std::to_string(max), std::to_string(min), false, 0, 0, withscores, reply_callback);
}

client&
client::zrevrangebyscore(const std::string& key, double max, double min, const reply_callback_t& reply_callback) {
  return zrevrangebyscore(key, std::to_string(max), std::to_string(min), false, 0, 0, false, reply_callback);
}

client&
client::zrevrangebyscore(const std::string& key, double max, double min, bool withscores, const reply_callback_t& reply_callback) {
  return zrevrangebyscore(key, std::to_string(max), std::to_string(min), false, 0, 0, withscores, reply_callback);
}

client&
client::zrevrangebyscore(const std::string& key, const std::string& max, const std::string& min, const reply_callback_t& reply_callback) {
  return zrevrangebyscore(key, max, min, false, 0, 0, false, reply_callback);
}

client&
client::zrevrangebyscore(const std::string& key, const std::string& max, const std::string& min, bool withscores, const reply_callback_t& reply_callback) {
  return zrevrangebyscore(key, max, min, false, 0, 0, withscores, reply_callback);
}

client&
client::zrevrangebyscore(const std::string& key, int max, int min, std::size_t offset, std::size_t count, const reply_callback_t& reply_callback) {
  return zrevrangebyscore(key, std::to_string(max), std::to_string(min), true, offset, count, false, reply_callback);
}

client&
client::zrevrangebyscore(const std::string& key, int max, int min, std::size_t offset, std::size_t count, bool withscores, const reply_callback_t& reply_callback) {
  return zrevrangebyscore(key, std::to_string(max), std::to_string(min), true, offset, count, withscores, reply_callback);
}

client&
client::zrevrangebyscore(const std::string& key, double max, double min, std::size_t offset, std::size_t count, const reply_callback_t& reply_callback) {
  return zrevrangebyscore(key, std::to_string(max), std::to_string(min), true, offset, count, false, reply_callback);
}

client&
client::zrevrangebyscore(const std::string& key, double max, double min, std::size_t offset, std::size_t count, bool withscores, const reply_callback_t& reply_callback) {
  return zrevrangebyscore(key, std::to_string(max), std::to_string(min), true, offset, count, withscores, reply_callback);
}

client&
client::zrevrangebyscore(const std::string& key, const std::string& max, const std::string& min, std::size_t offset, std::size_t count, const reply_callback_t& reply_callback) {
  return zrevrangebyscore(key, max, min, true, offset, count, false, reply_callback);
}

client&
client::zrevrangebyscore(const std::string& key, const std::string& max, const std::string& min, std::size_t offset, std::size_t count, bool withscores, const reply_callback_t& reply_callback) {
  return zrevrangebyscore(key, max, min, true, offset, count, withscores, reply_callback);
}

client&
client::zrevrangebyscore(const std::string& key, const std::string& max, const std::string& min, bool limit, std::size_t offset, std::size_t count, bool withscores, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"ZREVRANGEBYSCORE", key, max, min};

  //! withscores (optional)
  if (withscores) {
    cmd.push_back("WITHSCORES");
  }

  //! limit (optional)
  if (limit) {
    cmd.push_back("LIMIT");
    cmd.push_back(std::to_string(offset));
    cmd.push_back(std::to_string(count));
  }

  send(cmd, reply_callback);
  return *this;
}

client&
client::zrevrank(const std::string& key, const std::string& member, const reply_callback_t& reply_callback) {
  send({"ZREVRANK", key, member}, reply_callback);
  return *this;
}

client&
client::zscan(const std::string& key, std::size_t cursor, const reply_callback_t& reply_callback) {
  return zscan(key, cursor, "", 0, reply_callback);
}

client&
client::zscan(const std::string& key, std::size_t cursor, const std::string& pattern, const reply_callback_t& reply_callback) {
  return zscan(key, cursor, pattern, 0, reply_callback);
}

client&
client::zscan(const std::string& key, std::size_t cursor, std::size_t count, const reply_callback_t& reply_callback) {
  return zscan(key, cursor, "", count, reply_callback);
}

client&
client::zscan(const std::string& key, std::size_t cursor, const std::string& pattern, std::size_t count, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"ZSCAN", key, std::to_string(cursor)};

  if (!pattern.empty()) {
    cmd.push_back("MATCH");
    cmd.push_back(pattern);
  }

  if (count > 0) {
    cmd.push_back("COUNT");
    cmd.push_back(std::to_string(count));
  }

  send(cmd, reply_callback);
  return *this;
}

client&
client::zscore(const std::string& key, const std::string& member, const reply_callback_t& reply_callback) {
  send({"ZSCORE", key, member}, reply_callback);
  return *this;
}

client&
client::zunionstore(const std::string& destination, std::size_t numkeys, const std::vector<std::string>& keys, const std::vector<std::size_t> weights, aggregate_method method, const reply_callback_t& reply_callback) {
  std::vector<std::string> cmd = {"ZUNIONSTORE", destination, std::to_string(numkeys)};

  //! keys
  for (const auto& key : keys) {
    cmd.push_back(key);
  }

  //! weights (optional)
  if (!weights.empty()) {
    cmd.push_back("WEIGHTS");

    for (auto weight : weights) {
      cmd.push_back(std::to_string(weight));
    }
  }

  //! aggregate method
  if (method != aggregate_method::server_default) {
    cmd.push_back("AGGREGATE");
    cmd.push_back(aggregate_method_to_string(method));
  }

  send(cmd, reply_callback);
  return *this;
}

//!
//! Redis Commands
//! std::future-based
//!

std::future<reply>
client::exec_cmd(const std::function<client&(const reply_callback_t&)>& f) {
  auto prms = std::make_shared<std::promise<reply>>();

  f([prms](reply& reply) {
    prms->set_value(reply);
  });

  return prms->get_future();
}

std::future<reply>
client::send(const std::vector<std::string>& redis_cmd) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return send(redis_cmd, cb); });
}

std::future<reply>
client::append(const std::string& key, const std::string& value) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return append(key, value, cb); });
}

std::future<reply>
client::auth(const std::string& password) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return auth(password, cb); });
}

std::future<reply>
client::bgrewriteaof() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return bgrewriteaof(cb); });
}

std::future<reply>
client::bgsave() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return bgsave(cb); });
}

std::future<reply>
client::bitcount(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return bitcount(key, cb); });
}

std::future<reply>
client::bitcount(const std::string& key, int start, int end) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return bitcount(key, start, end, cb); });
}

std::future<reply>
client::bitfield(const std::string& key, const std::vector<bitfield_operation>& operations) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return bitfield(key, operations, cb); });
}

std::future<reply>
client::bitop(const std::string& operation, const std::string& destkey, const std::vector<std::string>& keys) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return bitop(operation, destkey, keys, cb); });
}

std::future<reply>
client::bitpos(const std::string& key, int bit) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return bitpos(key, bit, cb); });
}

std::future<reply>
client::bitpos(const std::string& key, int bit, int start) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return bitpos(key, bit, start, cb); });
}

std::future<reply>
client::bitpos(const std::string& key, int bit, int start, int end) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return bitpos(key, bit, start, end, cb); });
}

std::future<reply>
client::blpop(const std::vector<std::string>& keys, int timeout) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return blpop(keys, timeout, cb); });
}

std::future<reply>
client::brpop(const std::vector<std::string>& keys, int timeout) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return brpop(keys, timeout, cb); });
}

std::future<reply>
client::brpoplpush(const std::string& src, const std::string& dst, int timeout) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return brpoplpush(src, dst, timeout, cb); });
}

std::future<reply>
client::client_list() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return client_list(cb); });
}

std::future<reply>
client::client_getname() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return client_getname(cb); });
}

std::future<reply>
client::client_pause(int timeout) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return client_pause(timeout, cb); });
}

std::future<reply>
client::client_reply(const std::string& mode) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return client_reply(mode, cb); });
}

std::future<reply>
client::client_setname(const std::string& name) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return client_setname(name, cb); });
}

std::future<reply>
client::cluster_addslots(const std::vector<std::string>& p_slots) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return cluster_addslots(p_slots, cb); });
}

std::future<reply>
client::cluster_count_failure_reports(const std::string& node_id) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return cluster_count_failure_reports(node_id, cb); });
}

std::future<reply>
client::cluster_countkeysinslot(const std::string& slot) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return cluster_countkeysinslot(slot, cb); });
}

std::future<reply>
client::cluster_delslots(const std::vector<std::string>& p_slots) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return cluster_delslots(p_slots, cb); });
}

std::future<reply>
client::cluster_failover() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return cluster_failover(cb); });
}

std::future<reply>
client::cluster_failover(const std::string& mode) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return cluster_failover(mode, cb); });
}

std::future<reply>
client::cluster_forget(const std::string& node_id) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return cluster_forget(node_id, cb); });
}

std::future<reply>
client::cluster_getkeysinslot(const std::string& slot, int count) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return cluster_getkeysinslot(slot, count, cb); });
}

std::future<reply>
client::cluster_info() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return cluster_info(cb); });
}

std::future<reply>
client::cluster_keyslot(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return cluster_keyslot(key, cb); });
}

std::future<reply>
client::cluster_meet(const std::string& ip, int port) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return cluster_meet(ip, port, cb); });
}

std::future<reply>
client::cluster_nodes() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return cluster_nodes(cb); });
}

std::future<reply>
client::cluster_replicate(const std::string& node_id) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return cluster_replicate(node_id, cb); });
}

std::future<reply>
client::cluster_reset(const std::string& mode) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return cluster_reset(mode, cb); });
}

std::future<reply>
client::cluster_saveconfig() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return cluster_saveconfig(cb); });
}

std::future<reply>
client::cluster_set_config_epoch(const std::string& epoch) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return cluster_set_config_epoch(epoch, cb); });
}

std::future<reply>
client::cluster_setslot(const std::string& slot, const std::string& mode) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return cluster_setslot(slot, mode, cb); });
}

std::future<reply>
client::cluster_setslot(const std::string& slot, const std::string& mode, const std::string& node_id) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return cluster_setslot(slot, mode, node_id, cb); });
}

std::future<reply>
client::cluster_slaves(const std::string& node_id) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return cluster_slaves(node_id, cb); });
}

std::future<reply>
client::cluster_slots() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return cluster_slots(cb); });
}

std::future<reply>
client::command() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return command(cb); });
}

std::future<reply>
client::command_count() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return command_count(cb); });
}

std::future<reply>
client::command_getkeys() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return command_getkeys(cb); });
}

std::future<reply>
client::command_info(const std::vector<std::string>& command_name) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return command_info(command_name, cb); });
}

std::future<reply>
client::config_get(const std::string& param) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return config_get(param, cb); });
}

std::future<reply>
client::config_rewrite() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return config_rewrite(cb); });
}

std::future<reply>
client::config_set(const std::string& param, const std::string& val) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return config_set(param, val, cb); });
}

std::future<reply>
client::config_resetstat() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return config_resetstat(cb); });
}

std::future<reply>
client::dbsize() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return dbsize(cb); });
}

std::future<reply>
client::debug_object(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return debug_object(key, cb); });
}

std::future<reply>
client::debug_segfault() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return debug_segfault(cb); });
}

std::future<reply>
client::decr(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return decr(key, cb); });
}

std::future<reply>
client::decrby(const std::string& key, int val) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return decrby(key, val, cb); });
}

std::future<reply>
client::del(const std::vector<std::string>& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return del(key, cb); });
}

std::future<reply>
client::discard() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return discard(cb); });
}

std::future<reply>
client::dump(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return dump(key, cb); });
}

std::future<reply>
client::echo(const std::string& msg) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return echo(msg, cb); });
}

std::future<reply>
client::eval(const std::string& script, int numkeys, const std::vector<std::string>& keys, const std::vector<std::string>& args) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return eval(script, numkeys, keys, args, cb); });
}

std::future<reply>
client::evalsha(const std::string& sha1, int numkeys, const std::vector<std::string>& keys, const std::vector<std::string>& args) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return evalsha(sha1, numkeys, keys, args, cb); });
}

std::future<reply>
client::exec() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return exec(cb); });
}

std::future<reply>
client::exists(const std::vector<std::string>& keys) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return exists(keys, cb); });
}

std::future<reply>
client::expire(const std::string& key, int seconds) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return expire(key, seconds, cb); });
}

std::future<reply>
client::expireat(const std::string& key, int timestamp) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return expireat(key, timestamp, cb); });
}

std::future<reply>
client::flushall() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return flushall(cb); });
}

std::future<reply>
client::flushdb() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return flushdb(cb); });
}

std::future<reply>
client::geoadd(const std::string& key, const std::vector<std::tuple<std::string, std::string, std::string>>& long_lat_memb) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return geoadd(key, long_lat_memb, cb); });
}

std::future<reply>
client::geohash(const std::string& key, const std::vector<std::string>& members) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return geohash(key, members, cb); });
}

std::future<reply>
client::geopos(const std::string& key, const std::vector<std::string>& members) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return geopos(key, members, cb); });
}

std::future<reply>
client::geodist(const std::string& key, const std::string& member_1, const std::string& member_2, const std::string& unit) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return geodist(key, member_1, member_2, unit, cb); });
}

std::future<reply>
client::georadius(const std::string& key, double longitude, double latitude, double radius, geo_unit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, std::size_t count, const std::string& store_key, const std::string& storedist_key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return georadius(key, longitude, latitude, radius, unit, with_coord, with_dist, with_hash, asc_order, count, store_key, storedist_key, cb); });
}

std::future<reply>
client::georadiusbymember(const std::string& key, const std::string& member, double radius, geo_unit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, std::size_t count, const std::string& store_key, const std::string& storedist_key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return georadiusbymember(key, member, radius, unit, with_coord, with_dist, with_hash, asc_order, count, store_key, storedist_key, cb); });
}

std::future<reply>
client::get(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return get(key, cb); });
}

std::future<reply>
client::getbit(const std::string& key, int offset) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return getbit(key, offset, cb); });
}

std::future<reply>
client::getrange(const std::string& key, int start, int end) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return getrange(key, start, end, cb); });
}

std::future<reply>
client::getset(const std::string& key, const std::string& val) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return getset(key, val, cb); });
}

std::future<reply>
client::hdel(const std::string& key, const std::vector<std::string>& fields) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return hdel(key, fields, cb); });
}

std::future<reply>
client::hexists(const std::string& key, const std::string& field) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return hexists(key, field, cb); });
}

std::future<reply>
client::hget(const std::string& key, const std::string& field) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return hget(key, field, cb); });
}

std::future<reply>
client::hgetall(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return hgetall(key, cb); });
}

std::future<reply>
client::hincrby(const std::string& key, const std::string& field, int incr) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return hincrby(key, field, incr, cb); });
}

std::future<reply>
client::hincrbyfloat(const std::string& key, const std::string& field, float incr) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return hincrbyfloat(key, field, incr, cb); });
}

std::future<reply>
client::hkeys(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return hkeys(key, cb); });
}

std::future<reply>
client::hlen(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return hlen(key, cb); });
}

std::future<reply>
client::hmget(const std::string& key, const std::vector<std::string>& fields) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return hmget(key, fields, cb); });
}

std::future<reply>
client::hmset(const std::string& key, const std::vector<std::pair<std::string, std::string>>& field_val) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return hmset(key, field_val, cb); });
}

std::future<reply>
client::hscan(const std::string& key, std::size_t cursor) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return hscan(key, cursor, cb); });
}

std::future<reply>
client::hscan(const std::string& key, std::size_t cursor, const std::string& pattern) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return hscan(key, cursor, pattern, cb); });
}

std::future<reply>
client::hscan(const std::string& key, std::size_t cursor, std::size_t count) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return hscan(key, cursor, count, cb); });
}

std::future<reply>
client::hscan(const std::string& key, std::size_t cursor, const std::string& pattern, std::size_t count) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return hscan(key, cursor, pattern, count, cb); });
}

std::future<reply>
client::hset(const std::string& key, const std::string& field, const std::string& value) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return hset(key, field, value, cb); });
}

std::future<reply>
client::hsetnx(const std::string& key, const std::string& field, const std::string& value) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return hsetnx(key, field, value, cb); });
}

std::future<reply>
client::hstrlen(const std::string& key, const std::string& field) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return hstrlen(key, field, cb); });
}

std::future<reply>
client::hvals(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return hvals(key, cb); });
}

std::future<reply>
client::incr(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return incr(key, cb); });
}

std::future<reply>
client::incrby(const std::string& key, int incr) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return incrby(key, incr, cb); });
}

std::future<reply>
client::incrbyfloat(const std::string& key, float incr) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return incrbyfloat(key, incr, cb); });
}

std::future<reply>
client::info(const std::string& section) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return info(section, cb); });
}

std::future<reply>
client::keys(const std::string& pattern) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return keys(pattern, cb); });
}

std::future<reply>
client::lastsave() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return lastsave(cb); });
}

std::future<reply>
client::lindex(const std::string& key, int index) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return lindex(key, index, cb); });
}

std::future<reply>
client::linsert(const std::string& key, const std::string& before_after, const std::string& pivot, const std::string& value) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return linsert(key, before_after, pivot, value, cb); });
}

std::future<reply>
client::llen(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return llen(key, cb); });
}

std::future<reply>
client::lpop(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return lpop(key, cb); });
}

std::future<reply>
client::lpush(const std::string& key, const std::vector<std::string>& values) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return lpush(key, values, cb); });
}

std::future<reply>
client::lpushx(const std::string& key, const std::string& value) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return lpushx(key, value, cb); });
}

std::future<reply>
client::lrange(const std::string& key, int start, int stop) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return lrange(key, start, stop, cb); });
}

std::future<reply>
client::lrem(const std::string& key, int count, const std::string& value) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return lrem(key, count, value, cb); });
}

std::future<reply>
client::lset(const std::string& key, int index, const std::string& value) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return lset(key, index, value, cb); });
}

std::future<reply>
client::ltrim(const std::string& key, int start, int stop) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return ltrim(key, start, stop, cb); });
}

std::future<reply>
client::mget(const std::vector<std::string>& keys) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return mget(keys, cb); });
}

std::future<reply>
client::migrate(const std::string& host, int port, const std::string& key, const std::string& dest_db, int timeout, bool copy, bool replace, const std::vector<std::string>& keys) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return migrate(host, port, key, dest_db, timeout, copy, replace, keys, cb); });
}

std::future<reply>
client::monitor() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return monitor(cb); });
}

std::future<reply>
client::move(const std::string& key, const std::string& db) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return move(key, db, cb); });
}

std::future<reply>
client::mset(const std::vector<std::pair<std::string, std::string>>& key_vals) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return mset(key_vals, cb); });
}

std::future<reply>
client::msetnx(const std::vector<std::pair<std::string, std::string>>& key_vals) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return msetnx(key_vals, cb); });
}

std::future<reply>
client::multi() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return multi(cb); });
}

std::future<reply>
client::object(const std::string& subcommand, const std::vector<std::string>& args) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return object(subcommand, args, cb); });
}

std::future<reply>
client::persist(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return persist(key, cb); });
}

std::future<reply>
client::pexpire(const std::string& key, int milliseconds) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return pexpire(key, milliseconds, cb); });
}

std::future<reply>
client::pexpireat(const std::string& key, int milliseconds_timestamp) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return pexpireat(key, milliseconds_timestamp, cb); });
}

std::future<reply>
client::pfadd(const std::string& key, const std::vector<std::string>& elements) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return pfadd(key, elements, cb); });
}

std::future<reply>
client::pfcount(const std::vector<std::string>& keys) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return pfcount(keys, cb); });
}

std::future<reply>
client::pfmerge(const std::string& destkey, const std::vector<std::string>& sourcekeys) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return pfmerge(destkey, sourcekeys, cb); });
}

std::future<reply>
client::ping() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return ping(cb); });
}

std::future<reply>
client::ping(const std::string& message) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return ping(message, cb); });
}

std::future<reply>
client::psetex(const std::string& key, int milliseconds, const std::string& val) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return psetex(key, milliseconds, val, cb); });
}

std::future<reply>
client::publish(const std::string& channel, const std::string& message) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return publish(channel, message, cb); });
}

std::future<reply>
client::pubsub(const std::string& subcommand, const std::vector<std::string>& args) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return pubsub(subcommand, args, cb); });
}

std::future<reply>
client::pttl(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return pttl(key, cb); });
}

std::future<reply>
client::quit() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return quit(cb); });
}

std::future<reply>
client::randomkey() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return randomkey(cb); });
}

std::future<reply>
client::readonly() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return readonly(cb); });
}

std::future<reply>
client::readwrite() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return readwrite(cb); });
}

std::future<reply>
client::rename(const std::string& key, const std::string& newkey) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return rename(key, newkey, cb); });
}

std::future<reply>
client::renamenx(const std::string& key, const std::string& newkey) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return renamenx(key, newkey, cb); });
}

std::future<reply>
client::restore(const std::string& key, int ttl, const std::string& serialized_value) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return restore(key, ttl, serialized_value, cb); });
}

std::future<reply>
client::restore(const std::string& key, int ttl, const std::string& serialized_value, const std::string& replace) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return restore(key, ttl, serialized_value, replace, cb); });
}

std::future<reply>
client::role() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return role(cb); });
}

std::future<reply>
client::rpop(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return rpop(key, cb); });
}

std::future<reply>
client::rpoplpush(const std::string& src, const std::string& dst) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return rpoplpush(src, dst, cb); });
}

std::future<reply>
client::rpush(const std::string& key, const std::vector<std::string>& values) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return rpush(key, values, cb); });
}

std::future<reply>
client::rpushx(const std::string& key, const std::string& value) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return rpushx(key, value, cb); });
}

std::future<reply>
client::sadd(const std::string& key, const std::vector<std::string>& members) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sadd(key, members, cb); });
}

std::future<reply>
client::scan(std::size_t cursor) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return scan(cursor, cb); });
}

std::future<reply>
client::scan(std::size_t cursor, const std::string& pattern) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return scan(cursor, pattern, cb); });
}

std::future<reply>
client::scan(std::size_t cursor, std::size_t count) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return scan(cursor, count, cb); });
}

std::future<reply>
client::scan(std::size_t cursor, const std::string& pattern, std::size_t count) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return scan(cursor, pattern, count, cb); });
}

std::future<reply>
client::save() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return save(cb); });
}

std::future<reply>
client::scard(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return scard(key, cb); });
}

std::future<reply>
client::script_debug(const std::string& mode) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return script_debug(mode, cb); });
}

std::future<reply>
client::script_exists(const std::vector<std::string>& scripts) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return script_exists(scripts, cb); });
}

std::future<reply>
client::script_flush() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return script_flush(cb); });
}

std::future<reply>
client::script_kill() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return script_kill(cb); });
}

std::future<reply>
client::script_load(const std::string& script) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return script_load(script, cb); });
}

std::future<reply>
client::sdiff(const std::vector<std::string>& keys) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sdiff(keys, cb); });
}

std::future<reply>
client::sdiffstore(const std::string& dst, const std::vector<std::string>& keys) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sdiffstore(dst, keys, cb); });
}

std::future<reply>
client::select(int index) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return select(index, cb); });
}

std::future<reply>
client::set(const std::string& key, const std::string& value) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return set(key, value, cb); });
}

std::future<reply>
client::set_advanced(const std::string& key, const std::string& value, bool ex, int ex_sec, bool px, int px_milli, bool nx, bool xx) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return set_advanced(key, value, ex, ex_sec, px, px_milli, nx, xx, cb); });
}

std::future<reply>
client::setbit_(const std::string& key, int offset, const std::string& value) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return setbit_(key, offset, value, cb); });
}

std::future<reply>
client::setex(const std::string& key, int seconds, const std::string& value) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return setex(key, seconds, value, cb); });
}

std::future<reply>
client::setnx(const std::string& key, const std::string& value) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return setnx(key, value, cb); });
}

std::future<reply>
client::setrange(const std::string& key, int offset, const std::string& value) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return setrange(key, offset, value, cb); });
}

std::future<reply>
client::shutdown() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return shutdown(cb); });
}

std::future<reply>
client::shutdown(const std::string& save) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return shutdown(save, cb); });
}

std::future<reply>
client::sinter(const std::vector<std::string>& keys) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sinter(keys, cb); });
}

std::future<reply>
client::sinterstore(const std::string& dst, const std::vector<std::string>& keys) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sinterstore(dst, keys, cb); });
}

std::future<reply>
client::sismember(const std::string& key, const std::string& member) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sismember(key, member, cb); });
}

std::future<reply>
client::slaveof(const std::string& host, int port) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return slaveof(host, port, cb); });
}

std::future<reply>
client::slowlog(const std::string& subcommand) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return slowlog(subcommand, cb); });
}

std::future<reply>
client::slowlog(const std::string& subcommand, const std::string& argument) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return slowlog(subcommand, argument, cb); });
}

std::future<reply>
client::smembers(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return smembers(key, cb); });
}

std::future<reply>
client::smove(const std::string& src, const std::string& dst, const std::string& member) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return smove(src, dst, member, cb); });
}

std::future<reply>
client::sort(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sort(key, cb); });
}

std::future<reply>
client::sort(const std::string& key, const std::vector<std::string>& get_patterns, bool asc_order, bool alpha) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sort(key, get_patterns, asc_order, alpha, cb); });
}

std::future<reply>
client::sort(const std::string& key, std::size_t offset, std::size_t count, const std::vector<std::string>& get_patterns, bool asc_order, bool alpha) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sort(key, offset, count, get_patterns, asc_order, alpha, cb); });
}

std::future<reply>
client::sort(const std::string& key, const std::string& by_pattern, const std::vector<std::string>& get_patterns, bool asc_order, bool alpha) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sort(key, by_pattern, get_patterns, asc_order, alpha, cb); });
}

std::future<reply>
client::sort(const std::string& key, const std::vector<std::string>& get_patterns, bool asc_order, bool alpha, const std::string& store_dest) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sort(key, get_patterns, asc_order, alpha, store_dest, cb); });
}

std::future<reply>
client::sort(const std::string& key, std::size_t offset, std::size_t count, const std::vector<std::string>& get_patterns, bool asc_order, bool alpha, const std::string& store_dest) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sort(key, offset, count, get_patterns, asc_order, alpha, store_dest, cb); });
}

std::future<reply>
client::sort(const std::string& key, const std::string& by_pattern, const std::vector<std::string>& get_patterns, bool asc_order, bool alpha, const std::string& store_dest) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sort(key, by_pattern, get_patterns, asc_order, alpha, store_dest, cb); });
}

std::future<reply>
client::sort(const std::string& key, const std::string& by_pattern, std::size_t offset, std::size_t count, const std::vector<std::string>& get_patterns, bool asc_order, bool alpha) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sort(key, by_pattern, offset, count, get_patterns, asc_order, alpha, cb); });
}

std::future<reply>
client::sort(const std::string& key, const std::string& by_pattern, std::size_t offset, std::size_t count, const std::vector<std::string>& get_patterns, bool asc_order, bool alpha, const std::string& store_dest) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sort(key, by_pattern, offset, count, get_patterns, asc_order, alpha, store_dest, cb); });
}

std::future<reply>
client::spop(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return spop(key, cb); });
}

std::future<reply>
client::spop(const std::string& key, int count) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return spop(key, count, cb); });
}

std::future<reply>
client::srandmember(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return srandmember(key, cb); });
}

std::future<reply>
client::srandmember(const std::string& key, int count) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return srandmember(key, count, cb); });
}

std::future<reply>
client::srem(const std::string& key, const std::vector<std::string>& members) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return srem(key, members, cb); });
}

std::future<reply>
client::sscan(const std::string& key, std::size_t cursor) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sscan(key, cursor, cb); });
}

std::future<reply>
client::sscan(const std::string& key, std::size_t cursor, const std::string& pattern) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sscan(key, cursor, pattern, cb); });
}

std::future<reply>
client::sscan(const std::string& key, std::size_t cursor, std::size_t count) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sscan(key, cursor, count, cb); });
}

std::future<reply>
client::sscan(const std::string& key, std::size_t cursor, const std::string& pattern, std::size_t count) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sscan(key, cursor, pattern, count, cb); });
}

std::future<reply>
client::strlen(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return strlen(key, cb); });
}

std::future<reply>
client::sunion(const std::vector<std::string>& keys) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sunion(keys, cb); });
}

std::future<reply>
client::sunionstore(const std::string& dst, const std::vector<std::string>& keys) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sunionstore(dst, keys, cb); });
}

std::future<reply>
client::sync() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return sync(cb); });
}

std::future<reply>
client::time() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return time(cb); });
}

std::future<reply>
client::ttl(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return ttl(key, cb); });
}

std::future<reply>
client::type(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return type(key, cb); });
}

std::future<reply>
client::unwatch() {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return unwatch(cb); });
}

std::future<reply>
client::wait(int numslaves, int timeout) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return wait(numslaves, timeout, cb); });
}

std::future<reply>
client::watch(const std::vector<std::string>& keys) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return watch(keys, cb); });
}

std::future<reply>
client::zadd(const std::string& key, const std::vector<std::string>& options, const std::multimap<std::string, std::string>& score_members) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zadd(key, options, score_members, cb); });
}

std::future<reply>
client::zcard(const std::string& key) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zcard(key, cb); });
}

std::future<reply>
client::zcount(const std::string& key, int min, int max) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zcount(key, min, max, cb); });
}

std::future<reply>
client::zcount(const std::string& key, double min, double max) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zcount(key, min, max, cb); });
}

std::future<reply>
client::zcount(const std::string& key, const std::string& min, const std::string& max) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zcount(key, min, max, cb); });
}

std::future<reply>
client::zincrby(const std::string& key, int incr, const std::string& member) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zincrby(key, incr, member, cb); });
}

std::future<reply>
client::zincrby(const std::string& key, double incr, const std::string& member) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zincrby(key, incr, member, cb); });
}

std::future<reply>
client::zincrby(const std::string& key, const std::string& incr, const std::string& member) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zincrby(key, incr, member, cb); });
}

std::future<reply>
client::zinterstore(const std::string& destination, std::size_t numkeys, const std::vector<std::string>& keys, const std::vector<std::size_t> weights, aggregate_method method) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zinterstore(destination, numkeys, keys, weights, method, cb); });
}

std::future<reply>
client::zlexcount(const std::string& key, int min, int max) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zlexcount(key, min, max, cb); });
}

std::future<reply>
client::zlexcount(const std::string& key, double min, double max) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zlexcount(key, min, max, cb); });
}

std::future<reply>
client::zlexcount(const std::string& key, const std::string& min, const std::string& max) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zlexcount(key, min, max, cb); });
}

std::future<reply>
client::zrange(const std::string& key, int start, int stop, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrange(key, start, stop, withscores, cb); });
}

std::future<reply>
client::zrange(const std::string& key, double start, double stop, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrange(key, start, stop, withscores, cb); });
}

std::future<reply>
client::zrange(const std::string& key, const std::string& start, const std::string& stop, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrange(key, start, stop, withscores, cb); });
}

std::future<reply>
client::zrangebylex(const std::string& key, int min, int max, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrangebylex(key, min, max, withscores, cb); });
}

std::future<reply>
client::zrangebylex(const std::string& key, double min, double max, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrangebylex(key, min, max, withscores, cb); });
}

std::future<reply>
client::zrangebylex(const std::string& key, const std::string& min, const std::string& max, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrangebylex(key, min, max, withscores, cb); });
}

std::future<reply>
client::zrangebylex(const std::string& key, int min, int max, std::size_t offset, std::size_t count, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrangebylex(key, min, max, offset, count, withscores, cb); });
}

std::future<reply>
client::zrangebylex(const std::string& key, double min, double max, std::size_t offset, std::size_t count, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrangebylex(key, min, max, offset, count, withscores, cb); });
}

std::future<reply>
client::zrangebylex(const std::string& key, const std::string& min, const std::string& max, std::size_t offset, std::size_t count, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrangebylex(key, min, max, offset, count, withscores, cb); });
}

std::future<reply>
client::zrangebyscore(const std::string& key, int min, int max, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrangebyscore(key, min, max, withscores, cb); });
}

std::future<reply>
client::zrangebyscore(const std::string& key, double min, double max, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrangebyscore(key, min, max, withscores, cb); });
}

std::future<reply>
client::zrangebyscore(const std::string& key, const std::string& min, const std::string& max, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrangebyscore(key, min, max, withscores, cb); });
}

std::future<reply>
client::zrangebyscore(const std::string& key, int min, int max, std::size_t offset, std::size_t count, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrangebyscore(key, min, max, offset, count, withscores, cb); });
}

std::future<reply>
client::zrangebyscore(const std::string& key, double min, double max, std::size_t offset, std::size_t count, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrangebyscore(key, min, max, offset, count, withscores, cb); });
}

std::future<reply>
client::zrangebyscore(const std::string& key, const std::string& min, const std::string& max, std::size_t offset, std::size_t count, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrangebyscore(key, min, max, offset, count, withscores, cb); });
}

std::future<reply>
client::zrank(const std::string& key, const std::string& member) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrank(key, member, cb); });
}

std::future<reply>
client::zrem(const std::string& key, const std::vector<std::string>& members) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrem(key, members, cb); });
}

std::future<reply>
client::zremrangebylex(const std::string& key, int min, int max) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zremrangebylex(key, min, max, cb); });
}

std::future<reply>
client::zremrangebylex(const std::string& key, double min, double max) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zremrangebylex(key, min, max, cb); });
}

std::future<reply>
client::zremrangebylex(const std::string& key, const std::string& min, const std::string& max) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zremrangebylex(key, min, max, cb); });
}

std::future<reply>
client::zremrangebyrank(const std::string& key, int start, int stop) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zremrangebyrank(key, start, stop, cb); });
}

std::future<reply>
client::zremrangebyrank(const std::string& key, double start, double stop) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zremrangebyrank(key, start, stop, cb); });
}

std::future<reply>
client::zremrangebyrank(const std::string& key, const std::string& start, const std::string& stop) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zremrangebyrank(key, start, stop, cb); });
}

std::future<reply>
client::zremrangebyscore(const std::string& key, int min, int max) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zremrangebyscore(key, min, max, cb); });
}

std::future<reply>
client::zremrangebyscore(const std::string& key, double min, double max) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zremrangebyscore(key, min, max, cb); });
}

std::future<reply>
client::zremrangebyscore(const std::string& key, const std::string& min, const std::string& max) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zremrangebyscore(key, min, max, cb); });
}

std::future<reply>
client::zrevrange(const std::string& key, int start, int stop, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrevrange(key, start, stop, withscores, cb); });
}

std::future<reply>
client::zrevrange(const std::string& key, double start, double stop, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrevrange(key, start, stop, withscores, cb); });
}

std::future<reply>
client::zrevrange(const std::string& key, const std::string& start, const std::string& stop, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrevrange(key, start, stop, withscores, cb); });
}

std::future<reply>
client::zrevrangebylex(const std::string& key, int max, int min, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrevrangebylex(key, max, min, withscores, cb); });
}

std::future<reply>
client::zrevrangebylex(const std::string& key, double max, double min, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrevrangebylex(key, max, min, withscores, cb); });
}

std::future<reply>
client::zrevrangebylex(const std::string& key, const std::string& max, const std::string& min, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrevrangebylex(key, max, min, withscores, cb); });
}

std::future<reply>
client::zrevrangebylex(const std::string& key, int max, int min, std::size_t offset, std::size_t count, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrevrangebylex(key, max, min, offset, count, withscores, cb); });
}

std::future<reply>
client::zrevrangebylex(const std::string& key, double max, double min, std::size_t offset, std::size_t count, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrevrangebylex(key, max, min, offset, count, withscores, cb); });
}

std::future<reply>
client::zrevrangebylex(const std::string& key, const std::string& max, const std::string& min, std::size_t offset, std::size_t count, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrevrangebylex(key, max, min, offset, count, withscores, cb); });
}

std::future<reply>
client::zrevrangebyscore(const std::string& key, int max, int min, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrevrangebyscore(key, max, min, withscores, cb); });
}

std::future<reply>
client::zrevrangebyscore(const std::string& key, double max, double min, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrevrangebyscore(key, max, min, withscores, cb); });
}

std::future<reply>
client::zrevrangebyscore(const std::string& key, const std::string& max, const std::string& min, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrevrangebyscore(key, max, min, withscores, cb); });
}

std::future<reply>
client::zrevrangebyscore(const std::string& key, int max, int min, std::size_t offset, std::size_t count, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrevrangebyscore(key, max, min, offset, count, withscores, cb); });
}

std::future<reply>
client::zrevrangebyscore(const std::string& key, double max, double min, std::size_t offset, std::size_t count, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrevrangebyscore(key, max, min, offset, count, withscores, cb); });
}

std::future<reply>
client::zrevrangebyscore(const std::string& key, const std::string& max, const std::string& min, std::size_t offset, std::size_t count, bool withscores) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrevrangebyscore(key, max, min, offset, count, withscores, cb); });
}

std::future<reply>
client::zrevrank(const std::string& key, const std::string& member) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zrevrank(key, member, cb); });
}

std::future<reply>
client::zscan(const std::string& key, std::size_t cursor) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zscan(key, cursor, cb); });
}

std::future<reply>
client::zscan(const std::string& key, std::size_t cursor, const std::string& pattern) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zscan(key, cursor, pattern, cb); });
}

std::future<reply>
client::zscan(const std::string& key, std::size_t cursor, std::size_t count) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zscan(key, cursor, count, cb); });
}

std::future<reply>
client::zscan(const std::string& key, std::size_t cursor, const std::string& pattern, std::size_t count) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zscan(key, cursor, pattern, count, cb); });
}

std::future<reply>
client::zscore(const std::string& key, const std::string& member) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zscore(key, member, cb); });
}

std::future<reply>
client::zunionstore(const std::string& destination, std::size_t numkeys, const std::vector<std::string>& keys, const std::vector<std::size_t> weights, aggregate_method method) {
  return exec_cmd([=](const reply_callback_t& cb) -> client& { return zunionstore(destination, numkeys, keys, weights, method, cb); });
}

} //! cpp_redis
