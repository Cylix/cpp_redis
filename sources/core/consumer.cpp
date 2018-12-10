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

#include <functional>

using std::bind;
using namespace std::placeholders;

namespace cpp_redis {

	consumer_client_container::consumer_client_container() : ack_client(), poll_client() {
	}

	consumer::consumer(std::string stream, std::string consumer, size_t max_concurrency)
			: m_stream(std::move(stream)),
			  m_name(std::move(consumer)),
			  m_read_id("0"),
			  m_block_sec(-1),
			  m_max_concurrency(max_concurrency),
			  m_read_count(static_cast<int>(max_concurrency)),
			  m_callbacks() {
		// Supply the dispatch queue a callback to notify the queue when it is at max capacity
		m_dispatch_queue = dispatch_queue_ptr_t(
				new dispatch_queue(stream, [&](size_t size) {
						dispatch_changed_handler(size);
				}, max_concurrency));
		m_client = client_container_ptr_t(new consumer_client_container());
	}

	consumer &cpp_redis::consumer::subscribe(const std::string &group,
	                                         const consumer_callback_t &consumer_callback,
	                                         const acknowledgement_callback_t &acknowledgement_callback) {
		m_callbacks.insert({group,
		                    {consumer_callback,
		                     acknowledgement_callback}});
		return *this;
	}

	void consumer::dispatch_changed_handler(size_t size) {
		if (size >= m_max_concurrency) {
			dispatch_queue_full.store(true);
			dispatch_queue_changed.notify_all();
		}
	}

	void consumer::connect(const std::string &host, size_t port, const connect_callback_t &connect_callback,
	                       uint32_t timeout_ms, int32_t max_reconnects, uint32_t reconnect_interval_ms) {
		m_client->ack_client.connect(host, port, connect_callback, timeout_ms, max_reconnects, reconnect_interval_ms);
		m_client->poll_client.connect(host, port, connect_callback, timeout_ms, max_reconnects, reconnect_interval_ms);
	}

	void consumer::auth(const std::string &password,
	          const reply_callback_t &reply_callback) {
		m_client->ack_client.auth(password, reply_callback);
		m_client->poll_client.auth(password, reply_callback);
	}

	consumer &consumer::commit() {
		while (!is_ready) {
			if (!is_ready) {
				std::unique_lock<std::mutex> dispatch_lock(dispatch_queue_changed_mutex);
				dispatch_queue_changed.wait(dispatch_lock, [&]() {
						return !dispatch_queue_full.load();
				});
				m_read_count = static_cast<int>(m_max_concurrency - m_dispatch_queue->size());
				poll();
			}
		}
		return *this;
	}

	void consumer::poll() {
		for (auto &cb : m_callbacks) {
			m_client->poll_client.xreadgroup(
					{
							cb.first, m_name,
							{
									{m_stream},
									{m_read_id}
							},
							m_read_count, m_block_sec, false
					},
					[&](cpp_redis::reply &reply) {
							// The reply is an array if valid
							cpp_redis::xstream_reply s_reply(reply);
							if (!s_reply.is_null()) {
								__CPP_REDIS_LOG(2, "Stream " << s_reply)
								for (const auto &stream : s_reply) {
									for (auto &m : stream.Messages) {
										if (m_should_read_pending.load())
											m_read_id = m.get_id();
										try {
											m_dispatch_queue->dispatch(
													m,
													[&](const message_type &message) {
															auto response = cb.second.consumer_callback(message);

															// add results to result stream
															m_client->ack_client.xadd(
																	m_stream+":results",
																	"*",
																	response);

															// acknowledge task completion
															m_client->ack_client.xack(
																	m_stream,
																	cb.first,
																	{message.get_id()},
																	[&](const reply_t &r) {
																			if (r.is_integer()) {
																				auto ret_int = r.as_integer();
																				cb.second.acknowledgement_callback(ret_int);
																			}
																	}).sync_commit();
															return response;
													});
										} catch (std::exception &exc) {
											__CPP_REDIS_LOG(1,
											                "Processing failed for message id: " + m.get_id() +
											                "\nDetails: " + exc.what());
											throw exc;
										}
									}
								}
							} else {
								if (m_should_read_pending.load()) {
									m_should_read_pending.store(false);
									m_read_id = ">";
									// Set to block infinitely
									m_block_sec = 0;
									// Set to read 1
									m_read_count = 1;
								}
							}
							return;
					}).sync_commit(); //(std::chrono::milliseconds(1000));
		}
	}


} // namespace cpp_redis
