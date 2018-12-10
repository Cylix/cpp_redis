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
// SOFTWARE.#ifndef CPP_REDIS_CONVERT_HPP
//
// Code modified from https://github.com/embeddedartistry/embedded-resources/blob/master/examples/cpp/dispatch.cpp
//

#ifndef CPP_REDIS_DISPATCH_QUEUE_HPP
#define CPP_REDIS_DISPATCH_QUEUE_HPP

#include <thread>
#include <functional>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <queue>
#include <mutex>
#include <string>
#include <condition_variable>

#include <cpp_redis/impl/types.hpp>

namespace cpp_redis {
	typedef std::multimap<std::string, std::string> consumer_response_t;

	typedef std::function<consumer_response_t(const cpp_redis::message_type&)> dispatch_callback_t;

	typedef std::function<void(size_t size)> notify_callback_t;

	typedef struct dispatch_callback_collection {
			dispatch_callback_t callback;
			message_type message;
	} dispatch_callback_collection_t;

	class dispatch_queue {

	public:
			explicit dispatch_queue(std::string name, const notify_callback_t &notify_callback, size_t thread_cnt = 1);
			~dispatch_queue();

			// dispatch and copy
			void dispatch(const cpp_redis::message_type& message, const dispatch_callback_t& op);
			// dispatch and move
			void dispatch(const cpp_redis::message_type& message, dispatch_callback_t&& op);

			// Deleted operations
			dispatch_queue(const dispatch_queue& rhs) = delete;
			dispatch_queue& operator=(const dispatch_queue& rhs) = delete;
			dispatch_queue(dispatch_queue&& rhs) = delete;
			dispatch_queue& operator=(dispatch_queue&& rhs) = delete;

			size_t size();

	private:
			std::string m_name;
			std::mutex m_threads_lock;
			mutable std::vector<std::thread> m_threads;
			std::mutex m_mq_mutex;
			std::queue<dispatch_callback_collection_t> m_mq;
			std::condition_variable m_cv;
			bool m_quit = false;

			notify_callback_t notify_handler;

			void dispatch_thread_handler();
	};

	typedef dispatch_queue dispatch_queue_t;
	typedef std::unique_ptr<dispatch_queue> dispatch_queue_ptr_t;
}


#endif //CPP_REDIS_DISPATCH_QUEUE_HPP
