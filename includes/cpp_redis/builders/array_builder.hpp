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

#ifndef CPP_REDIS_BUILDERS_ARRAY_BUILDER_HPP_
#define CPP_REDIS_BUILDERS_ARRAY_BUILDER_HPP_

#include <cpp_redis/builders/builder_iface.hpp>
#include <cpp_redis/builders/integer_builder.hpp>
#include <cpp_redis/core/reply.hpp>

namespace cpp_redis {

	namespace builders {

/**
 * builder to build redis array replies
 *
 */
		class array_builder : public builder_iface {
		public:
/**
 * ctor
 *
 */
				array_builder();

/**
 * dtor
 *
 */
				~array_builder() override = default;

/**
 * copy ctor
 *
 */
				array_builder(const array_builder &) = delete;

/**
 * assignment operator
 *
 */
				array_builder &operator=(const array_builder &) = delete;

		public:
/**
 * take data as parameter which is consumed to build the reply
 * every bytes used to build the reply must be removed from the buffer passed as parameter
 *
 * @param data data to be consumed
 * @return current instance
 *
 */
				builder_iface &operator<<(std::string &data) override;

/**
 * @return whether the reply could be built
 *
 */
				bool reply_ready() const override;

/**
 * @return reply object
 *
 */
				reply get_reply() const override;

		private:
/**
 * take data as parameter which is consumed to determine array size
 * every bytes used to build size is removed from the buffer passed as parameter
 *
 * @param buffer data to be consumer
 * @return true if the size could be found
 *
 */
				bool fetch_array_size(std::string &buffer);

/**
 * take data as parameter which is consumed to build an array row
 * every bytes used to build row is removed from the buffer passed as parameter
 *
 * @param buffer data to be consumer
 * @return true if the row could be built
 *
 */
				bool build_row(std::string &buffer);

		private:
/**
 * builder used to fetch the array size
 *
 */
				integer_builder m_int_builder;

/**
 * built array size
 *
 */
				uint64_t m_array_size;

/**
 * current builder used to build current row
 *
 */
				std::unique_ptr<builder_iface> m_current_builder;

/**
 * whether the reply is ready or not
 *
 */
				bool m_reply_ready;

/**
 * reply to be built (or built)
 *
 */
				reply m_reply;
		};

	} // namespace builders

} // namespace cpp_redis

#endif
