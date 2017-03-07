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
#include <future>
#include <memory>

#include <cpp_redis/logger.hpp>
#include <cpp_redis/redis_client.hpp>

namespace cpp_redis {

class future_client {

public:
  using result_type             = reply;
  using promise                 = std::promise<result_type>;
  using future                  = std::future<result_type>;
  using reply_callback_t        = redis_client::reply_callback_t;
  using disconnection_handler_t = redis_client::disconnection_handler_t;

private:
  using rc    = redis_client;
  using rcb_t = reply_callback_t;

  using call_t = std::function<rc&(const reply_callback_t&)>;

  //! Execute a command on the redis_client and tie the callback to a future
  future
  exec_cmd(call_t f) {
    auto prms = std::make_shared<promise>();
    f([prms](reply& reply) {
      prms->set_value(reply);
    }).commit();
    return prms->get_future();
  }

public:
  //! ctor & dtor
  future_client(void)  = default;
  ~future_client(void) = default;

  void
  connect(const std::string& host = "127.0.0.1", std::size_t port = 6379,
    const disconnection_handler_t& disconnection_handler = nullptr) {
    m_client.connect(host, port, disconnection_handler);
  }
  void
  disconnect() { m_client.disconnect(); }
  bool
  is_connected() { return m_client.is_connected(); }

  future
  send(const std::vector<std::string>& redis_cmd) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.send(redis_cmd, cb); });
  }

  future
  append(const std::string& key, const std::string& value) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.append(key, value, cb); });
  }
  future
  auth(const std::string& password) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.auth(password, cb); });
  }
  future
  bgrewriteaof() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.bgrewriteaof(cb); });
  }
  future
  bgsave() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.bgsave(cb); });
  }
  future
  bitcount(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.bitcount(key, cb); });
  }
  future
  bitcount(const std::string& key, int start, int end) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.bitcount(key, start, end, cb); });
  }
  // future bitfield(const std::string& key) key [get type offset] [set type offset value] [incrby type offset increment] [overflow wrap|sat|fail]
  future
  bitop(const std::string& operation, const std::string& destkey, const std::vector<std::string>& keys) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.bitop(operation, destkey, keys, cb); });
  }
  future
  bitpos(const std::string& key, int bit) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.bitpos(key, bit, cb); });
  }
  future
  bitpos(const std::string& key, int bit, int start) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.bitpos(key, bit, start, cb); });
  }
  future
  bitpos(const std::string& key, int bit, int start, int end) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.bitpos(key, bit, start, end, cb); });
  }
  future
  blpop(const std::vector<std::string>& keys, int timeout) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.blpop(keys, timeout, cb); });
  }
  future
  brpop(const std::vector<std::string>& keys, int timeout) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.brpop(keys, timeout, cb); });
  }
  future
  brpoplpush(const std::string& src, const std::string& dst, int timeout) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.brpoplpush(src, dst, timeout, cb); });
  }
  // future client_kill() [ip:port] [id client-id] [type normal|master|slave|pubsub] [addr ip:port] [skipme yes/no]
  future
  client_list() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.client_list(cb); });
  }
  future
  client_getname() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.client_getname(cb); });
  }
  future
  client_pause(int timeout) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.client_pause(timeout, cb); });
  }
  future
  client_reply(const std::string& mode) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.client_reply(mode, cb); });
  }
  future
  client_setname(const std::string& name) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.client_setname(name, cb); });
  }
  future
  cluster_addslots(const std::vector<std::string>& p_slots) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_addslots(p_slots, cb); });
  }
  future
  cluster_count_failure_reports(const std::string& node_id) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_count_failure_reports(node_id, cb); });
  }
  future
  cluster_countkeysinslot(const std::string& slot) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_countkeysinslot(slot, cb); });
  }
  future
  cluster_delslots(const std::vector<std::string>& p_slots) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_delslots(p_slots, cb); });
  }
  future
  cluster_failover() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_failover(cb); });
  }
  future
  cluster_failover(const std::string& mode) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_failover(mode, cb); });
  }
  future
  cluster_forget(const std::string& node_id) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_forget(node_id, cb); });
  }
  future
  cluster_getkeysinslot(const std::string& slot, int count) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_getkeysinslot(slot, count, cb); });
  }
  future
  cluster_info() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_info(cb); });
  }
  future
  cluster_keyslot(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_keyslot(key, cb); });
  }
  future
  cluster_meet(const std::string& ip, int port) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_meet(ip, port, cb); });
  }
  future
  cluster_nodes() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_nodes(cb); });
  }
  future
  cluster_replicate(const std::string& node_id) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_replicate(node_id, cb); });
  }
  future
  cluster_reset(const std::string& mode = "soft") {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_reset(mode, cb); });
  }
  future
  cluster_saveconfig() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_saveconfig(cb); });
  }
  future
  cluster_set_config_epoch(const std::string& epoch) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_set_config_epoch(epoch, cb); });
  }
  future
  cluster_setslot(const std::string& slot, const std::string& mode) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_setslot(slot, mode, cb); });
  }
  future
  cluster_setslot(const std::string& slot, const std::string& mode, const std::string& node_id) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_setslot(slot, mode, node_id, cb); });
  }
  future
  cluster_slaves(const std::string& node_id) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_slaves(node_id, cb); });
  }
  future
  cluster_slots() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_slots(cb); });
  }
  future
  command() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.command(cb); });
  }
  future
  command_count() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.command_count(cb); });
  }
  future
  command_getkeys() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.command_getkeys(cb); });
  }
  future
  command_info(const std::vector<std::string>& command_name) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.command_info(command_name, cb); });
  }
  future
  config_get(const std::string& param) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.config_get(param, cb); });
  }
  future
  config_rewrite() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.config_rewrite(cb); });
  }
  future
  config_set(const std::string& param, const std::string& val) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.config_set(param, val, cb); });
  }
  future
  config_resetstat() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.config_resetstat(cb); });
  }
  future
  dbsize() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.dbsize(cb); });
  }
  future
  debug_object(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.debug_object(key, cb); });
  }
  future
  debug_segfault() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.debug_segfault(cb); });
  }
  future
  decr(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.decr(key, cb); });
  }
  future
  decrby(const std::string& key, int val) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.decrby(key, val, cb); });
  }
  future
  del(const std::vector<std::string>& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.del(key, cb); });
  }
  future
  discard() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.discard(cb); });
  }
  future
  dump(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.dump(key, cb); });
  }
  future
  echo(const std::string& msg) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.echo(msg, cb); });
  }
  future
  eval(const std::string& script, int numkeys, const std::vector<std::string>& keys, const std::vector<std::string>& args) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.eval(script, numkeys, keys, args, cb); });
  }
  future
  evalsha(const std::string& sha1, int numkeys, const std::vector<std::string>& keys, const std::vector<std::string>& args) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.evalsha(sha1, numkeys, keys, args, cb); });
  }
  future
  exec() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.exec(cb); });
  }
  future
  exists(const std::vector<std::string>& keys) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.exists(keys, cb); });
  }
  future
  expire(const std::string& key, int seconds) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.expire(key, seconds, cb); });
  }
  future
  expireat(const std::string& key, int timestamp) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.expireat(key, timestamp, cb); });
  }
  future
  flushall() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.flushall(cb); });
  }
  future
  flushdb() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.flushdb(cb); });
  }
  future
  geoadd(const std::string& key, const std::vector<std::tuple<std::string, std::string, std::string>>& long_lat_memb) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.geoadd(key, long_lat_memb, cb); });
  }
  future
  geohash(const std::string& key, const std::vector<std::string>& members) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.geohash(key, members, cb); });
  }
  future
  geopos(const std::string& key, const std::vector<std::string>& members) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.geopos(key, members, cb); });
  }
  future
  geodist(const std::string& key, const std::string& member_1, const std::string& member_2, const std::string& unit = "m") {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.geodist(key, member_1, member_2, unit, cb); });
  }
  // future georadius() key longitude latitude radius m|km|ft|mi [withcoord] [withdist] [withhash] [count count] [asc|desc] [store key] [storedist key]
  // future georadiusbymember() key member radius m|km|ft|mi [withcoord] [withdist] [withhash] [count count] [asc|desc] [store key] [storedist key]
  future
  get(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.get(key, cb); });
  }
  future
  getbit(const std::string& key, int offset) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.getbit(key, offset, cb); });
  }
  future
  getrange(const std::string& key, int start, int end) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.getrange(key, start, end, cb); });
  }
  future
  getset(const std::string& key, const std::string& val) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.getset(key, val, cb); });
  }
  future
  hdel(const std::string& key, const std::vector<std::string>& fields) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hdel(key, fields, cb); });
  }
  future
  hexists(const std::string& key, const std::string& field) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hexists(key, field, cb); });
  }
  future
  hget(const std::string& key, const std::string& field) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hget(key, field, cb); });
  }
  future
  hgetall(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hgetall(key, cb); });
  }
  future
  hincrby(const std::string& key, const std::string& field, int incr) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hincrby(key, field, incr, cb); });
  }
  future
  hincrbyfloat(const std::string& key, const std::string& field, float incr) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hincrbyfloat(key, field, incr, cb); });
  }
  future
  hkeys(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hkeys(key, cb); });
  }
  future
  hlen(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hlen(key, cb); });
  }
  future
  hmget(const std::string& key, const std::vector<std::string>& fields) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hmget(key, fields, cb); });
  }
  future
  hmset(const std::string& key, const std::vector<std::pair<std::string, std::string>>& field_val) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hmset(key, field_val, cb); });
  }
  future
  hset(const std::string& key, const std::string& field, const std::string& value) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hset(key, field, value, cb); });
  }
  future
  hsetnx(const std::string& key, const std::string& field, const std::string& value) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hsetnx(key, field, value, cb); });
  }
  future
  hstrlen(const std::string& key, const std::string& field) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hstrlen(key, field, cb); });
  }
  future
  hvals(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hvals(key, cb); });
  }
  future
  incr(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.incr(key, cb); });
  }
  future
  incrby(const std::string& key, int incr) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.incrby(key, incr, cb); });
  }
  future
  incrbyfloat(const std::string& key, float incr) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.incrbyfloat(key, incr, cb); });
  }
  future
  info(const std::string& section = "default") {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.info(section, cb); });
  }
  future
  keys(const std::string& pattern) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.keys(pattern, cb); });
  }
  future
  lastsave() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.lastsave(cb); });
  }
  future
  lindex(const std::string& key, int index) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.lindex(key, index, cb); });
  }
  future
  linsert(const std::string& key, const std::string& before_after, const std::string& pivot, const std::string& value) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.linsert(key, before_after, pivot, value, cb); });
  }
  future
  llen(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.llen(key, cb); });
  }
  future
  lpop(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.lpop(key, cb); });
  }
  future
  lpush(const std::string& key, const std::vector<std::string>& values) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.lpush(key, values, cb); });
  }
  future
  lpushx(const std::string& key, const std::string& value) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.lpushx(key, value, cb); });
  }
  future
  lrange(const std::string& key, int start, int stop) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.lrange(key, start, stop, cb); });
  }
  future
  lrem(const std::string& key, int count, const std::string& value) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.lrem(key, count, value, cb); });
  }
  future
  lset(const std::string& key, int index, const std::string& value) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.lset(key, index, value, cb); });
  }
  future
  ltrim(const std::string& key, int start, int stop) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.ltrim(key, start, stop, cb); });
  }
  future
  mget(const std::vector<std::string>& keys) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.mget(keys, cb); });
  }
  future
  migrate(const std::string& host, int port, const std::string& key, const std::string& dest_db, int timeout, bool copy = false, bool replace = false, const std::vector<std::string>& keys = {}) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.migrate(host, port, key, dest_db, timeout, copy, replace, keys, cb); });
  }
  future
  monitor() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.monitor(cb); });
  }
  future
  move(const std::string& key, const std::string& db) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.move(key, db, cb); });
  }
  future
  mset(const std::vector<std::pair<std::string, std::string>>& key_vals) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.mset(key_vals, cb); });
  }
  future
  msetnx(const std::vector<std::pair<std::string, std::string>>& key_vals) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.msetnx(key_vals, cb); });
  }
  future
  multi() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.multi(cb); });
  }
  future
  object(const std::string& subcommand, const std::vector<std::string>& args) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.object(subcommand, args, cb); });
  }
  future
  persist(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.persist(key, cb); });
  }
  future
  pexpire(const std::string& key, int milliseconds) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.pexpire(key, milliseconds, cb); });
  }
  future
  pexpireat(const std::string& key, int milliseconds_timestamp) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.pexpireat(key, milliseconds_timestamp, cb); });
  }
  future
  pfadd(const std::string& key, const std::vector<std::string>& elements) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.pfadd(key, elements, cb); });
  }
  future
  pfcount(const std::vector<std::string>& keys) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.pfcount(keys, cb); });
  }
  future
  pfmerge(const std::string& destkey, const std::vector<std::string>& sourcekeys) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.pfmerge(destkey, sourcekeys, cb); });
  }
  future
  ping() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.ping(cb); });
  }
  future
  ping(const std::string& message) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.ping(message, cb); });
  }
  future
  psetex(const std::string& key, int milliseconds, const std::string& val) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.psetex(key, milliseconds, val, cb); });
  }
  future
  publish(const std::string& channel, const std::string& message) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.publish(channel, message, cb); });
  }
  future
  pubsub(const std::string& subcommand, const std::vector<std::string>& args) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.pubsub(subcommand, args, cb); });
  }
  future
  pttl(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.pttl(key, cb); });
  }
  future
  quit() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.quit(cb); });
  }
  future
  randomkey() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.randomkey(cb); });
  }
  future
  readonly() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.readonly(cb); });
  }
  future
  readwrite() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.readwrite(cb); });
  }
  future
  rename(const std::string& key, const std::string& newkey) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.rename(key, newkey, cb); });
  }
  future
  renamenx(const std::string& key, const std::string& newkey) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.renamenx(key, newkey, cb); });
  }
  future
  restore(const std::string& key, int ttl, const std::string& serialized_value) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.restore(key, ttl, serialized_value, cb); });
  }
  future
  restore(const std::string& key, int ttl, const std::string& serialized_value, const std::string& replace) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.restore(key, ttl, serialized_value, replace, cb); });
  }
  future
  role() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.role(cb); });
  }
  future
  rpop(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.rpop(key, cb); });
  }
  future
  rpoplpush(const std::string& src, const std::string& dst) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.rpoplpush(src, dst, cb); });
  }
  future
  rpush(const std::string& key, const std::vector<std::string>& values) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.rpush(key, values, cb); });
  }
  future
  rpushx(const std::string& key, const std::string& value) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.rpushx(key, value, cb); });
  }
  future
  sadd(const std::string& key, const std::vector<std::string>& members) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.sadd(key, members, cb); });
  }
  future
  save() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.save(cb); });
  }
  future
  scard(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.scard(key, cb); });
  }
  future
  script_debug(const std::string& mode) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.script_debug(mode, cb); });
  }
  future
  script_exists(const std::vector<std::string>& scripts) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.script_exists(scripts, cb); });
  }
  future
  script_flush() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.script_flush(cb); });
  }
  future
  script_kill() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.script_kill(cb); });
  }
  future
  script_load(const std::string& script) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.script_load(script, cb); });
  }
  future
  sdiff(const std::vector<std::string>& keys) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.sdiff(keys, cb); });
  }
  future
  sdiffstore(const std::string& dst, const std::vector<std::string>& keys) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.sdiffstore(dst, keys, cb); });
  }
  future
  select(int index) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.select(index, cb); });
  }
  future
  set(const std::string& key, const std::string& value) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.set(key, value, cb); });
  }
  future
  set_advanced(const std::string& key, const std::string& value, bool ex = false, int ex_sec = 0, bool px = false, int px_milli = 0, bool nx = false, bool xx = false) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.set_advanced(key, value, ex, ex_sec, px, px_milli, nx, xx, cb); });
  }
  future
  setbit_(const std::string& key, int offset, const std::string& value) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.setbit_(key, offset, value, cb); });
  }
  future
  setex(const std::string& key, int seconds, const std::string& value) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.setex(key, seconds, value, cb); });
  }
  future
  setnx(const std::string& key, const std::string& value) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.setnx(key, value, cb); });
  }
  future
  setrange(const std::string& key, int offset, const std::string& value) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.setrange(key, offset, value, cb); });
  }
  future
  shutdown() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.shutdown(cb); });
  }
  future
  shutdown(const std::string& save) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.shutdown(save, cb); });
  }
  future
  sinter(const std::vector<std::string>& keys) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.sinter(keys, cb); });
  }
  future
  sinterstore(const std::string& dst, const std::vector<std::string>& keys) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.sinterstore(dst, keys, cb); });
  }
  future
  sismember(const std::string& key, const std::string& member) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.sismember(key, member, cb); });
  }
  future
  slaveof(const std::string& host, int port) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.slaveof(host, port, cb); });
  }
  future
  slowlog(const std::string& subcommand) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.slowlog(subcommand, cb); });
  }
  future
  slowlog(const std::string& subcommand, const std::string& argument) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.slowlog(subcommand, argument, cb); });
  }
  future
  smembers(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.smembers(key, cb); });
  }
  future
  smove(const std::string& src, const std::string& dst, const std::string& member) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.smove(src, dst, member, cb); });
  }
  // future sort() key [by pattern] [limit offset count] [get pattern [get pattern ...]] [asc|desc] [alpha] [store destination]
  future
  spop(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.spop(key, cb); });
  }
  future
  spop(const std::string& key, int count) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.spop(key, count, cb); });
  }
  future
  srandmember(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.srandmember(key, cb); });
  }
  future
  srandmember(const std::string& key, int count) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.srandmember(key, count, cb); });
  }
  future
  srem(const std::string& key, const std::vector<std::string>& members) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.srem(key, members, cb); });
  }
  future
  strlen(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.strlen(key, cb); });
  }
  future
  sunion(const std::vector<std::string>& keys) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.sunion(keys, cb); });
  }
  future
  sunionstore(const std::string& dst, const std::vector<std::string>& keys) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.sunionstore(dst, keys, cb); });
  }
  future
  sync() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.sync(cb); });
  }
  future
  time() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.time(cb); });
  }
  future
  ttl(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.ttl(key, cb); });
  }
  future
  type(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.type(key, cb); });
  }
  future
  unwatch() {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.unwatch(cb); });
  }
  future
  wait(int numslaves, int timeout) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.wait(numslaves, timeout, cb); });
  }
  future
  watch(const std::vector<std::string>& keys) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.watch(keys, cb); });
  }
  // future zadd() key [nx|xx] [ch] [incr] score member [score member ...]
  future
  zcard(const std::string& key) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zcard(key, cb); });
  }
  future
  zcount(const std::string& key, int min, int max) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zcount(key, min, max, cb); });
  }
  future
  zcount(const std::string& key, double min, double max) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zcount(key, min, max, cb); });
  }
  future
  zcount(const std::string& key, const std::string& min, const std::string& max) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zcount(key, min, max, cb); });
  }
  future
  zincrby(const std::string& key, int incr, const std::string& member) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zincrby(key, incr, member, cb); });
  }
  future
  zincrby(const std::string& key, double incr, const std::string& member) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zincrby(key, incr, member, cb); });
  }
  future
  zincrby(const std::string& key, const std::string& incr, const std::string& member) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zincrby(key, incr, member, cb); });
  }
  // future zinterstore() destination numkeys key [key ...] [weights weight [weight ...]] [aggregate sum|min|max]
  future
  zlexcount(const std::string& key, int min, int max) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zlexcount(key, min, max, cb); });
  }
  future
  zlexcount(const std::string& key, double min, double max) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zlexcount(key, min, max, cb); });
  }
  future
  zlexcount(const std::string& key, const std::string& min, const std::string& max) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zlexcount(key, min, max, cb); });
  }
  future
  zrange(const std::string& key, int start, int stop, bool withscores = false) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zrange(key, start, stop, withscores, cb); });
  }
  future
  zrange(const std::string& key, double start, double stop, bool withscores = false) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zrange(key, start, stop, withscores, cb); });
  }
  future
  zrange(const std::string& key, const std::string& start, const std::string& stop, bool withscores = false) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zrange(key, start, stop, withscores, cb); });
  }
  // future zrangebylex() key min max [limit offset count]
  // future zrevrangebylex() key max min [limit offset count]
  // future zrangebyscore() key min max [withscores] [limit offset count]
  future
  zrank(const std::string& key, const std::string& member) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zrank(key, member, cb); });
  }
  future
  zrem(const std::string& key, const std::vector<std::string>& members) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zrem(key, members, cb); });
  }
  future
  zremrangebylex(const std::string& key, int min, int max) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zremrangebylex(key, min, max, cb); });
  }
  future
  zremrangebylex(const std::string& key, double min, double max) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zremrangebylex(key, min, max, cb); });
  }
  future
  zremrangebylex(const std::string& key, const std::string& min, const std::string& max) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zremrangebylex(key, min, max, cb); });
  }
  future
  zremrangebyrank(const std::string& key, int start, int stop) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zremrangebyrank(key, start, stop, cb); });
  }
  future
  zremrangebyrank(const std::string& key, double start, double stop) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zremrangebyrank(key, start, stop, cb); });
  }
  future
  zremrangebyrank(const std::string& key, const std::string& start, const std::string& stop) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zremrangebyrank(key, start, stop, cb); });
  }
  future
  zremrangebyscore(const std::string& key, int min, int max) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zremrangebyscore(key, min, max, cb); });
  }
  future
  zremrangebyscore(const std::string& key, double min, double max) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zremrangebyscore(key, min, max, cb); });
  }
  future
  zremrangebyscore(const std::string& key, const std::string& min, const std::string& max) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zremrangebyscore(key, min, max, cb); });
  }
  future
  zrevrange(const std::string& key, int start, int stop, bool withscores = false) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zrevrange(key, start, stop, withscores, cb); });
  }
  future
  zrevrange(const std::string& key, double start, double stop, bool withscores = false) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zrevrange(key, start, stop, withscores, cb); });
  }
  future
  zrevrange(const std::string& key, const std::string& start, const std::string& stop, bool withscores = false) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zrevrange(key, start, stop, withscores, cb); });
  }
  // future zrevrangebyscore() key max min [withscores] [limit offset count]
  future
  zrevrank(const std::string& key, const std::string& member) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zrevrank(key, member, cb); });
  }
  future
  zscore(const std::string& key, const std::string& member) {
    return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zscore(key, member, cb); });
  }
  // future zunionstore() destination numkeys key [key ...] [weights weight [weight ...]] [aggregate sum|min|max]
  // future scan() cursor [match pattern] [count count]
  // future sscan() key cursor [match pattern] [count count]
  // future hscan() key cursor [match pattern] [count count]
  // future zscan() key cursor [match pattern] [count count]

private:
  redis_client m_client;
};

} //! cpp_redis
