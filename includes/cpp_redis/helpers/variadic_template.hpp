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

#include <type_traits>

namespace cpp_redis {
namespace helpers {

//!
//! type traits to return last element of a variadic list
//!
template <typename T, typename... Args>
struct back {
  //!
  //! last type of variadic list
  //!
  using type = typename back<Args...>::type;
};

//!
//! type traits to return last element of a variadic list
//!
template <typename T>
struct back<T> {
  //!
  //! templated type
  //!
  using type = T;
};

//!
//! type traits to return front element of a variadic list
//!
template <typename T, typename... Ts>
struct front {
  //!
  //! front type of variadic list
  //!
  using type = T;
};

//!
//! type traits to check if type is present in variadic list
//!
template <typename T1, typename T2, typename... Ts>
struct is_type_present {
  //!
  //! true if T1 is present in remaining types of variadic list
  //! false otherwise
  //!
  static constexpr bool value = std::is_same<T1, T2>::value
                                  ? true
                                  : is_type_present<T1, Ts...>::value;
};

//!
//! type traits to check if type is present in variadic list
//!
template <typename T1, typename T2>
struct is_type_present<T1, T2> {
  //!
  //! true if T1 and T2 are the same
  //! false otherwise
  //!
  static constexpr bool value = std::is_same<T1, T2>::value;
};

//!
//! type traits to check if type is not present in variadic list
//!
template <typename T, typename... Args>
struct is_different_types {
  //!
  //! true if T is not in remaining types of variadic list
  //! false otherwise
  //!
  static constexpr bool value = is_type_present<T, Args...>::value
                                  ? false
                                  : is_different_types<Args...>::value;
};

//!
//! type traits to check if type is not present in variadic list
//!
template <typename T1>
struct is_different_types<T1> {
  //!
  //! true
  //!
  static constexpr bool value = true;
};

} // namespace helpers

} // namespace cpp_redis
