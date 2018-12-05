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

#include <deque>
#include <memory>
#include <stdexcept>
#include <string>

#include <cpp_redis/builders/builder_iface.hpp>
#include <cpp_redis/core/reply.hpp>

namespace cpp_redis {

namespace builders {

/**
 * class coordinating the several builders and the builder factory to build all the replies returned by redis server
 *
 */
class reply_builder {
public:
/**
 * ctor
 *
 */
  reply_builder();
/**
 * dtor
 *
 */
  ~reply_builder() = default;

/**
 * copy ctor
 *
 */
  reply_builder(const reply_builder&) = delete;
/**
 * assignment operator
 *
 */
  reply_builder& operator=(const reply_builder&) = delete;

public:
/**
 * add data to reply builder
 * data is used to build replies that can be retrieved with get_front later on if reply_available returns true
 *
 * @param data data to be used for building replies
 * @return current instance
 *
 */
  reply_builder& operator<<(const std::string& data);

/**
 * similar as get_front, store reply in the passed parameter
 *
 * @param reply reference to the reply object where to store the first available reply
 *
 */
  void operator>>(reply& reply);

/**
 * @return the first available reply
 *
 */
  const reply& get_front() const;

/**
 * pop the first available reply
 *
 */
  void pop_front();

/**
 * @return whether a reply is available
 *
 */
  bool reply_available() const;

/**
 * reset the reply builder to its initial state (clear internal buffer and stages)
 *
 */
  void reset();

private:
/**
 * build reply using m_buffer content
 *
 * @return whether the reply has been fully built or not
 *
 */
  bool build_reply();

private:
/**
 * buffer to be used to build data
 *
 */
  std::string m_buffer;

/**
 * current builder used to build current reply
 *
 */
  std::unique_ptr<builder_iface> m_builder;

/**
 * queue of available (built) replies
 *
 */
  std::deque<reply> m_available_replies;
};

} // namespace builders

} // namespace cpp_redis
