#include <utility>

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
// SOFTWARE.#include "consumer.hpp"



#include <cpp_redis/core/consumer.hpp>

namespace cpp_redis {

	consumer::consumer(std::string stream, std::string consumer, size_t max_concurrency)
			: m_stream(std::move(stream)),
			  m_name(std::move(consumer)),
			  m_max_concurrency(max_concurrency),
			  m_proc_queue(stream, max_concurrency) {

	}

	consumer &cpp_redis::consumer::subscribe(const std::string &group,
	                                         const consumer_callback_t &consumer_callback,
	                                         const acknowledgement_callback_t &acknowledgement_callback) {
		m_task_queue[group] = {consumer_callback, acknowledgement_callback};
		return *this;
	}

	void consumer::connect(const std::string &host, size_t port, const connect_callback_t &connect_callback,
	                       uint32_t timeout_ms, int32_t max_reconnects, uint32_t reconnect_interval_ms) {

		m_client.connect(host, port, connect_callback, timeout_ms, max_reconnects, reconnect_interval_ms);
		m_client.connect(host, port, connect_callback, timeout_ms, max_reconnects, reconnect_interval_ms);
	}

	consumer &consumer::commit() {
		// Set the consumer id to 0 so that we start with failed messages
		std::string consumer_name = "0";

		std::unique_lock<std::mutex> cv_mutex_lock(m_cv_mutex);
		while (!is_ready) {
			m_cv.wait(cv_mutex_lock);
			if (!is_ready)
			if (m_max_concurrency > m_proc_queue.size()) {

				for (auto q : m_task_queue) {
					auto cb = q.second;
					auto group = q.first;
					m_sub_client.xreadgroup({group, consumer_name, {{m_stream}, {">"}}
					                         ,1, -1, false} // count, block, no_ack
					                         ,[&](cpp_redis::reply &reply) {
							cpp_redis::xstream_reply xs(reply);
							if (xs.empty()) {
								if (consumer_name == "0") {
									consumer_name = m_name;
								}
							} else {
								for (auto s : xs) {
									for (const auto &m : s.Messages) {
										try {
											cb.consumer_callback(m);
											m_client.xack(m_stream, group, {m.Id}); // Acknowledge reply
										} catch (std::exception &exc) {
											__CPP_REDIS_LOG(1, "Processing failed for message id: " + m.Id + "\nDetails: " + exc.what())
										}
									}
								}
							}
					});
				}
			}
		}
		return *this;
	}

} // namespace cpp_redis
