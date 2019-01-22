#include <utility>

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
// SOFTWARE.#ifndef CPP_REDIS_TYPES_HPP
#ifndef CPP_REDIS_IMPL_TYPES_HPP
#define CPP_REDIS_IMPL_TYPES_HPP

#include <cpp_redis/core/reply.hpp>
#include <cpp_redis/misc/convert.hpp>
#include <cpp_redis/misc/logger.hpp>
#include <cpp_redis/misc/optional.hpp>
#include <sstream>
#include <string>
#include <vector>

#include <map>


namespace cpp_redis {

class serializer_type {
public:
  inline serializer_type() {}

  /**
 * @return the underlying string
 *
 */
  virtual const std::string& as_string() const = 0;

  /**
 * @return the underlying integer
 *
 */
  virtual optional_t<int64_t> try_get_int() const = 0;

protected:
  std::string m_str_val;
};

typedef std::shared_ptr<serializer_type> serializer_ptr_t;

template <typename T>
class message_impl {
public:
  virtual const std::string get_id() const = 0;

  virtual const message_impl& set_id(std::string id) = 0;

  virtual T find(std::string key) const = 0;

  virtual const message_impl& push(std::string key, T value) = 0;

  virtual const message_impl& push(std::vector<std::pair<std::string, T>> values) = 0;

  virtual const message_impl& push(typename std::vector<T>::const_iterator ptr_begin,
    typename std::vector<T>::const_iterator ptr_end) = 0;

  virtual const std::multimap<std::string, T>& get_values() const = 0;

protected:
  std::string m_id;
  std::multimap<std::string, T> m_values;
};

class message_type : public message_impl<reply_t> {
public:
  inline const std::string
  get_id() const override { return m_id; };

  inline const message_type&
  set_id(std::string id) override {
    m_id = id;
    return *this;
  }

  inline reply_t
  find(std::string key) const override {
    auto it = m_values.find(key);
    if (it != m_values.end())
      return it->second;
    else
      throw "value not found";
  };

  inline message_type&
  push(std::string key, reply_t value) override {
    m_values.insert({key, std::move(value)});
    return *this;
  }

  inline message_type&
  push(std::vector<std::pair<std::string, reply_t>> values) override {
    for (auto& v : values) {
      m_values.insert({v.first, std::move(v.second)});
    }
    return *this;
  }

  inline message_type&
  push(std::vector<reply_t>::const_iterator ptr_begin,
    std::vector<reply_t>::const_iterator ptr_end) override {
    std::string key;
    size_t i = 2;
    for (auto pb = ptr_begin; pb != ptr_end; pb++) {
      if (i % 2 == 0) {
        key = pb->as_string();
      }
      else {
        m_values.insert({key, *pb});
      }
    }
    return *this;
  }

  inline const std::multimap<std::string, reply_t>&
  get_values() const override {
    return m_values;
  };

  inline std::multimap<std::string, std::string>
  get_str_values() const {
    std::multimap<std::string, std::string> ret;
    for (auto& v : m_values) {
      std::stringstream s;
      s << v.second;
      ret.insert({v.first, s.str()});
    }
    return ret;
  };
};
} // namespace cpp_redis

#endif //CPP_REDIS_TYPES_HPP
