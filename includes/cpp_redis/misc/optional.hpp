// The MIT License (MIT)
//
// Copyright (c) 11/27/18 nick. <nbatkins@gmail.com>
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
// SOFTWARE.#ifndef CPP_REDIS_OPTIONAL_HPP
#ifndef CPP_REDIS_OPTIONAL_HPP
#define CPP_REDIS_OPTIONAL_HPP

#include <string>

// Prefer std optional
#if __cplusplus >= 201703L
#include <optional>

namespace cpp_redis {
template <class T>
using optional_t = std::optional<T>;

template <int I, class T>
using enableIf = typename std::enable_if<I, T>::type;
#else

#include <cpp_redis/misc/logger.hpp>

namespace cpp_redis {
template <int I, class T>
using enableIf = typename std::enable_if<I, T>::type;

template <class T>
struct optional {
  optional(T value) : m_value(value) {}
//  optional<T>&
//  operator()(T value) {
//    m_value = value;
//    return *this;
//  }

  T m_value;

  template <class U>
  enableIf<std::is_convertible<U, T>::value, T>
  value_or(U&& v) const {
    __CPP_REDIS_LOG(1, "value_or(U&& v)\n")
    return std::forward<U>(v);
  }

  template <class F>
  auto
  value_or(F&& action) const -> decltype(action()) {
    return action();
  }
};

template <class T>
using optional_t = optional<T>;
#endif

} // namespace cpp_redis

#endif //CPP_REDIS_OPTIONAL_HPP
