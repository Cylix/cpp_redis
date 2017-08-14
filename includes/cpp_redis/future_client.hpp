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
#include <vector>

#include <cpp_redis/logger.hpp>
#include <cpp_redis/network/tcp_client_iface.hpp>
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
  future exec_cmd(call_t f);

public:
//! ctor & dtor
#ifndef __CPP_REDIS_USE_CUSTOM_TCP_CLIENT
  future_client(void) = default;
#endif /* __CPP_REDIS_USE_CUSTOM_TCP_CLIENT */
  explicit future_client(const std::shared_ptr<network::tcp_client_iface>& tcp_client);
  ~future_client(void) = default;

  //! handle connection
  void connect(const std::string& host = "127.0.0.1", std::size_t port = 6379, const disconnection_handler_t& disconnection_handler = nullptr);
  void disconnect(bool wait_for_removal = false);
  bool is_connected();

  //! send cmd
  future send(const std::vector<std::string>& redis_cmd);

  //! commit pipelined transaction
  future_client& commit(void);
  future_client& sync_commit(void);

  template <class Rep, class Period>
  future_client&
  sync_commit(const std::chrono::duration<Rep, Period>& timeout) {
    m_client.sync_commit(timeout);
    return *this;
  }

public:
  future append(const std::string& key, const std::string& value);
  future auth(const std::string& password);
  future bgrewriteaof();
  future bgsave();
  future bitcount(const std::string& key);
  future bitcount(const std::string& key, int start, int end);
  // future bitfield(const std::string& key) key [get type offset] [set type offset value] [incrby type offset increment] [overflow wrap|sat|fail]
  future bitop(const std::string& operation, const std::string& destkey, const std::vector<std::string>& keys);
  future bitpos(const std::string& key, int bit);
  future bitpos(const std::string& key, int bit, int start);
  future bitpos(const std::string& key, int bit, int start, int end);
  future blpop(const std::vector<std::string>& keys, int timeout);
  future brpop(const std::vector<std::string>& keys, int timeout);
  future brpoplpush(const std::string& src, const std::string& dst, int timeout);
  // future client_kill() [ip:port] [id client-id] [type normal|master|slave|pubsub] [addr ip:port] [skipme yes/no]
  future client_list();
  future client_getname();
  future client_pause(int timeout);
  future client_reply(const std::string& mode);
  future client_setname(const std::string& name);
  future cluster_addslots(const std::vector<std::string>& p_slots);
  future cluster_count_failure_reports(const std::string& node_id);
  future cluster_countkeysinslot(const std::string& slot);
  future cluster_delslots(const std::vector<std::string>& p_slots);
  future cluster_failover();
  future cluster_failover(const std::string& mode);
  future cluster_forget(const std::string& node_id);
  future cluster_getkeysinslot(const std::string& slot, int count);
  future cluster_info();
  future cluster_keyslot(const std::string& key);
  future cluster_meet(const std::string& ip, int port);
  future cluster_nodes();
  future cluster_replicate(const std::string& node_id);
  future cluster_reset(const std::string& mode = "soft");
  future cluster_saveconfig();
  future cluster_set_config_epoch(const std::string& epoch);
  future cluster_setslot(const std::string& slot, const std::string& mode);
  future cluster_setslot(const std::string& slot, const std::string& mode, const std::string& node_id);
  future cluster_slaves(const std::string& node_id);
  future cluster_slots();
  future command();
  future command_count();
  future command_getkeys();
  future command_info(const std::vector<std::string>& command_name);
  future config_get(const std::string& param);
  future config_rewrite();
  future config_set(const std::string& param, const std::string& val);
  future config_resetstat();
  future dbsize();
  future debug_object(const std::string& key);
  future debug_segfault();
  future decr(const std::string& key);
  future decrby(const std::string& key, int val);
  future del(const std::vector<std::string>& key);
  future discard();
  future dump(const std::string& key);
  future echo(const std::string& msg);
  future eval(const std::string& script, int numkeys, const std::vector<std::string>& keys, const std::vector<std::string>& args);
  future evalsha(const std::string& sha1, int numkeys, const std::vector<std::string>& keys, const std::vector<std::string>& args);
  future exec();
  future exists(const std::vector<std::string>& keys);
  future expire(const std::string& key, int seconds);
  future expireat(const std::string& key, int timestamp);
  future flushall();
  future flushdb();
  future geoadd(const std::string& key, const std::vector<std::tuple<std::string, std::string, std::string>>& long_lat_memb);
  future geohash(const std::string& key, const std::vector<std::string>& members);
  future geopos(const std::string& key, const std::vector<std::string>& members);
  future geodist(const std::string& key, const std::string& member_1, const std::string& member_2, const std::string& unit = "m");
  // future georadius() key longitude latitude radius m|km|ft|mi [withcoord] [withdist] [withhash] [count count] [asc|desc] [store key] [storedist key]
  // future georadiusbymember() key member radius m|km|ft|mi [withcoord] [withdist] [withhash] [count count] [asc|desc] [store key] [storedist key]
  future get(const std::string& key);
  future getbit(const std::string& key, int offset);
  future getrange(const std::string& key, int start, int end);
  future getset(const std::string& key, const std::string& val);
  future hdel(const std::string& key, const std::vector<std::string>& fields);
  future hexists(const std::string& key, const std::string& field);
  future hget(const std::string& key, const std::string& field);
  future hgetall(const std::string& key);
  future hincrby(const std::string& key, const std::string& field, int incr);
  future hincrbyfloat(const std::string& key, const std::string& field, float incr);
  future hkeys(const std::string& key);
  future hlen(const std::string& key);
  future hmget(const std::string& key, const std::vector<std::string>& fields);
  future hmset(const std::string& key, const std::vector<std::pair<std::string, std::string>>& field_val);
  future hset(const std::string& key, const std::string& field, const std::string& value);
  future hsetnx(const std::string& key, const std::string& field, const std::string& value);
  future hstrlen(const std::string& key, const std::string& field);
  future hvals(const std::string& key);
  future incr(const std::string& key);
  future incrby(const std::string& key, int incr);
  future incrbyfloat(const std::string& key, float incr);
  future info(const std::string& section = "default");
  future keys(const std::string& pattern);
  future lastsave();
  future lindex(const std::string& key, int index);
  future linsert(const std::string& key, const std::string& before_after, const std::string& pivot, const std::string& value);
  future llen(const std::string& key);
  future lpop(const std::string& key);
  future lpush(const std::string& key, const std::vector<std::string>& values);
  future lpushx(const std::string& key, const std::string& value);
  future lrange(const std::string& key, int start, int stop);
  future lrem(const std::string& key, int count, const std::string& value);
  future lset(const std::string& key, int index, const std::string& value);
  future ltrim(const std::string& key, int start, int stop);
  future mget(const std::vector<std::string>& keys);
  future migrate(const std::string& host, int port, const std::string& key, const std::string& dest_db, int timeout, bool copy = false, bool replace = false, const std::vector<std::string>& keys = {});
  future monitor();
  future move(const std::string& key, const std::string& db);
  future mset(const std::vector<std::pair<std::string, std::string>>& key_vals);
  future msetnx(const std::vector<std::pair<std::string, std::string>>& key_vals);
  future multi();
  future object(const std::string& subcommand, const std::vector<std::string>& args);
  future persist(const std::string& key);
  future pexpire(const std::string& key, int milliseconds);
  future pexpireat(const std::string& key, int milliseconds_timestamp);
  future pfadd(const std::string& key, const std::vector<std::string>& elements);
  future pfcount(const std::vector<std::string>& keys);
  future pfmerge(const std::string& destkey, const std::vector<std::string>& sourcekeys);
  future ping();
  future ping(const std::string& message);
  future psetex(const std::string& key, int milliseconds, const std::string& val);
  future publish(const std::string& channel, const std::string& message);
  future pubsub(const std::string& subcommand, const std::vector<std::string>& args);
  future pttl(const std::string& key);
  future quit();
  future randomkey();
  future readonly();
  future readwrite();
  future rename(const std::string& key, const std::string& newkey);
  future renamenx(const std::string& key, const std::string& newkey);
  future restore(const std::string& key, int ttl, const std::string& serialized_value);
  future restore(const std::string& key, int ttl, const std::string& serialized_value, const std::string& replace);
  future role();
  future rpop(const std::string& key);
  future rpoplpush(const std::string& src, const std::string& dst);
  future rpush(const std::string& key, const std::vector<std::string>& values);
  future rpushx(const std::string& key, const std::string& value);
  future sadd(const std::string& key, const std::vector<std::string>& members);
  future save();
  future scard(const std::string& key);
  future script_debug(const std::string& mode);
  future script_exists(const std::vector<std::string>& scripts);
  future script_flush();
  future script_kill();
  future script_load(const std::string& script);
  future sdiff(const std::vector<std::string>& keys);
  future sdiffstore(const std::string& dst, const std::vector<std::string>& keys);
  future select(int index);
  future set(const std::string& key, const std::string& value);
  future set_advanced(const std::string& key, const std::string& value, bool ex = false, int ex_sec = 0, bool px = false, int px_milli = 0, bool nx = false, bool xx = false);
  future setbit_(const std::string& key, int offset, const std::string& value);
  future setex(const std::string& key, int seconds, const std::string& value);
  future setnx(const std::string& key, const std::string& value);
  future setrange(const std::string& key, int offset, const std::string& value);
  future shutdown();
  future shutdown(const std::string& save);
  future sinter(const std::vector<std::string>& keys);
  future sinterstore(const std::string& dst, const std::vector<std::string>& keys);
  future sismember(const std::string& key, const std::string& member);
  future slaveof(const std::string& host, int port);
  future slowlog(const std::string& subcommand);
  future slowlog(const std::string& subcommand, const std::string& argument);
  future smembers(const std::string& key);
  future smove(const std::string& src, const std::string& dst, const std::string& member);
  // future sort() key [by pattern] [limit offset count] [get pattern [get pattern ...]] [asc|desc] [alpha] [store destination]
  future spop(const std::string& key);
  future spop(const std::string& key, int count);
  future srandmember(const std::string& key);
  future srandmember(const std::string& key, int count);
  future srem(const std::string& key, const std::vector<std::string>& members);
  future strlen(const std::string& key);
  future sunion(const std::vector<std::string>& keys);
  future sunionstore(const std::string& dst, const std::vector<std::string>& keys);
  future sync();
  future time();
  future ttl(const std::string& key);
  future type(const std::string& key);
  future unwatch();
  future wait(int numslaves, int timeout);
  future watch(const std::vector<std::string>& keys);
  // future zadd() key [nx|xx] [ch] [incr] score member [score member ...]
  future zcard(const std::string& key);
  future zcount(const std::string& key, int min, int max);
  future zcount(const std::string& key, double min, double max);
  future zcount(const std::string& key, const std::string& min, const std::string& max);
  future zincrby(const std::string& key, int incr, const std::string& member);
  future zincrby(const std::string& key, double incr, const std::string& member);
  future zincrby(const std::string& key, const std::string& incr, const std::string& member);
  // future zinterstore() destination numkeys key [key ...] [weights weight [weight ...]] [aggregate sum|min|max]
  future zlexcount(const std::string& key, int min, int max);
  future zlexcount(const std::string& key, double min, double max);
  future zlexcount(const std::string& key, const std::string& min, const std::string& max);
  future zrange(const std::string& key, int start, int stop, bool withscores = false);
  future zrange(const std::string& key, double start, double stop, bool withscores = false);
  future zrange(const std::string& key, const std::string& start, const std::string& stop, bool withscores = false);
  // future zrangebylex() key min max [limit offset count]
  // future zrevrangebylex() key max min [limit offset count]
  // future zrangebyscore() key min max [withscores] [limit offset count]
  future zrank(const std::string& key, const std::string& member);
  future zrem(const std::string& key, const std::vector<std::string>& members);
  future zremrangebylex(const std::string& key, int min, int max);
  future zremrangebylex(const std::string& key, double min, double max);
  future zremrangebylex(const std::string& key, const std::string& min, const std::string& max);
  future zremrangebyrank(const std::string& key, int start, int stop);
  future zremrangebyrank(const std::string& key, double start, double stop);
  future zremrangebyrank(const std::string& key, const std::string& start, const std::string& stop);
  future zremrangebyscore(const std::string& key, int min, int max);
  future zremrangebyscore(const std::string& key, double min, double max);
  future zremrangebyscore(const std::string& key, const std::string& min, const std::string& max);
  future zrevrange(const std::string& key, int start, int stop, bool withscores = false);
  future zrevrange(const std::string& key, double start, double stop, bool withscores = false);
  future zrevrange(const std::string& key, const std::string& start, const std::string& stop, bool withscores = false);
  // future zrevrangebyscore() key max min [withscores] [limit offset count]
  future zrevrank(const std::string& key, const std::string& member);
  future zscore(const std::string& key, const std::string& member);
  // future zunionstore() destination numkeys key [key ...] [weights weight [weight ...]] [aggregate sum|min|max]
  // future scan() cursor [match pattern] [count count]
  // future sscan() key cursor [match pattern] [count count]
  // future hscan() key cursor [match pattern] [count count]
  // future zscan() key cursor [match pattern] [count count]

private:
  redis_client m_client;
};

} //! cpp_redis
