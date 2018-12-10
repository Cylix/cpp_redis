#include <utility>

/*
 * Created by nick on 11/22/18.
 * Copyright(c) 2018 Iris. All rights reserved.
 * Use and copying of this software and preparation of derivative
 * works based upon this software are  not permitted.  Any distribution
 * of this software or derivative works must comply with all applicable
 * Canadian export control laws.
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
 */

#include <cpp_redis/misc/dispatch_queue.hpp>

namespace cpp_redis {

	dispatch_queue::dispatch_queue(std::string name, const notify_callback_t &notify_callback, size_t thread_cnt) :
			m_name(name), m_threads(thread_cnt), m_mq(), notify_handler(notify_callback) {
		printf("Creating dispatch queue: %s\n", name.c_str());
		printf("Dispatch threads: %zu\n", thread_cnt);

		for (auto &i : m_threads) {
			i = std::thread(&dispatch_queue::dispatch_thread_handler, this);
		}
	}

	dispatch_queue::~dispatch_queue() {
		printf("Destructor: Destroying dispatch threads...\n");

		// Signal to dispatch threads that it's time to wrap up
		std::unique_lock<std::mutex> lock(m_threads_lock);
		m_quit = true;
		lock.unlock();
		m_cv.notify_all();

		// Wait for threads to finish before we exit
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i].joinable()) {
				printf("Destructor: Joining thread %zu until completion\n", i);
				m_threads[i].join();
			}
		}
	}

	void dispatch_queue::dispatch(const cpp_redis::message_type& message, const dispatch_callback_t &op) {
		std::unique_lock<std::mutex> lock(m_threads_lock);
		m_mq.push({op, message});

		// Manual unlocking is done before notifying, to avoid waking up
		// the waiting thread only to block again (see notify_one for details)
		lock.unlock();
		m_cv.notify_all();
	}

	void dispatch_queue::dispatch(const cpp_redis::message_type& message, dispatch_callback_t &&op) {
		std::unique_lock<std::mutex> lock(m_threads_lock);
		m_mq.push({std::move(op), message});

		// Manual unlocking is done before notifying, to avoid waking up
		// the waiting thread only to block again (see notify_one for details)
		lock.unlock();
		m_cv.notify_all();
	}

	void dispatch_queue::dispatch_thread_handler() {
		std::unique_lock<std::mutex> lock(m_threads_lock);

		do {
			//Wait until we have data or a quit signal
			m_cv.wait(lock, [this] {
					return (!m_mq.empty() || m_quit);
			});

			notify_handler(m_mq.size());

			//after wait, we own the lock
			if (!m_quit && !m_mq.empty()) {
				auto op = std::move(m_mq.front());
				m_mq.pop();

				//unlock now that we're done messing with the queue
				lock.unlock();

				auto vals = op.message.get_values();

				for (auto v : vals) {
					std::cout << v.second << std::endl;
				}

				auto res = op.callback(op.message);

				lock.lock();
			}
		} while (!m_quit);
	}

	size_t dispatch_queue::size() {
		std::lock_guard<std::mutex> mq_lock(m_mq_mutex);
		long res = m_mq.size();
		//unlock now that we're done messing with the queue
		//mq_lock.unlock();
		return static_cast<size_t>(res);
	}
}