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

#include <cpp_redis/builders/builder_iface.hpp>
#include <cpp_redis/builders/integer_builder.hpp>
#include <cpp_redis/reply.hpp>

namespace cpp_redis {

namespace builders {

class array_builder : public builder_iface {
public:
  //! ctor & dtor
  array_builder(void);
  ~array_builder(void) = default;

  //! copy ctor & assignment operator
  array_builder(const array_builder&) = delete;
  array_builder& operator=(const array_builder&) = delete;

public:
  //! builder_iface impl
  builder_iface& operator<<(std::string&);
  bool reply_ready(void) const;
  reply get_reply(void) const;

private:
  bool fetch_array_size(std::string& buffer);
  bool build_row(std::string& buffer);

private:
  integer_builder m_int_builder;
  uint64_t m_array_size;

  std::unique_ptr<builder_iface> m_current_builder;

  bool m_reply_ready;
  reply m_reply;
};

} //! builders

} //! cpp_redis
