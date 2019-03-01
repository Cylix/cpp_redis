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

#include <cpp_redis/core/reply.hpp>
#include <cpp_redis/misc/error.hpp>
#include <cpp_redis/misc/logger.hpp>

namespace cpp_redis {

	reply::reply()
			: m_type(type::null) {}

	reply::reply(const std::string &value, string_type reply_type)
			: m_type(static_cast<type>(reply_type)), m_str_val(value) {
	}

	reply::reply(int64_t value)
			: m_type(type::integer), m_int_val(value) {}

	reply::reply(const std::vector<reply> &rows)
			: m_type(type::array), m_rows(rows) {}

	reply::reply(reply &&other) noexcept {
		m_type = other.m_type;
		m_rows = std::move(other.m_rows);
		m_str_val = std::move(other.m_str_val);
		m_int_val = other.m_int_val;
	}

	optional_t<int64_t> reply::try_get_int() const {
		if (is_integer())
			return optional_t<int64_t>(m_int_val);

		__CPP_REDIS_LOG(1, "Reply is not an integer");
		return {0};
	}

	reply &
	reply::operator=(reply &&other) noexcept {
		if (this != &other) {
			m_type = other.m_type;
			m_rows = std::move(other.m_rows);
			m_str_val = std::move(other.m_str_val);
			m_int_val = other.m_int_val;
		}

		return *this;
	}

	bool
	reply::ok() const {
		return !is_error();
	}

	bool
	reply::ko() const {
		return !ok();
	}

	const std::string &
	reply::error() const {
		if (!is_error())
			throw cpp_redis::redis_error("Reply is not an error");

		return as_string();
	}

	reply::operator bool() const {
		return !is_error() && !is_null();
	}

	void
	reply::set() {
		m_type = type::null;
	}

	void
	reply::set(const std::string &value, string_type reply_type) {
		m_type = static_cast<type>(reply_type);
		m_str_val = value;
	}

	void
	reply::set(int64_t value) {
		m_type = type::integer;
		m_int_val = value;
	}

	void
	reply::set(const std::vector<reply> &rows) {
		m_type = type::array;
		m_rows = rows;
	}

	reply &
	reply::operator<<(const reply &reply) {
		m_type = type::array;
		m_rows.push_back(reply);

		return *this;
	}

	bool
	reply::is_array() const {
		return m_type == type::array;
	}

	bool
	reply::is_string() const {
		return is_simple_string() || is_bulk_string() || is_error();
	}

	bool
	reply::is_simple_string() const {
		return m_type == type::simple_string;
	}

	bool
	reply::is_bulk_string() const {
		return m_type == type::bulk_string;
	}

	bool
	reply::is_error() const {
		return m_type == type::error;
	}

	bool
	reply::is_integer() const {
		return m_type == type::integer;
	}

	bool
	reply::is_null() const {
		return m_type == type::null;
	}

	const std::vector<reply> &
	reply::as_array() const {
		if (!is_array())
			throw cpp_redis::redis_error("Reply is not an array");

		return m_rows;
	}

	const std::string &
	reply::as_string() const {
		if (!is_string())
			throw cpp_redis::redis_error("Reply is not a string");

		return m_str_val;
	}

	int64_t
	reply::as_integer() const {
		if (!is_integer())
			throw cpp_redis::redis_error("Reply is not an integer");

		return m_int_val;
	}

	reply::type
	reply::get_type() const {
		return m_type;
	}

} // namespace cpp_redis

std::ostream &
operator<<(std::ostream &os, const cpp_redis::reply &reply) {
	switch (reply.get_type()) {
		case cpp_redis::reply::type::error:
			os << reply.error();
			break;
		case cpp_redis::reply::type::bulk_string:
			os << reply.as_string();
			break;
		case cpp_redis::reply::type::simple_string:
			os << reply.as_string();
			break;
		case cpp_redis::reply::type::null:
			os << std::string("(nil)");
			break;
		case cpp_redis::reply::type::integer:
			os << reply.as_integer();
			break;
		case cpp_redis::reply::type::array:
			for (const auto &item : reply.as_array())
				os << item;
			break;
	}

	return os;
}
