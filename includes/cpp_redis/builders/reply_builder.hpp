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
#include <cpp_redis/reply.hpp>

namespace cpp_redis {

namespace builders {

class reply_builder {
public:
  //! ctor & dtor
  reply_builder(void);
  ~reply_builder(void) = default;

  //! copy ctor & assignment operator
  reply_builder(const reply_builder&) = delete;
  reply_builder& operator=(const reply_builder&) = delete;

public:
  //! add data to reply builder
  reply_builder& operator<<(const std::string& data);

  //! get reply
  void operator>>(reply& reply);
  const reply& get_front(void) const;
  void pop_front(void);

  //! returns whether a reply is available
  bool reply_available(void) const;

private:
  //! build reply. Return whether the reply has been fully built or not
  bool build_reply(void);

private:
  std::string m_buffer;
  std::unique_ptr<builder_iface> m_builder;
  std::deque<reply> m_available_replies;
};

} //! builders

} //! cpp_redis
