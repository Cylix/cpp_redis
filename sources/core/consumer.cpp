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
			  m_max_concurrency(max_concurrency),
			  m_callbacks() {
		m_dispatch_queue = dispatch_queue_ptr_t(new dispatch_queue(stream, [&](size_t size) {
				dispatch_changed_handler(size);
		}, max_concurrency));
		m_client = client_container_ptr_t(new consumer_client_container());
	}

	consumer &cpp_redis::consumer::subscribe(const std::string &group,
	                                         const consumer_callback_t &consumer_callback,
	                                         const acknowledgement_callback_t &acknowledgement_callback) {
		//std::lock_guard<std::mutex> task_queue_lock(m_callbacks_mutex);
		m_callbacks.insert({group, {consumer_callback, acknowledgement_callback}});
		return *this;
	}

	void consumer::dispatch_changed_handler(size_t size) {
		if (size >= m_max_concurrency) {
			dispatch_queue_full.store(true);
			dispatch_queue_changed.notify_all();

			std::cout << "Notified" <<
			          std::endl;
		}
	}

	void consumer::connect(const std::string &host, size_t port, const connect_callback_t &connect_callback,
	                       uint32_t timeout_ms, int32_t max_reconnects, uint32_t reconnect_interval_ms) {
		m_client->ack_client.connect(host, port, connect_callback, timeout_ms, max_reconnects, reconnect_interval_ms);
		m_client->poll_client.connect(host, port, connect_callback, timeout_ms, max_reconnects, reconnect_interval_ms);
	}

	consumer &consumer::commit() {
		std::thread reply_thread([&]() { process(); });
		while (!is_ready) {
			if (!is_ready) {
				std::unique_lock<std::mutex> dispatch_lock(dispatch_queue_changed_mutex);
				dispatch_queue_changed.wait(dispatch_lock, [&]() { return !dispatch_queue_full.load(); });
				poll();
			}
		}
		return *this;
	}

	void consumer::poll() {
		for (auto &cb : m_callbacks) {
			read_group_handler({cb.first, m_name, {{m_stream}, {">"}}, 1, 0, false}); // count, block (-1 for no block), no_ack
		}
		//std::lock_guard<std::mutex> task_queue_lock(m_callbacks_mutex);
		//std::string consumer_name = m_name;
		//std::string consumer_name = (is_new ? "0" : m_name);
		//auto q = m_.find("groupone");
		//task_queue_lock.lock();
		//auto group = q->first;
		//auto cb_container = q->second;
		//task_queue_lock.unlock();
	}

	void consumer::read_group_handler(const xreadgroup_options_t &a) {
		m_client->poll_client.xreadgroup(a, [&](cpp_redis::reply &reply) {
				cpp_redis::xstream_reply xs(reply);
				if (xs.empty())
					if (m_should_read_pending.load())
						m_should_read_pending.store(false);
				// Notify the reply reader of changes
				push_reply(reply);
		}).sync_commit();
	}

	void consumer::push_reply(const reply_t &reply) {
		std::lock_guard<std::mutex> m_replies_lock(m_replies_mutex);
		m_replies.push(reply);
		if (replies_empty.load())
			replies_empty.store(false);
		replies_changed.notify_all();
	}

	reply_t consumer::pop_reply() {
		std::lock_guard<std::mutex> pop_replies_lock(m_replies_mutex);
		defer _(nullptr, [&](...){ m_replies.pop(); });
		return m_replies.back();
	}

	void consumer::process() {
		while (!is_ready) {
			if (!is_ready) {
				std::unique_lock<std::mutex> process_replies_lock(replies_changed_mutex);
				replies_changed.wait(process_replies_lock, [&]() { return !replies_empty.load(); });

				std::cout << "Process" << std::endl;
				auto r = pop_reply();
				xstream_reply streams(r);
				stream_reply_handler(streams);
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}
		}
	}

	void consumer::stream_reply_handler(const xstream_reply& streams) {
		for (auto stream : streams) {
			for (auto &m : stream.Messages) {
				try {
					std::string group_id = m.get_id();
					auto task = m_callbacks.find(group_id);
					auto callback_container = task->second;

					auto callback = [&](const message_type &message) {
							auto response = callback_container.consumer_callback(message);
							m_client->ack_client.xack(m_stream, group_id, {message.get_id()}, [&](const reply &r) {
									if (r.is_integer())
										callback_container.acknowledgement_callback(r.as_integer());
							}).commit();
							return response;
					};
					m_dispatch_queue->dispatch(m, callback);
				} catch (std::exception &exc) {
					__CPP_REDIS_LOG(1, "Processing failed for message id: " + m.get_id() + "\nDetails: " + exc.what());
					throw exc;
				}
			}
		}
	}

} // namespace cpp_redis
