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

#include <iostream>
#include <string>
#include <vector>

#include <cpp_redis/misc/optional.hpp>

#include <cstdint>

namespace cpp_redis {

/**
 * cpp_redis::reply is the class that wraps Redis server replies.
 * That is, cpp_redis::reply objects are passed as parameters of commands callbacks and contain the server's response.
 *
 */
	class reply {
	public:
#define __CPP_REDIS_REPLY_ERR 0
#define __CPP_REDIS_REPLY_BULK 1
#define __CPP_REDIS_REPLY_SIMPLE 2
#define __CPP_REDIS_REPLY_NULL 3
#define __CPP_REDIS_REPLY_INT 4
#define __CPP_REDIS_REPLY_ARRAY 5

/**
 * type of reply, based on redis server standard replies
 *
 */
			enum class type {
					error = __CPP_REDIS_REPLY_ERR,
					bulk_string = __CPP_REDIS_REPLY_BULK,
					simple_string = __CPP_REDIS_REPLY_SIMPLE,
					null = __CPP_REDIS_REPLY_NULL,
					integer = __CPP_REDIS_REPLY_INT,
					array = __CPP_REDIS_REPLY_ARRAY
			};

/**
 * specific type of replies for string-based replies
 *
 */
			enum class string_type {
					error = __CPP_REDIS_REPLY_ERR,
					bulk_string = __CPP_REDIS_REPLY_BULK,
					simple_string = __CPP_REDIS_REPLY_SIMPLE
			};

	public:
/**
 * default ctor (set a null reply)
 *
 */
			reply();

/**
 * ctor for string values
 *
 * @param value string value
 * @param reply_type of string reply
 *
 */
			reply(const std::string &value, string_type reply_type);

/**
 * ctor for int values
 *
 * @param value integer value
 *
 */
			explicit reply(int64_t value);

/**
 * ctor for array values
 *
 * @param rows array reply
 * @return current instance
 *
 */
			explicit reply(const std::vector<reply> &rows);

/**
 * dtor
 *
 */
			~reply() = default;

/**
 * copy ctor
 *
 */
			reply(const reply &) = default;

/**
 * assignment operator
 *
 */
			reply &operator=(const reply &) = default;

/**
 * move ctor
 *
 */
			reply(reply &&) noexcept;

/**
 * move assignment operator
 *
 */
			reply &operator=(reply &&) noexcept;

	public:
/**
 * @return whether the reply is an array
 *
 */
			bool is_array() const;

/**
 * @return whether the reply is a string (simple, bulk, error)
 *
 */
			bool is_string() const;

/**
 * @return whether the reply is a simple string
 *
 */
			bool is_simple_string() const;

/**
 * @return whether the reply is a bulk string
 *
 */
			bool is_bulk_string() const;

/**
 * @return whether the reply is an error
 *
 */
			bool is_error() const;

/**
 * @return whether the reply is an integer
 *
 */
			bool is_integer() const;

/**
 * @return whether the reply is null
 *
 */
			bool is_null() const;

	public:
/**
 * @return true if function is not an error
 *
 */
			bool ok() const;

/**
 * @return true if function is an error
 *
 */
			bool ko() const;

/**
 * convenience implicit conversion, same as !is_null() / ok()
 *
 */
			explicit operator bool() const;

	public:
			optional_t<int64_t> try_get_int() const;

	public:
/**
 * @return the underlying error
 *
 */
			const std::string &error() const;

/**
 * @return the underlying array
 *
 */
			const std::vector<reply> &as_array() const;

/**
 * @return the underlying string
 *
 */
			const std::string &as_string() const;

/**
 * @return the underlying integer
 *
 */
			int64_t as_integer() const;

	public:
/**
 * set reply as null
 *
 */
			void set();

/**
 * set a string reply
 *
 * @param value string value
 * @param reply_type of string reply
 *
 */
			void set(const std::string &value, string_type reply_type);

/**
 * set an integer reply
 *
 * @param value integer value
 *
 */
			void set(int64_t value);

/**
 * set an array reply
 *
 * @param rows array reply
 *
 */
			void set(const std::vector<reply> &rows);

/**
 * for array replies, add a new row to the reply
 *
 * @param reply new row to be appended
 * @return current instance
 *
 */
			reply &operator<<(const reply &reply);

	public:
/**
 * @return reply type
 *
 */
			type get_type() const;

	private:
			type m_type;
			std::vector<cpp_redis::reply> m_rows;
			std::string m_str_val;
			int64_t m_int_val;
	};

	typedef reply reply_t;

} // namespace cpp_redis

/**
 * support for output
 *
 */
std::ostream &operator<<(std::ostream &os, const cpp_redis::reply_t &reply);
