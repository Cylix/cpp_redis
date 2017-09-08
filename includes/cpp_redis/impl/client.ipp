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

#include <functional>
#include <iostream>

namespace cpp_redis {

template <typename T>
typename std::enable_if<std::is_same<T, client::client_type>::value>::type
client::client_kill_unpack_arg(std::vector<std::string>& redis_cmd, reply_callback_t&, client_type type) {
  redis_cmd.emplace_back("TYPE");
  std::string type_string;

  switch (type) {
  case client_type::normal: type_string = "normal"; break;
  case client_type::master: type_string = "master"; break;
  case client_type::pubsub: type_string = "pubsub"; break;
  case client_type::slave: type_string = "slave"; break;
  }

  redis_cmd.emplace_back(type_string);
}

template <typename T>
typename std::enable_if<std::is_same<T, bool>::value>::type
client::client_kill_unpack_arg(std::vector<std::string>& redis_cmd, reply_callback_t&, bool skip) {
  redis_cmd.emplace_back("SKIPME");
  redis_cmd.emplace_back(skip ? "yes" : "no");
}

template <typename T>
typename std::enable_if<std::is_integral<T>::value>::type
client::client_kill_unpack_arg(std::vector<std::string>& redis_cmd, reply_callback_t&, uint64_t id) {
  redis_cmd.emplace_back("ID");
  redis_cmd.emplace_back(std::to_string(id));
}

template <typename T>
typename std::enable_if<std::is_class<T>::value>::type
client::client_kill_unpack_arg(std::vector<std::string>&, reply_callback_t& reply_callback, const T& cb) {
  reply_callback = cb;
}

template <typename T, typename... Ts>
void
client::client_kill_impl(std::vector<std::string>& redis_cmd, reply_callback_t& reply, const T& arg, const Ts&... args) {
  static_assert(!std::is_class<T>::value, "Reply callback should be in the end of the argument list");
  client_kill_unpack_arg<T>(redis_cmd, reply, arg);
  client_kill_impl(redis_cmd, reply, args...);
};

template <typename T>
void
client::client_kill_impl(std::vector<std::string>& redis_cmd, reply_callback_t& reply, const T& arg) {
  client_kill_unpack_arg<T>(redis_cmd, reply, arg);
};

template <typename T, typename... Ts>
inline client&
client::client_kill(const T& arg, const Ts&... args) {
  static_assert(helpers::is_different_types<T, Ts...>::value, "Should only have one distinct value per filter type");
  static_assert(!(std::is_class<T>::value && std::is_same<T, typename helpers::back<T, Ts...>::type>::value), "Should have at least one filter");

  std::vector<std::string> redis_cmd({"CLIENT", "KILL"});
  reply_callback_t reply_cb = nullptr;
  client_kill_impl<T, Ts...>(redis_cmd, reply_cb, arg, args...);

  return send(redis_cmd, reply_cb);
}

template <typename T, typename... Ts>
inline client&
client::client_kill(const std::string& host, int port, const T& arg, const Ts&... args) {
  static_assert(helpers::is_different_types<T, Ts...>::value, "Should only have one distinct value per filter type");
  std::vector<std::string> redis_cmd({"CLIENT", "KILL"});

  //! If we have other type than lambda, then it's a filter
  if (!std::is_class<T>::value) {
    redis_cmd.emplace_back("ADDR");
  }

  redis_cmd.emplace_back(host + ":" + std::to_string(port));
  reply_callback_t reply_cb = nullptr;
  client_kill_impl<T, Ts...>(redis_cmd, reply_cb, arg, args...);

  return send(redis_cmd, reply_cb);
}

inline client&
client::client_kill(const std::string& host, int port) {
  return client_kill(host, port, reply_callback_t(nullptr));
}

template <typename... Ts>
inline client&
client::client_kill(const char* host, int port, const Ts&... args) {
  return client_kill(std::string(host), port, args...);
}

template <typename T, typename... Ts>
std::future<reply>
client::client_kill_future(const T arg, const Ts... args) {
  //! gcc 4.8 doesn't handle variadic template capture arguments (appears in 4.9)
  //! so std::bind should capture all arguments because of the compiler.
  return exec_cmd(std::bind([this](T arg, Ts... args, const reply_callback_t& cb) -> client& {
    return client_kill(arg, args..., cb);
  },
    arg, args..., std::placeholders::_1));
}

} // namespace cpp_redis
