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

#include <cpp_redis/core/dispatch_queue.hpp>

namespace cpp_redis {

	dispatch_queue::dispatch_queue(std::string name, size_t thread_cnt) :
			name_(name), threads_(thread_cnt) {
		printf("Creating dispatch queue: %s\n", name.c_str());
		printf("Dispatch threads: %zu\n", thread_cnt);

		for (auto &i : threads_) {
			i = std::thread(&dispatch_queue::dispatch_thread_handler, this);
		}
	}

	dispatch_queue::~dispatch_queue() {
		printf("Destructor: Destroying dispatch threads...\n");

		// Signal to dispatch threads that it's time to wrap up
		std::unique_lock<std::mutex> lock(lock_);
		quit_ = true;
		lock.unlock();
		cv_.notify_all();

		// Wait for threads to finish before we exit
		for (size_t i = 0; i < threads_.size(); i++) {
			if (threads_[i].joinable()) {
				printf("Destructor: Joining thread %zu until completion\n", i);
				threads_[i].join();
			}
		}
	}

	void dispatch_queue::dispatch(const fp_t &op) {
		std::unique_lock<std::mutex> lock(lock_);
		q_.push(op);

		// Manual unlocking is done before notifying, to avoid waking up
		// the waiting thread only to block again (see notify_one for details)
		lock.unlock();
		cv_.notify_all();
	}

	void dispatch_queue::dispatch(fp_t &&op) {
		std::unique_lock<std::mutex> lock(lock_);
		q_.push(std::move(op));

		// Manual unlocking is done before notifying, to avoid waking up
		// the waiting thread only to block again (see notify_one for details)
		lock.unlock();
		cv_.notify_all();
	}

	void dispatch_queue::dispatch_thread_handler() {
		std::unique_lock<std::mutex> lock(lock_);

		do {
			//Wait until we have data or a quit signal
			cv_.wait(lock, [this] {
					return (!q_.empty() || quit_);
			});

			//after wait, we own the lock
			if (!quit_ && !q_.empty()) {
				auto op = std::move(q_.front());
				q_.pop();

				//unlock now that we're done messing with the queue
				lock.unlock();

				op();

				lock.lock();
			}
		} while (!quit_);
	}
}