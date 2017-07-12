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
#include <iostream>

namespace cpp_redis {
class client_kill {

public:
  enum class type {
    normal,
    master,
    pubsub,
    slave
  };

private:
  using CB = std::function<void(cpp_redis::reply&)>;

  template <typename T>
  typename std::enable_if<std::is_same<T, type>::value>::type
  client_kill_unpack_arg(std::vector<std::string>& redis_cmd, CB& /* */, type type) {
    redis_cmd.emplace_back("TYPE");
    std::string type_string;

    switch (type) {
    case client_kill::type::normal: type_string = "normal"; break;
    case client_kill::type::master: type_string = "master"; break;
    case client_kill::type::pubsub: type_string = "pubsub"; break;
    case client_kill::type::slave: type_string  = "slave"; break;
    }

    redis_cmd.emplace_back(type_string);
  }

  template <typename T>
  typename std::enable_if<std::is_same<T, bool>::value>::type
  client_kill_unpack_arg(std::vector<std::string>& redis_cmd, CB& /* */, bool skip) {
    redis_cmd.emplace_back("SKIPME");
    redis_cmd.emplace_back(skip ? "yes" : "no");
  }

  template <typename T>
  typename std::enable_if<std::is_integral<T>::value>::type
  client_kill_unpack_arg(std::vector<std::string>& redis_cmd, CB& /* */, uint64_t id) {
    redis_cmd.emplace_back("ID");
    redis_cmd.emplace_back(std::to_string(id));
  }

  template <typename T>
  typename std::enable_if<std::is_class<T>::value>::type
  client_kill_unpack_arg(std::vector<std::string>& /* */, CB& reply_callback, const T& cb) {
    reply_callback = cb;
  }

protected:
  template <typename T, typename... Ts>
  void
  client_kill_impl(std::vector<std::string>& redis_cmd, CB& reply, const T& arg, const Ts&... args) {
    static_assert(!std::is_class<T>::value, "Reply callback should be in the end of the argument list");
    client_kill_unpack_arg<T>(redis_cmd, reply, arg);
    client_kill_impl(redis_cmd, reply, args...);
  };

  template <typename T>
  void
  client_kill_impl(std::vector<std::string>& redis_cmd, CB& reply, const T& arg) {
    client_kill_unpack_arg<T>(redis_cmd, reply, arg);
  };
};
} //! cpp_redis
