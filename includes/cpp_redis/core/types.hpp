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

#ifndef CPP_REDIS_TYPES_HPP
#define CPP_REDIS_TYPES_HPP

#include <string>
#include <vector>


namespace cpp_redis {
	class range {
	public:
			enum class range_state {
					omit,
					include
			};
			explicit range(range_state state);
			explicit range(int count);
			range(int min, int max);
			range(int min, int max, int count);

			bool should_omit() const;
			std::vector<std::string> get_args();
			std::vector<std::string> get_xpending_args() const;

	private:
			int m_count;
			int m_min;
			int m_max;
			range_state m_state;
	};

	typedef range range_t;
} // namespace cpp_redis


#endif //CPP_REDIS_TYPES_HPP
