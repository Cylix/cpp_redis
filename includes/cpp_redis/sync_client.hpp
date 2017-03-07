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

#include <cpp_redis/future_client.hpp>
#include <cpp_redis/logger.hpp>

namespace cpp_redis {

class sync_client {

public:
  using result_type             = reply;
  using disconnection_handler_t = redis_client::disconnection_handler_t;

public:
  //! ctor & dtor
  sync_client(void)  = default;
  ~sync_client(void) = default;

  void
  connect(const std::string& host = "127.0.0.1", std::size_t port = 6379,
    const disconnection_handler_t& disconnection_handler = nullptr) {
    m_client.connect(host, port, disconnection_handler);
  }
  void
  disconnect() { m_client.disconnect(); }
  bool
  is_connected() { return m_client.is_connected(); }

  reply
  send(const std::vector<std::string>& redis_cmd) {
    return m_client.send(redis_cmd).get();
  }

  reply
  append(const std::string& key, const std::string& value) {
    return m_client.append(key, value).get();
  }
  reply
  auth(const std::string& password) {
    return m_client.auth(password).get();
  }
  reply
  bgrewriteaof() {
    return m_client.bgrewriteaof().get();
  }
  reply
  bgsave() {
    return m_client.bgsave().get();
  }
  reply
  bitcount(const std::string& key) {
    return m_client.bitcount(key).get();
  }
  reply
  bitcount(const std::string& key, int start, int end) {
    return m_client.bitcount(key, start, end).get();
  }
  // reply bitfield(const std::string& key) key [get type offset] [set type offset value] [incrby type offset increment] [overflow wrap|sat|fail]
  reply
  bitop(const std::string& operation, const std::string& destkey, const std::vector<std::string>& keys) {
    return m_client.bitop(operation, destkey, keys).get();
  }
  reply
  bitpos(const std::string& key, int bit) {
    return m_client.bitpos(key, bit).get();
  }
  reply
  bitpos(const std::string& key, int bit, int start) {
    return m_client.bitpos(key, bit, start).get();
  }
  reply
  bitpos(const std::string& key, int bit, int start, int end) {
    return m_client.bitpos(key, bit, start, end).get();
  }
  reply
  blpop(const std::vector<std::string>& keys, int timeout) {
    return m_client.blpop(keys, timeout).get();
  }
  reply
  brpop(const std::vector<std::string>& keys, int timeout) {
    return m_client.brpop(keys, timeout).get();
  }
  reply
  brpoplpush(const std::string& src, const std::string& dst, int timeout) {
    return m_client.brpoplpush(src, dst, timeout).get();
  }
  // reply client_kill() [ip:port] [id client-id] [type normal|master|slave|pubsub] [addr ip:port] [skipme yes/no]
  reply
  client_list() {
    return m_client.client_list().get();
  }
  reply
  client_getname() {
    return m_client.client_getname().get();
  }
  reply
  client_pause(int timeout) {
    return m_client.client_pause(timeout).get();
  }
  reply
  client_reply(const std::string& mode) {
    return m_client.client_reply(mode).get();
  }
  reply
  client_setname(const std::string& name) {
    return m_client.client_setname(name).get();
  }
  reply
  cluster_addslots(const std::vector<std::string>& p_slots) {
    return m_client.cluster_addslots(p_slots).get();
  }
  reply
  cluster_count_failure_reports(const std::string& node_id) {
    return m_client.cluster_count_failure_reports(node_id).get();
  }
  reply
  cluster_countkeysinslot(const std::string& slot) {
    return m_client.cluster_countkeysinslot(slot).get();
  }
  reply
  cluster_delslots(const std::vector<std::string>& p_slots) {
    return m_client.cluster_delslots(p_slots).get();
  }
  reply
  cluster_failover() {
    return m_client.cluster_failover().get();
  }
  reply
  cluster_failover(const std::string& mode) {
    return m_client.cluster_failover(mode).get();
  }
  reply
  cluster_forget(const std::string& node_id) {
    return m_client.cluster_forget(node_id).get();
  }
  reply
  cluster_getkeysinslot(const std::string& slot, int count) {
    return m_client.cluster_getkeysinslot(slot, count).get();
  }
  reply
  cluster_info() {
    return m_client.cluster_info().get();
  }
  reply
  cluster_keyslot(const std::string& key) {
    return m_client.cluster_keyslot(key).get();
  }
  reply
  cluster_meet(const std::string& ip, int port) {
    return m_client.cluster_meet(ip, port).get();
  }
  reply
  cluster_nodes() {
    return m_client.cluster_nodes().get();
  }
  reply
  cluster_replicate(const std::string& node_id) {
    return m_client.cluster_replicate(node_id).get();
  }
  reply
  cluster_reset(const std::string& mode = "soft") {
    return m_client.cluster_reset(mode).get();
  }
  reply
  cluster_saveconfig() {
    return m_client.cluster_saveconfig().get();
  }
  reply
  cluster_set_config_epoch(const std::string& epoch) {
    return m_client.cluster_set_config_epoch(epoch).get();
  }
  reply
  cluster_setslot(const std::string& slot, const std::string& mode) {
    return m_client.cluster_setslot(slot, mode).get();
  }
  reply
  cluster_setslot(const std::string& slot, const std::string& mode, const std::string& node_id) {
    return m_client.cluster_setslot(slot, mode, node_id).get();
  }
  reply
  cluster_slaves(const std::string& node_id) {
    return m_client.cluster_slaves(node_id).get();
  }
  reply
  cluster_slots() {
    return m_client.cluster_slots().get();
  }
  reply
  command() {
    return m_client.command().get();
  }
  reply
  command_count() {
    return m_client.command_count().get();
  }
  reply
  command_getkeys() {
    return m_client.command_getkeys().get();
  }
  reply
  command_info(const std::vector<std::string>& command_name) {
    return m_client.command_info(command_name).get();
  }
  reply
  config_get(const std::string& param) {
    return m_client.config_get(param).get();
  }
  reply
  config_rewrite() {
    return m_client.config_rewrite().get();
  }
  reply
  config_set(const std::string& param, const std::string& val) {
    return m_client.config_set(param, val).get();
  }
  reply
  config_resetstat() {
    return m_client.config_resetstat().get();
  }
  reply
  dbsize() {
    return m_client.dbsize().get();
  }
  reply
  debug_object(const std::string& key) {
    return m_client.debug_object(key).get();
  }
  reply
  debug_segfault() {
    return m_client.debug_segfault().get();
  }
  reply
  decr(const std::string& key) {
    return m_client.decr(key).get();
  }
  reply
  decrby(const std::string& key, int val) {
    return m_client.decrby(key, val).get();
  }
  reply
  del(const std::vector<std::string>& key) {
    return m_client.del(key).get();
  }
  reply
  discard() {
    return m_client.discard().get();
  }
  reply
  dump(const std::string& key) {
    return m_client.dump(key).get();
  }
  reply
  echo(const std::string& msg) {
    return m_client.echo(msg).get();
  }
  reply
  eval(const std::string& script, int numkeys, const std::vector<std::string>& keys, const std::vector<std::string>& args) {
    return m_client.eval(script, numkeys, keys, args).get();
  }
  reply
  evalsha(const std::string& sha1, int numkeys, const std::vector<std::string>& keys, const std::vector<std::string>& args) {
    return m_client.evalsha(sha1, numkeys, keys, args).get();
  }
  reply
  exec() {
    return m_client.exec().get();
  }
  reply
  exists(const std::vector<std::string>& keys) {
    return m_client.exists(keys).get();
  }
  reply
  expire(const std::string& key, int seconds) {
    return m_client.expire(key, seconds).get();
  }
  reply
  expireat(const std::string& key, int timestamp) {
    return m_client.expireat(key, timestamp).get();
  }
  reply
  flushall() {
    return m_client.flushall().get();
  }
  reply
  flushdb() {
    return m_client.flushdb().get();
  }
  reply
  geoadd(const std::string& key, const std::vector<std::tuple<std::string, std::string, std::string>>& long_lat_memb) {
    return m_client.geoadd(key, long_lat_memb).get();
  }
  reply
  geohash(const std::string& key, const std::vector<std::string>& members) {
    return m_client.geohash(key, members).get();
  }
  reply
  geopos(const std::string& key, const std::vector<std::string>& members) {
    return m_client.geopos(key, members).get();
  }
  reply
  geodist(const std::string& key, const std::string& member_1, const std::string& member_2, const std::string& unit = "m") {
    return m_client.geodist(key, member_1, member_2, unit).get();
  }
  // reply georadius() key longitude latitude radius m|km|ft|mi [withcoord] [withdist] [withhash] [count count] [asc|desc] [store key] [storedist key]
  // reply georadiusbymember() key member radius m|km|ft|mi [withcoord] [withdist] [withhash] [count count] [asc|desc] [store key] [storedist key]
  reply
  get(const std::string& key) {
    return m_client.get(key).get();
  }
  reply
  getbit(const std::string& key, int offset) {
    return m_client.getbit(key, offset).get();
  }
  reply
  getrange(const std::string& key, int start, int end) {
    return m_client.getrange(key, start, end).get();
  }
  reply
  getset(const std::string& key, const std::string& val) {
    return m_client.getset(key, val).get();
  }
  reply
  hdel(const std::string& key, const std::vector<std::string>& fields) {
    return m_client.hdel(key, fields).get();
  }
  reply
  hexists(const std::string& key, const std::string& field) {
    return m_client.hexists(key, field).get();
  }
  reply
  hget(const std::string& key, const std::string& field) {
    return m_client.hget(key, field).get();
  }
  reply
  hgetall(const std::string& key) {
    return m_client.hgetall(key).get();
  }
  reply
  hincrby(const std::string& key, const std::string& field, int incr) {
    return m_client.hincrby(key, field, incr).get();
  }
  reply
  hincrbyfloat(const std::string& key, const std::string& field, float incr) {
    return m_client.hincrbyfloat(key, field, incr).get();
  }
  reply
  hkeys(const std::string& key) {
    return m_client.hkeys(key).get();
  }
  reply
  hlen(const std::string& key) {
    return m_client.hlen(key).get();
  }
  reply
  hmget(const std::string& key, const std::vector<std::string>& fields) {
    return m_client.hmget(key, fields).get();
  }
  reply
  hmset(const std::string& key, const std::vector<std::pair<std::string, std::string>>& field_val) {
    return m_client.hmset(key, field_val).get();
  }
  reply
  hset(const std::string& key, const std::string& field, const std::string& value) {
    return m_client.hset(key, field, value).get();
  }
  reply
  hsetnx(const std::string& key, const std::string& field, const std::string& value) {
    return m_client.hsetnx(key, field, value).get();
  }
  reply
  hstrlen(const std::string& key, const std::string& field) {
    return m_client.hstrlen(key, field).get();
  }
  reply
  hvals(const std::string& key) {
    return m_client.hvals(key).get();
  }
  reply
  incr(const std::string& key) {
    return m_client.incr(key).get();
  }
  reply
  incrby(const std::string& key, int incr) {
    return m_client.incrby(key, incr).get();
  }
  reply
  incrbyfloat(const std::string& key, float incr) {
    return m_client.incrbyfloat(key, incr).get();
  }
  reply
  info(const std::string& section = "default") {
    return m_client.info(section).get();
  }
  reply
  keys(const std::string& pattern) {
    return m_client.keys(pattern).get();
  }
  reply
  lastsave() {
    return m_client.lastsave().get();
  }
  reply
  lindex(const std::string& key, int index) {
    return m_client.lindex(key, index).get();
  }
  reply
  linsert(const std::string& key, const std::string& before_after, const std::string& pivot, const std::string& value) {
    return m_client.linsert(key, before_after, pivot, value).get();
  }
  reply
  llen(const std::string& key) {
    return m_client.llen(key).get();
  }
  reply
  lpop(const std::string& key) {
    return m_client.lpop(key).get();
  }
  reply
  lpush(const std::string& key, const std::vector<std::string>& values) {
    return m_client.lpush(key, values).get();
  }
  reply
  lpushx(const std::string& key, const std::string& value) {
    return m_client.lpushx(key, value).get();
  }
  reply
  lrange(const std::string& key, int start, int stop) {
    return m_client.lrange(key, start, stop).get();
  }
  reply
  lrem(const std::string& key, int count, const std::string& value) {
    return m_client.lrem(key, count, value).get();
  }
  reply
  lset(const std::string& key, int index, const std::string& value) {
    return m_client.lset(key, index, value).get();
  }
  reply
  ltrim(const std::string& key, int start, int stop) {
    return m_client.ltrim(key, start, stop).get();
  }
  reply
  mget(const std::vector<std::string>& keys) {
    return m_client.mget(keys).get();
  }
  reply
  migrate(const std::string& host, int port, const std::string& key, const std::string& dest_db, int timeout, bool copy = false, bool replace = false, const std::vector<std::string>& keys = {}) {
    return m_client.migrate(host, port, key, dest_db, timeout, copy, replace, keys).get();
  }
  reply
  monitor() {
    return m_client.monitor().get();
  }
  reply
  move(const std::string& key, const std::string& db) {
    return m_client.move(key, db).get();
  }
  reply
  mset(const std::vector<std::pair<std::string, std::string>>& key_vals) {
    return m_client.mset(key_vals).get();
  }
  reply
  msetnx(const std::vector<std::pair<std::string, std::string>>& key_vals) {
    return m_client.msetnx(key_vals).get();
  }
  reply
  multi() {
    return m_client.multi().get();
  }
  reply
  object(const std::string& subcommand, const std::vector<std::string>& args) {
    return m_client.object(subcommand, args).get();
  }
  reply
  persist(const std::string& key) {
    return m_client.persist(key).get();
  }
  reply
  pexpire(const std::string& key, int milliseconds) {
    return m_client.pexpire(key, milliseconds).get();
  }
  reply
  pexpireat(const std::string& key, int milliseconds_timestamp) {
    return m_client.pexpireat(key, milliseconds_timestamp).get();
  }
  reply
  pfadd(const std::string& key, const std::vector<std::string>& elements) {
    return m_client.pfadd(key, elements).get();
  }
  reply
  pfcount(const std::vector<std::string>& keys) {
    return m_client.pfcount(keys).get();
  }
  reply
  pfmerge(const std::string& destkey, const std::vector<std::string>& sourcekeys) {
    return m_client.pfmerge(destkey, sourcekeys).get();
  }
  reply
  ping() {
    return m_client.ping().get();
  }
  reply
  ping(const std::string& message) {
    return m_client.ping(message).get();
  }
  reply
  psetex(const std::string& key, int milliseconds, const std::string& val) {
    return m_client.psetex(key, milliseconds, val).get();
  }
  reply
  publish(const std::string& channel, const std::string& message) {
    return m_client.publish(channel, message).get();
  }
  reply
  pubsub(const std::string& subcommand, const std::vector<std::string>& args) {
    return m_client.pubsub(subcommand, args).get();
  }
  reply
  pttl(const std::string& key) {
    return m_client.pttl(key).get();
  }
  reply
  quit() {
    return m_client.quit().get();
  }
  reply
  randomkey() {
    return m_client.randomkey().get();
  }
  reply
  readonly() {
    return m_client.readonly().get();
  }
  reply
  readwrite() {
    return m_client.readwrite().get();
  }
  reply
  rename(const std::string& key, const std::string& newkey) {
    return m_client.rename(key, newkey).get();
  }
  reply
  renamenx(const std::string& key, const std::string& newkey) {
    return m_client.renamenx(key, newkey).get();
  }
  reply
  restore(const std::string& key, int ttl, const std::string& serialized_value) {
    return m_client.restore(key, ttl, serialized_value).get();
  }
  reply
  restore(const std::string& key, int ttl, const std::string& serialized_value, const std::string& replace) {
    return m_client.restore(key, ttl, serialized_value, replace).get();
  }
  reply
  role() {
    return m_client.role().get();
  }
  reply
  rpop(const std::string& key) {
    return m_client.rpop(key).get();
  }
  reply
  rpoplpush(const std::string& src, const std::string& dest) {
    return m_client.rpoplpush(src, dest).get();
  }
  reply
  rpush(const std::string& key, const std::vector<std::string>& values) {
    return m_client.rpush(key, values).get();
  }
  reply
  rpushx(const std::string& key, const std::string& value) {
    return m_client.rpushx(key, value).get();
  }
  reply
  sadd(const std::string& key, const std::vector<std::string>& members) {
    return m_client.sadd(key, members).get();
  }
  reply
  save() {
    return m_client.save().get();
  }
  reply
  scard(const std::string& key) {
    return m_client.scard(key).get();
  }
  reply
  script_debug(const std::string& mode) {
    return m_client.script_debug(mode).get();
  }
  reply
  script_exists(const std::vector<std::string>& scripts) {
    return m_client.script_exists(scripts).get();
  }
  reply
  script_flush() {
    return m_client.script_flush().get();
  }
  reply
  script_kill() {
    return m_client.script_kill().get();
  }
  reply
  script_load(const std::string& script) {
    return m_client.script_load(script).get();
  }
  reply
  sdiff(const std::vector<std::string>& keys) {
    return m_client.sdiff(keys).get();
  }
  reply
  sdiffstore(const std::string& dest, const std::vector<std::string>& keys) {
    return m_client.sdiffstore(dest, keys).get();
  }
  reply
  select(int index) {
    return m_client.select(index).get();
  }
  reply
  set(const std::string& key, const std::string& value) {
    return m_client.set(key, value).get();
  }
  reply
  set_advanced(const std::string& key, const std::string& value, bool ex = false, int ex_sec = 0, bool px = false, int px_milli = 0, bool nx = false, bool xx = false) {
    return m_client.set_advanced(key, value, ex, ex_sec, px, px_milli, nx, xx).get();
  }
  reply
  setbit_(const std::string& key, int offset, const std::string& value) {
    return m_client.setbit_(key, offset, value).get();
  }
  reply
  setex(const std::string& key, int seconds, const std::string& value) {
    return m_client.setex(key, seconds, value).get();
  }
  reply
  setnx(const std::string& key, const std::string& value) {
    return m_client.setnx(key, value).get();
  }
  reply
  setrange(const std::string& key, int offset, const std::string& value) {
    return m_client.setrange(key, offset, value).get();
  }
  reply
  shutdown() {
    return m_client.shutdown().get();
  }
  reply
  shutdown(const std::string& save) {
    return m_client.shutdown(save).get();
  }
  reply
  sinter(const std::vector<std::string>& keys) {
    return m_client.sinter(keys).get();
  }
  reply
  sinterstore(const std::string& dest, const std::vector<std::string>& keys) {
    return m_client.sinterstore(dest, keys).get();
  }
  reply
  sismember(const std::string& key, const std::string& member) {
    return m_client.sismember(key, member).get();
  }
  reply
  slaveof(const std::string& host, int port) {
    return m_client.slaveof(host, port).get();
  }
  reply
  slowlog(const std::string& subcommand) {
    return m_client.slowlog(subcommand).get();
  }
  reply
  slowlog(const std::string& subcommand, const std::string& argument) {
    return m_client.slowlog(subcommand, argument).get();
  }
  reply
  smembers(const std::string& key) {
    return m_client.smembers(key).get();
  }
  reply
  smove(const std::string& src, const std::string& dest, const std::string& member) {
    return m_client.smove(src, dest, member).get();
  }
  // reply sort() key [by pattern] [limit offset count] [get pattern [get pattern ...]] [asc|desc] [alpha] [store dest]
  reply
  spop(const std::string& key) {
    return m_client.spop(key).get();
  }
  reply
  spop(const std::string& key, int count) {
    return m_client.spop(key, count).get();
  }
  reply
  srandmember(const std::string& key) {
    return m_client.srandmember(key).get();
  }
  reply
  srandmember(const std::string& key, int count) {
    return m_client.srandmember(key, count).get();
  }
  reply
  srem(const std::string& key, const std::vector<std::string>& members) {
    return m_client.srem(key, members).get();
  }
  reply
  strlen(const std::string& key) {
    return m_client.strlen(key).get();
  }
  reply
  sunion(const std::vector<std::string>& keys) {
    return m_client.sunion(keys).get();
  }
  reply
  sunionstore(const std::string& dest, const std::vector<std::string>& keys) {
    return m_client.sunionstore(dest, keys).get();
  }
  reply
  sync() {
    return m_client.sync().get();
  }
  reply
  time() {
    return m_client.time().get();
  }
  reply
  ttl(const std::string& key) {
    return m_client.ttl(key).get();
  }
  reply
  type(const std::string& key) {
    return m_client.type(key).get();
  }
  reply
  unwatch() {
    return m_client.unwatch().get();
  }
  reply
  wait(int numslaves, int timeout) {
    return m_client.wait(numslaves, timeout).get();
  }
  reply
  watch(const std::vector<std::string>& keys) {
    return m_client.watch(keys).get();
  }
  // reply zadd() key [nx|xx] [ch] [incr] score member [score member ...]
  reply
  zcard(const std::string& key) {
    return m_client.zcard(key).get();
  }
  reply
  zcount(const std::string& key, int min, int max) {
    return m_client.zcount(key, min, max).get();
  }
  reply
  zcount(const std::string& key, double min, double max) {
    return m_client.zcount(key, min, max).get();
  }
  reply
  zcount(const std::string& key, const std::string& min, const std::string& max) {
    return m_client.zcount(key, min, max).get();
  }
  reply
  zincrby(const std::string& key, int incr, const std::string& member) {
    return m_client.zincrby(key, incr, member).get();
  }
  reply
  zincrby(const std::string& key, double incr, const std::string& member) {
    return m_client.zincrby(key, incr, member).get();
  }
  reply
  zincrby(const std::string& key, const std::string& incr, const std::string& member) {
    return m_client.zincrby(key, incr, member).get();
  }
  // reply zinterstore() dest numkeys key [key ...] [weights weight [weight ...]] [aggregate sum|min|max]
  reply
  zlexcount(const std::string& key, int min, int max) {
    return m_client.zlexcount(key, min, max).get();
  }
  reply
  zlexcount(const std::string& key, double min, double max) {
    return m_client.zlexcount(key, min, max).get();
  }
  reply
  zlexcount(const std::string& key, const std::string& min, const std::string& max) {
    return m_client.zlexcount(key, min, max).get();
  }
  reply
  zrange(const std::string& key, int start, int stop, bool withscores = false) {
    return m_client.zrange(key, start, stop, withscores).get();
  }
  reply
  zrange(const std::string& key, double start, double stop, bool withscores = false) {
    return m_client.zrange(key, start, stop, withscores).get();
  }
  reply
  zrange(const std::string& key, const std::string& start, const std::string& stop, bool withscores = false) {
    return m_client.zrange(key, start, stop, withscores).get();
  }
  // reply zrangebylex() key min max [limit offset count]
  // reply zrevrangebylex() key max min [limit offset count]
  // reply zrangebyscore() key min max [withscores] [limit offset count]
  reply
  zrank(const std::string& key, const std::string& member) {
    return m_client.zrank(key, member).get();
  }
  reply
  zrem(const std::string& key, const std::vector<std::string>& members) {
    return m_client.zrem(key, members).get();
  }
  reply
  zremrangebylex(const std::string& key, int min, int max) {
    return m_client.zremrangebylex(key, min, max).get();
  }
  reply
  zremrangebylex(const std::string& key, double min, double max) {
    return m_client.zremrangebylex(key, min, max).get();
  }
  reply
  zremrangebylex(const std::string& key, const std::string& min, const std::string& max) {
    return m_client.zremrangebylex(key, min, max).get();
  }
  reply
  zremrangebyrank(const std::string& key, int start, int stop) {
    return m_client.zremrangebyrank(key, start, stop).get();
  }
  reply
  zremrangebyrank(const std::string& key, double start, double stop) {
    return m_client.zremrangebyrank(key, start, stop).get();
  }
  reply
  zremrangebyrank(const std::string& key, const std::string& start, const std::string& stop) {
    return m_client.zremrangebyrank(key, start, stop).get();
  }
  reply
  zremrangebyscore(const std::string& key, int min, int max) {
    return m_client.zremrangebyscore(key, min, max).get();
  }
  reply
  zremrangebyscore(const std::string& key, double min, double max) {
    return m_client.zremrangebyscore(key, min, max).get();
  }
  reply
  zremrangebyscore(const std::string& key, const std::string& min, const std::string& max) {
    return m_client.zremrangebyscore(key, min, max).get();
  }
  reply
  zrevrange(const std::string& key, int start, int stop, bool withscores = false) {
    return m_client.zrevrange(key, start, stop, withscores).get();
  }
  reply
  zrevrange(const std::string& key, double start, double stop, bool withscores = false) {
    return m_client.zrevrange(key, start, stop, withscores).get();
  }
  reply
  zrevrange(const std::string& key, const std::string& start, const std::string& stop, bool withscores = false) {
    return m_client.zrevrange(key, start, stop, withscores).get();
  }
  // reply zrevrangebyscore() key max min [withscores] [limit offset count]
  reply
  zrevrank(const std::string& key, const std::string& member) {
    return m_client.zrevrank(key, member).get();
  }
  reply
  zscore(const std::string& key, const std::string& member) {
    return m_client.zscore(key, member).get();
  }
  // reply zunionstore() dest numkeys key [key ...] [weights weight [weight ...]] [aggregate sum|min|max]
  // reply scan() cursor [match pattern] [count count]
  // reply sscan() key cursor [match pattern] [count count]
  // reply hscan() key cursor [match pattern] [count count]
  // reply zscan() key cursor [match pattern] [count count]

private:
  future_client m_client;
};

} //! cpp_redis
