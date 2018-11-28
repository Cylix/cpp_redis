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
// SOFTWARE.#ifndef CPP_REDIS_CONSUMER_HPP

#ifndef CPP_REDIS_CONSUMER_HPP
#define CPP_REDIS_CONSUMER_HPP

#include <string>
#include <cpp_redis/core/client.hpp>
#include <cpp_redis/misc/dispatch_queue.hpp>

namespace cpp_redis {

	//!
	//! reply callback called whenever a reply is received
	//! takes as parameter the received reply
	//!
	typedef std::function<void(const xmessage_t &)> consumer_callback_t;

	typedef struct consumer_callback_container {
			consumer_callback_t consumer_callback;
			acknowledgement_callback_t acknowledgement_callback;
	} consumer_callback_container_t;

	typedef std::map<std::string, consumer_callback_container_t> consumer_queue_t;

	class consumer {
	public:
			explicit consumer(std::string stream, std::string consumer, size_t max_concurrency = std::thread::hardware_concurrency());

			consumer &subscribe(const std::string &group,
			                    const consumer_callback_t &consumer_callback,
			                    const acknowledgement_callback_t &acknowledgement_callback = nullptr);

			//! \brief Connect to redis server
			//! \param host host to be connected to
			//! \param port port to be connected to
			//! \param connect_callback connect handler to be called on connect events (may be null)
			//! \param timeout_ms maximum time to connect
			//! \param max_reconnects maximum attempts of reconnection if connection dropped
			//! \param reconnect_interval_ms time between two attempts of reconnection
			void connect(
					const std::string &host = "127.0.0.1",
					std::size_t port = 6379,
					const connect_callback_t &connect_callback = nullptr,
					std::uint32_t timeout_ms = 0,
					std::int32_t max_reconnects = 0,
					std::uint32_t reconnect_interval_ms = 0);

			//!
			//! commit pipelined transaction
			//! that is, send to the network all commands pipelined by calling send() / subscribe() / ...
			//!
			//! \return current instance
			//!
			consumer &commit();

	private:
			std::string m_stream;
			std::string m_name;
			size_t m_max_concurrency;
			client m_client;
			client m_sub_client;
			consumer_queue_t m_task_queue;
			dispatch_queue_t m_proc_queue;

			bool is_ready = false;
			std::condition_variable m_cv;
			std::mutex m_cv_mutex;
	};

} // namespace cpp_redis

#endif //CPP_REDIS_CONSUMER_HPP
