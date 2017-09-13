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

#include <cpp_redis/builders/array_builder.hpp>
#include <cpp_redis/builders/builders_factory.hpp>
#include <cpp_redis/builders/bulk_string_builder.hpp>
#include <cpp_redis/builders/error_builder.hpp>
#include <cpp_redis/builders/integer_builder.hpp>
#include <cpp_redis/builders/simple_string_builder.hpp>
#include <cpp_redis/misc/error.hpp>
#include <cpp_redis/misc/logger.hpp>

namespace cpp_redis {

namespace builders {

std::unique_ptr<builder_iface>
create_builder(char id) {
  switch (id) {
  case '+':
    return std::unique_ptr<simple_string_builder>{new simple_string_builder()};
  case '-':
    return std::unique_ptr<error_builder>{new error_builder()};
  case ':':
    return std::unique_ptr<integer_builder>{new integer_builder()};
  case '$':
    return std::unique_ptr<bulk_string_builder>{new bulk_string_builder()};
  case '*':
    return std::unique_ptr<array_builder>{new array_builder()};
  default:
    __CPP_REDIS_LOG(error, "cpp_redis::builders::create_builder receives invalid data type");
    throw redis_error("Invalid data");
  }
}

} // namespace builders

} // namespace cpp_redis
