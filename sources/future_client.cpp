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

#include <cpp_redis/future_client.hpp>

namespace cpp_redis {

future_client::future_client(const std::shared_ptr<network::tcp_client_iface>& tcp_client)
: m_client(tcp_client) {}

future_client::future
future_client::exec_cmd(call_t f) {
  auto prms = std::make_shared<promise>();

  f([prms](reply& reply) {
    prms->set_value(reply);
  }).commit();

  return prms->get_future();
}

void
future_client::connect(const std::string& host, std::size_t port, const disconnection_handler_t& disconnection_handler) {
  m_client.connect(host, port, disconnection_handler);
}

void
future_client::disconnect(bool wait_for_removal) { m_client.disconnect(wait_for_removal); }

bool
future_client::is_connected(void) { return m_client.is_connected(); }

future_client&
future_client::commit(void) {
  m_client.commit();
  return *this;
}

future_client&
future_client::sync_commit(void) {
  m_client.sync_commit();
  return *this;
}

future_client::future
future_client::send(const std::vector<std::string>& redis_cmd) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.send(redis_cmd, cb); });
}

future_client::future
future_client::append(const std::string& key, const std::string& value) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.append(key, value, cb); });
}

future_client::future
future_client::auth(const std::string& password) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.auth(password, cb); });
}

future_client::future
future_client::bgrewriteaof() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.bgrewriteaof(cb); });
}

future_client::future
future_client::bgsave() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.bgsave(cb); });
}

future_client::future
future_client::bitcount(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.bitcount(key, cb); });
}

future_client::future
future_client::bitcount(const std::string& key, int start, int end) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.bitcount(key, start, end, cb); });
}

future_client::future
future_client::bitop(const std::string& operation, const std::string& destkey, const std::vector<std::string>& keys) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.bitop(operation, destkey, keys, cb); });
}

future_client::future
future_client::bitpos(const std::string& key, int bit) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.bitpos(key, bit, cb); });
}

future_client::future
future_client::bitpos(const std::string& key, int bit, int start) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.bitpos(key, bit, start, cb); });
}

future_client::future
future_client::bitpos(const std::string& key, int bit, int start, int end) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.bitpos(key, bit, start, end, cb); });
}

future_client::future
future_client::blpop(const std::vector<std::string>& keys, int timeout) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.blpop(keys, timeout, cb); });
}

future_client::future
future_client::brpop(const std::vector<std::string>& keys, int timeout) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.brpop(keys, timeout, cb); });
}

future_client::future
future_client::brpoplpush(const std::string& src, const std::string& dst, int timeout) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.brpoplpush(src, dst, timeout, cb); });
}

future_client::future
future_client::client_list() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.client_list(cb); });
}

future_client::future
future_client::client_getname() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.client_getname(cb); });
}

future_client::future
future_client::client_pause(int timeout) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.client_pause(timeout, cb); });
}

future_client::future
future_client::client_reply(const std::string& mode) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.client_reply(mode, cb); });
}

future_client::future
future_client::client_setname(const std::string& name) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.client_setname(name, cb); });
}

future_client::future
future_client::cluster_addslots(const std::vector<std::string>& p_slots) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_addslots(p_slots, cb); });
}

future_client::future
future_client::cluster_count_failure_reports(const std::string& node_id) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_count_failure_reports(node_id, cb); });
}

future_client::future
future_client::cluster_countkeysinslot(const std::string& slot) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_countkeysinslot(slot, cb); });
}

future_client::future
future_client::cluster_delslots(const std::vector<std::string>& p_slots) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_delslots(p_slots, cb); });
}

future_client::future
future_client::cluster_failover() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_failover(cb); });
}

future_client::future
future_client::cluster_failover(const std::string& mode) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_failover(mode, cb); });
}

future_client::future
future_client::cluster_forget(const std::string& node_id) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_forget(node_id, cb); });
}

future_client::future
future_client::cluster_getkeysinslot(const std::string& slot, int count) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_getkeysinslot(slot, count, cb); });
}

future_client::future
future_client::cluster_info() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_info(cb); });
}

future_client::future
future_client::cluster_keyslot(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_keyslot(key, cb); });
}

future_client::future
future_client::cluster_meet(const std::string& ip, int port) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_meet(ip, port, cb); });
}

future_client::future
future_client::cluster_nodes() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_nodes(cb); });
}

future_client::future
future_client::cluster_replicate(const std::string& node_id) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_replicate(node_id, cb); });
}

future_client::future
future_client::cluster_reset(const std::string& mode) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_reset(mode, cb); });
}

future_client::future
future_client::cluster_saveconfig() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_saveconfig(cb); });
}

future_client::future
future_client::cluster_set_config_epoch(const std::string& epoch) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_set_config_epoch(epoch, cb); });
}

future_client::future
future_client::cluster_setslot(const std::string& slot, const std::string& mode) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_setslot(slot, mode, cb); });
}

future_client::future
future_client::cluster_setslot(const std::string& slot, const std::string& mode, const std::string& node_id) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_setslot(slot, mode, node_id, cb); });
}

future_client::future
future_client::cluster_slaves(const std::string& node_id) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_slaves(node_id, cb); });
}

future_client::future
future_client::cluster_slots() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.cluster_slots(cb); });
}

future_client::future
future_client::command() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.command(cb); });
}

future_client::future
future_client::command_count() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.command_count(cb); });
}

future_client::future
future_client::command_getkeys() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.command_getkeys(cb); });
}

future_client::future
future_client::command_info(const std::vector<std::string>& command_name) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.command_info(command_name, cb); });
}

future_client::future
future_client::config_get(const std::string& param) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.config_get(param, cb); });
}

future_client::future
future_client::config_rewrite() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.config_rewrite(cb); });
}

future_client::future
future_client::config_set(const std::string& param, const std::string& val) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.config_set(param, val, cb); });
}

future_client::future
future_client::config_resetstat() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.config_resetstat(cb); });
}

future_client::future
future_client::dbsize() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.dbsize(cb); });
}

future_client::future
future_client::debug_object(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.debug_object(key, cb); });
}

future_client::future
future_client::debug_segfault() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.debug_segfault(cb); });
}

future_client::future
future_client::decr(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.decr(key, cb); });
}

future_client::future
future_client::decrby(const std::string& key, int val) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.decrby(key, val, cb); });
}

future_client::future
future_client::del(const std::vector<std::string>& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.del(key, cb); });
}

future_client::future
future_client::discard() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.discard(cb); });
}

future_client::future
future_client::dump(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.dump(key, cb); });
}

future_client::future
future_client::echo(const std::string& msg) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.echo(msg, cb); });
}

future_client::future
future_client::eval(const std::string& script, int numkeys, const std::vector<std::string>& keys, const std::vector<std::string>& args) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.eval(script, numkeys, keys, args, cb); });
}

future_client::future
future_client::evalsha(const std::string& sha1, int numkeys, const std::vector<std::string>& keys, const std::vector<std::string>& args) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.evalsha(sha1, numkeys, keys, args, cb); });
}

future_client::future
future_client::exec() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.exec(cb); });
}

future_client::future
future_client::exists(const std::vector<std::string>& keys) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.exists(keys, cb); });
}

future_client::future
future_client::expire(const std::string& key, int seconds) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.expire(key, seconds, cb); });
}

future_client::future
future_client::expireat(const std::string& key, int timestamp) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.expireat(key, timestamp, cb); });
}

future_client::future
future_client::flushall() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.flushall(cb); });
}

future_client::future
future_client::flushdb() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.flushdb(cb); });
}

future_client::future
future_client::geoadd(const std::string& key, const std::vector<std::tuple<std::string, std::string, std::string>>& long_lat_memb) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.geoadd(key, long_lat_memb, cb); });
}

future_client::future
future_client::geohash(const std::string& key, const std::vector<std::string>& members) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.geohash(key, members, cb); });
}

future_client::future
future_client::geopos(const std::string& key, const std::vector<std::string>& members) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.geopos(key, members, cb); });
}

future_client::future
future_client::geodist(const std::string& key, const std::string& member_1, const std::string& member_2, const std::string& unit) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.geodist(key, member_1, member_2, unit, cb); });
}

future_client::future
future_client::get(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.get(key, cb); });
}

future_client::future
future_client::getbit(const std::string& key, int offset) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.getbit(key, offset, cb); });
}

future_client::future
future_client::getrange(const std::string& key, int start, int end) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.getrange(key, start, end, cb); });
}

future_client::future
future_client::getset(const std::string& key, const std::string& val) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.getset(key, val, cb); });
}

future_client::future
future_client::hdel(const std::string& key, const std::vector<std::string>& fields) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hdel(key, fields, cb); });
}

future_client::future
future_client::hexists(const std::string& key, const std::string& field) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hexists(key, field, cb); });
}

future_client::future
future_client::hget(const std::string& key, const std::string& field) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hget(key, field, cb); });
}

future_client::future
future_client::hgetall(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hgetall(key, cb); });
}

future_client::future
future_client::hincrby(const std::string& key, const std::string& field, int incr) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hincrby(key, field, incr, cb); });
}

future_client::future
future_client::hincrbyfloat(const std::string& key, const std::string& field, float incr) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hincrbyfloat(key, field, incr, cb); });
}

future_client::future
future_client::hkeys(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hkeys(key, cb); });
}

future_client::future
future_client::hlen(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hlen(key, cb); });
}

future_client::future
future_client::hmget(const std::string& key, const std::vector<std::string>& fields) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hmget(key, fields, cb); });
}

future_client::future
future_client::hmset(const std::string& key, const std::vector<std::pair<std::string, std::string>>& field_val) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hmset(key, field_val, cb); });
}

future_client::future
future_client::hset(const std::string& key, const std::string& field, const std::string& value) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hset(key, field, value, cb); });
}

future_client::future
future_client::hsetnx(const std::string& key, const std::string& field, const std::string& value) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hsetnx(key, field, value, cb); });
}

future_client::future
future_client::hstrlen(const std::string& key, const std::string& field) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hstrlen(key, field, cb); });
}

future_client::future
future_client::hvals(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.hvals(key, cb); });
}

future_client::future
future_client::incr(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.incr(key, cb); });
}

future_client::future
future_client::incrby(const std::string& key, int incr) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.incrby(key, incr, cb); });
}

future_client::future
future_client::incrbyfloat(const std::string& key, float incr) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.incrbyfloat(key, incr, cb); });
}

future_client::future
future_client::info(const std::string& section) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.info(section, cb); });
}

future_client::future
future_client::keys(const std::string& pattern) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.keys(pattern, cb); });
}

future_client::future
future_client::lastsave() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.lastsave(cb); });
}

future_client::future
future_client::lindex(const std::string& key, int index) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.lindex(key, index, cb); });
}

future_client::future
future_client::linsert(const std::string& key, const std::string& before_after, const std::string& pivot, const std::string& value) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.linsert(key, before_after, pivot, value, cb); });
}

future_client::future
future_client::llen(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.llen(key, cb); });
}

future_client::future
future_client::lpop(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.lpop(key, cb); });
}

future_client::future
future_client::lpush(const std::string& key, const std::vector<std::string>& values) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.lpush(key, values, cb); });
}

future_client::future
future_client::lpushx(const std::string& key, const std::string& value) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.lpushx(key, value, cb); });
}

future_client::future
future_client::lrange(const std::string& key, int start, int stop) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.lrange(key, start, stop, cb); });
}

future_client::future
future_client::lrem(const std::string& key, int count, const std::string& value) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.lrem(key, count, value, cb); });
}

future_client::future
future_client::lset(const std::string& key, int index, const std::string& value) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.lset(key, index, value, cb); });
}

future_client::future
future_client::ltrim(const std::string& key, int start, int stop) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.ltrim(key, start, stop, cb); });
}

future_client::future
future_client::mget(const std::vector<std::string>& keys) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.mget(keys, cb); });
}

future_client::future
future_client::migrate(const std::string& host, int port, const std::string& key, const std::string& dest_db, int timeout, bool copy, bool replace, const std::vector<std::string>& keys) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.migrate(host, port, key, dest_db, timeout, copy, replace, keys, cb); });
}

future_client::future
future_client::monitor() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.monitor(cb); });
}

future_client::future
future_client::move(const std::string& key, const std::string& db) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.move(key, db, cb); });
}

future_client::future
future_client::mset(const std::vector<std::pair<std::string, std::string>>& key_vals) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.mset(key_vals, cb); });
}

future_client::future
future_client::msetnx(const std::vector<std::pair<std::string, std::string>>& key_vals) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.msetnx(key_vals, cb); });
}

future_client::future
future_client::multi() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.multi(cb); });
}

future_client::future
future_client::object(const std::string& subcommand, const std::vector<std::string>& args) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.object(subcommand, args, cb); });
}

future_client::future
future_client::persist(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.persist(key, cb); });
}

future_client::future
future_client::pexpire(const std::string& key, int milliseconds) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.pexpire(key, milliseconds, cb); });
}

future_client::future
future_client::pexpireat(const std::string& key, int milliseconds_timestamp) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.pexpireat(key, milliseconds_timestamp, cb); });
}

future_client::future
future_client::pfadd(const std::string& key, const std::vector<std::string>& elements) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.pfadd(key, elements, cb); });
}

future_client::future
future_client::pfcount(const std::vector<std::string>& keys) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.pfcount(keys, cb); });
}

future_client::future
future_client::pfmerge(const std::string& destkey, const std::vector<std::string>& sourcekeys) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.pfmerge(destkey, sourcekeys, cb); });
}

future_client::future
future_client::ping() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.ping(cb); });
}

future_client::future
future_client::ping(const std::string& message) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.ping(message, cb); });
}

future_client::future
future_client::psetex(const std::string& key, int milliseconds, const std::string& val) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.psetex(key, milliseconds, val, cb); });
}

future_client::future
future_client::publish(const std::string& channel, const std::string& message) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.publish(channel, message, cb); });
}

future_client::future
future_client::pubsub(const std::string& subcommand, const std::vector<std::string>& args) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.pubsub(subcommand, args, cb); });
}

future_client::future
future_client::pttl(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.pttl(key, cb); });
}

future_client::future
future_client::quit() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.quit(cb); });
}

future_client::future
future_client::randomkey() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.randomkey(cb); });
}

future_client::future
future_client::readonly() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.readonly(cb); });
}

future_client::future
future_client::readwrite() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.readwrite(cb); });
}

future_client::future
future_client::rename(const std::string& key, const std::string& newkey) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.rename(key, newkey, cb); });
}

future_client::future
future_client::renamenx(const std::string& key, const std::string& newkey) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.renamenx(key, newkey, cb); });
}

future_client::future
future_client::restore(const std::string& key, int ttl, const std::string& serialized_value) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.restore(key, ttl, serialized_value, cb); });
}

future_client::future
future_client::restore(const std::string& key, int ttl, const std::string& serialized_value, const std::string& replace) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.restore(key, ttl, serialized_value, replace, cb); });
}

future_client::future
future_client::role() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.role(cb); });
}

future_client::future
future_client::rpop(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.rpop(key, cb); });
}

future_client::future
future_client::rpoplpush(const std::string& src, const std::string& dst) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.rpoplpush(src, dst, cb); });
}

future_client::future
future_client::rpush(const std::string& key, const std::vector<std::string>& values) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.rpush(key, values, cb); });
}

future_client::future
future_client::rpushx(const std::string& key, const std::string& value) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.rpushx(key, value, cb); });
}

future_client::future
future_client::sadd(const std::string& key, const std::vector<std::string>& members) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.sadd(key, members, cb); });
}

future_client::future
future_client::save() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.save(cb); });
}

future_client::future
future_client::scard(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.scard(key, cb); });
}

future_client::future
future_client::script_debug(const std::string& mode) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.script_debug(mode, cb); });
}

future_client::future
future_client::script_exists(const std::vector<std::string>& scripts) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.script_exists(scripts, cb); });
}

future_client::future
future_client::script_flush() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.script_flush(cb); });
}

future_client::future
future_client::script_kill() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.script_kill(cb); });
}

future_client::future
future_client::script_load(const std::string& script) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.script_load(script, cb); });
}

future_client::future
future_client::sdiff(const std::vector<std::string>& keys) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.sdiff(keys, cb); });
}

future_client::future
future_client::sdiffstore(const std::string& dst, const std::vector<std::string>& keys) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.sdiffstore(dst, keys, cb); });
}

future_client::future
future_client::select(int index) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.select(index, cb); });
}

future_client::future
future_client::set(const std::string& key, const std::string& value) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.set(key, value, cb); });
}

future_client::future
future_client::set_advanced(const std::string& key, const std::string& value, bool ex, int ex_sec, bool px, int px_milli, bool nx, bool xx) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.set_advanced(key, value, ex, ex_sec, px, px_milli, nx, xx, cb); });
}

future_client::future
future_client::setbit_(const std::string& key, int offset, const std::string& value) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.setbit_(key, offset, value, cb); });
}

future_client::future
future_client::setex(const std::string& key, int seconds, const std::string& value) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.setex(key, seconds, value, cb); });
}

future_client::future
future_client::setnx(const std::string& key, const std::string& value) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.setnx(key, value, cb); });
}

future_client::future
future_client::setrange(const std::string& key, int offset, const std::string& value) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.setrange(key, offset, value, cb); });
}

future_client::future
future_client::shutdown() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.shutdown(cb); });
}

future_client::future
future_client::shutdown(const std::string& save) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.shutdown(save, cb); });
}

future_client::future
future_client::sinter(const std::vector<std::string>& keys) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.sinter(keys, cb); });
}

future_client::future
future_client::sinterstore(const std::string& dst, const std::vector<std::string>& keys) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.sinterstore(dst, keys, cb); });
}

future_client::future
future_client::sismember(const std::string& key, const std::string& member) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.sismember(key, member, cb); });
}

future_client::future
future_client::slaveof(const std::string& host, int port) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.slaveof(host, port, cb); });
}

future_client::future
future_client::slowlog(const std::string& subcommand) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.slowlog(subcommand, cb); });
}

future_client::future
future_client::slowlog(const std::string& subcommand, const std::string& argument) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.slowlog(subcommand, argument, cb); });
}

future_client::future
future_client::smembers(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.smembers(key, cb); });
}

future_client::future
future_client::smove(const std::string& src, const std::string& dst, const std::string& member) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.smove(src, dst, member, cb); });
}

future_client::future
future_client::spop(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.spop(key, cb); });
}

future_client::future
future_client::spop(const std::string& key, int count) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.spop(key, count, cb); });
}

future_client::future
future_client::srandmember(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.srandmember(key, cb); });
}

future_client::future
future_client::srandmember(const std::string& key, int count) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.srandmember(key, count, cb); });
}

future_client::future
future_client::srem(const std::string& key, const std::vector<std::string>& members) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.srem(key, members, cb); });
}

future_client::future
future_client::strlen(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.strlen(key, cb); });
}

future_client::future
future_client::sunion(const std::vector<std::string>& keys) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.sunion(keys, cb); });
}

future_client::future
future_client::sunionstore(const std::string& dst, const std::vector<std::string>& keys) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.sunionstore(dst, keys, cb); });
}

future_client::future
future_client::sync() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.sync(cb); });
}

future_client::future
future_client::time() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.time(cb); });
}

future_client::future
future_client::ttl(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.ttl(key, cb); });
}

future_client::future
future_client::type(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.type(key, cb); });
}

future_client::future
future_client::unwatch() {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.unwatch(cb); });
}

future_client::future
future_client::wait(int numslaves, int timeout) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.wait(numslaves, timeout, cb); });
}

future_client::future
future_client::watch(const std::vector<std::string>& keys) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.watch(keys, cb); });
}

future_client::future
future_client::zcard(const std::string& key) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zcard(key, cb); });
}

future_client::future
future_client::zcount(const std::string& key, int min, int max) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zcount(key, min, max, cb); });
}

future_client::future
future_client::zcount(const std::string& key, double min, double max) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zcount(key, min, max, cb); });
}

future_client::future
future_client::zcount(const std::string& key, const std::string& min, const std::string& max) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zcount(key, min, max, cb); });
}

future_client::future
future_client::zincrby(const std::string& key, int incr, const std::string& member) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zincrby(key, incr, member, cb); });
}

future_client::future
future_client::zincrby(const std::string& key, double incr, const std::string& member) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zincrby(key, incr, member, cb); });
}

future_client::future
future_client::zincrby(const std::string& key, const std::string& incr, const std::string& member) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zincrby(key, incr, member, cb); });
}

future_client::future
future_client::zlexcount(const std::string& key, int min, int max) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zlexcount(key, min, max, cb); });
}

future_client::future
future_client::zlexcount(const std::string& key, double min, double max) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zlexcount(key, min, max, cb); });
}

future_client::future
future_client::zlexcount(const std::string& key, const std::string& min, const std::string& max) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zlexcount(key, min, max, cb); });
}

future_client::future
future_client::zrange(const std::string& key, int start, int stop, bool withscores) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zrange(key, start, stop, withscores, cb); });
}

future_client::future
future_client::zrange(const std::string& key, double start, double stop, bool withscores) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zrange(key, start, stop, withscores, cb); });
}

future_client::future
future_client::zrange(const std::string& key, const std::string& start, const std::string& stop, bool withscores) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zrange(key, start, stop, withscores, cb); });
}

future_client::future
future_client::zrank(const std::string& key, const std::string& member) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zrank(key, member, cb); });
}

future_client::future
future_client::zrem(const std::string& key, const std::vector<std::string>& members) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zrem(key, members, cb); });
}

future_client::future
future_client::zremrangebylex(const std::string& key, int min, int max) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zremrangebylex(key, min, max, cb); });
}

future_client::future
future_client::zremrangebylex(const std::string& key, double min, double max) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zremrangebylex(key, min, max, cb); });
}

future_client::future
future_client::zremrangebylex(const std::string& key, const std::string& min, const std::string& max) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zremrangebylex(key, min, max, cb); });
}

future_client::future
future_client::zremrangebyrank(const std::string& key, int start, int stop) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zremrangebyrank(key, start, stop, cb); });
}

future_client::future
future_client::zremrangebyrank(const std::string& key, double start, double stop) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zremrangebyrank(key, start, stop, cb); });
}

future_client::future
future_client::zremrangebyrank(const std::string& key, const std::string& start, const std::string& stop) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zremrangebyrank(key, start, stop, cb); });
}

future_client::future
future_client::zremrangebyscore(const std::string& key, int min, int max) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zremrangebyscore(key, min, max, cb); });
}

future_client::future
future_client::zremrangebyscore(const std::string& key, double min, double max) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zremrangebyscore(key, min, max, cb); });
}

future_client::future
future_client::zremrangebyscore(const std::string& key, const std::string& min, const std::string& max) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zremrangebyscore(key, min, max, cb); });
}

future_client::future
future_client::zrevrange(const std::string& key, int start, int stop, bool withscores) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zrevrange(key, start, stop, withscores, cb); });
}

future_client::future
future_client::zrevrange(const std::string& key, double start, double stop, bool withscores) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zrevrange(key, start, stop, withscores, cb); });
}

future_client::future
future_client::zrevrange(const std::string& key, const std::string& start, const std::string& stop, bool withscores) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zrevrange(key, start, stop, withscores, cb); });
}

future_client::future
future_client::zrevrank(const std::string& key, const std::string& member) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zrevrank(key, member, cb); });
}

future_client::future
future_client::zscore(const std::string& key, const std::string& member) {
  return exec_cmd([=](const rcb_t& cb) -> rc& { return m_client.zscore(key, member, cb); });
}

} //! cpp_redis
