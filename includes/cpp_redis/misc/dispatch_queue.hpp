/*
 *
 * Created by nick on 11/22/18.
 *
 * Copyright(c) 2018 Iris. All rights reserved.
 *
 * Use and copying of this software and preparation of derivative
 * works based upon this software are  not permitted.  Any distribution
 * of this software or derivative works must comply with all applicable
 * Canadian export control laws.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL IRIS OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

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
	typedef std::function<cpp_redis::message_type(const cpp_redis::message_type&)> dispatch_callback_t;

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
}


#endif //CPP_REDIS_DISPATCH_QUEUE_HPP
