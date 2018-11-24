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

namespace cpp_redis {
	class dispatch_queue {
			typedef std::function<void(void)> fp_t;

	public:
			explicit dispatch_queue(std::string name, size_t thread_cnt = 1);
			~dispatch_queue();

			// dispatch and copy
			void dispatch(const fp_t& op);
			// dispatch and move
			void dispatch(fp_t&& op);

			// Deleted operations
			dispatch_queue(const dispatch_queue& rhs) = delete;
			dispatch_queue& operator=(const dispatch_queue& rhs) = delete;
			dispatch_queue(dispatch_queue&& rhs) = delete;
			dispatch_queue& operator=(dispatch_queue&& rhs) = delete;

	private:
			std::string name_;
			std::mutex lock_;
			std::vector<std::thread> threads_;
			std::queue<fp_t> q_;
			std::condition_variable cv_;
			bool quit_ = false;

			void dispatch_thread_handler(void);
	};
}


#endif //CPP_REDIS_DISPATCH_QUEUE_HPP
