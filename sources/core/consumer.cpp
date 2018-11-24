// The MIT License (MIT)
//
// Copyright (c) 2015-2018 Nicholas Atkins <nbatkins@gmail.com>
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

#include <utility>

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

#include <cpp_redis/core/consumer.hpp>

namespace cpp_redis {

#ifndef __CPP_REDIS_USE_CUSTOM_TCP_CLIENT

	consumer::consumer()
			: m_name(generate_rand()), m_reconnecting(false), m_cancel(false),
			  m_dispatch_queue(m_name, std::thread::hardware_concurrency()), m_auth_reply_callback(nullptr) {
		__CPP_REDIS_LOG(debug, "cpp_redis::consumer created");
	}

	consumer::consumer(consumer_options_t options)
			: m_options(std::move(options)), m_client(), m_sentinel(), m_reconnecting(false), m_cancel(false),
			  m_dispatch_queue(m_options.name, std::thread::hardware_concurrency()), m_auth_reply_callback(nullptr) {
		__CPP_REDIS_LOG(debug, "cpp_redis::consumer created");
	}

#endif /* __CPP_REDIS_USE_CUSTOM_TCP_CLIENT */

	consumer::consumer(const std::shared_ptr<network::tcp_client_iface> &tcp_client, const consumer_options_t &options)
			: m_options(options), m_client(tcp_client), m_sentinel(tcp_client), m_reconnecting(false), m_cancel(false), m_processing_count(0),
			  m_dispatch_queue(m_options.name, std::thread::hardware_concurrency()), m_auth_reply_callback(nullptr) {
		__CPP_REDIS_LOG(debug, "cpp_redis::consumer created");
	}

	consumer::~consumer() {
		//! ensure we stopped reconnection attempts
		if (!m_cancel) {
			cancel_reconnect();
		}

		//! If for some reason sentinel is connected then disconnect now.
		if (m_sentinel.is_connected()) {
			m_sentinel.disconnect(true);
		}

		//! disconnect underlying tcp socket
		if (m_client.is_connected()) {
			m_client.disconnect(true);
		}

		__CPP_REDIS_LOG(debug, "cpp_redis::consumer destroyed");
	}


	void
	consumer::connect(
			const std::string &name,
			const connect_callback_t &connect_callback,
			std::uint32_t timeout_msecs,
			std::int32_t max_reconnects,
			std::uint32_t reconnect_interval_msecs) {
		//! Save for auto reconnects
		m_master_name = name;

		//! We rely on the sentinel to tell us which redis server is currently the master.
		if (m_sentinel.get_master_addr_by_name(name, m_redis_server, m_redis_port, true)) {
			connect(m_redis_server, m_redis_port, connect_callback, timeout_msecs, max_reconnects, reconnect_interval_msecs);
		} else {
			throw redis_error("cpp_redis::consumer::connect() could not find master for m_name " + name);
		}
	}


	void
	consumer::connect(
			const std::string &host, std::size_t port,
			const connect_callback_t &connect_callback,
			std::uint32_t timeout_msecs,
			std::int32_t max_reconnects,
			std::uint32_t reconnect_interval_msecs) {
		__CPP_REDIS_LOG(debug, "cpp_redis::consumer attempts to connect");

		//! Save for auto reconnects
		m_redis_server = host;
		m_redis_port = port;
		m_connect_callback = connect_callback;
		m_max_reconnects = max_reconnects;
		m_reconnect_interval_msecs = reconnect_interval_msecs;

		//! notify start
		if (m_connect_callback) {
			m_connect_callback(host, port, connect_state::start);
		}

		auto disconnection_handler = std::bind(&consumer::connection_disconnection_handler, this, std::placeholders::_1);
		auto receive_handler = std::bind(&consumer::connection_receive_handler, this, std::placeholders::_1,
		                                 std::placeholders::_2);
		m_client.connect();
		//m_client.connect(host, port, disconnection_handler, receive_handler, timeout_msecs);

		//! notify end
		if (m_connect_callback) {
			m_connect_callback(m_redis_server, m_redis_port, connect_state::ok);
		}

		__CPP_REDIS_LOG(info, "cpp_redis::consumer connected");
	}

	void
	consumer::add_sentinel(const std::string &host, std::size_t port, std::uint32_t timeout_msecs) {
		m_sentinel.add_sentinel(host, port, timeout_msecs);
	}

	const sentinel &
	consumer::get_sentinel() const {
		return m_sentinel;
	}

	sentinel &
	consumer::get_sentinel() {
		return m_sentinel;
	}

	void
	consumer::clear_sentinels() {
		m_sentinel.clear_sentinels();
	}

	void
	consumer::cancel_reconnect() {
		m_cancel = true;
	}

	consumer &
	consumer::auth(const std::string &password, const reply_callback_t &reply_callback) {
		__CPP_REDIS_LOG(debug, "cpp_redis::consumer attempts to authenticate");

		m_password = password;
		m_auth_reply_callback = reply_callback;

		m_client.auth(password);

		//m_client.send({"AUTH", password});

		__CPP_REDIS_LOG(info, "cpp_redis::consumer AUTH command sent");

		return *this;
	}

	void
	consumer::disconnect(bool wait_for_removal) {
		__CPP_REDIS_LOG(debug, "cpp_redis::consumer attempts to disconnect");
		m_client.disconnect(wait_for_removal);
		__CPP_REDIS_LOG(info, "cpp_redis::consumer disconnected");
	}

	bool
	consumer::is_connected() const {
		return m_client.is_connected();
	}

	bool
	consumer::is_reconnecting() const {
		return m_reconnecting;
	}

	consumer &
	consumer::subscribe(const std::string &session_name, const std::string &group_name, const read_callback_t &callback,
	                    const acknowledgement_callback_t &acknowledgement_callback) {
		m_options.session_name = session_name;
		m_options.group_name = group_name;
		m_read_callback = callback;

		__CPP_REDIS_LOG(debug, "cpp_redis::consumer attempts to subscribe to session_name " + session_name);
		/*std::thread sub_loop([this]{
				while(m_sig_end == 0) {
					std::cout << "Max concurrency: " << max_concurrency << std::endl;
					if (m_processing_count <= max_concurrency) {
						m_client.send({"XREADGROUP",
						               "BLOCK", "0",
						               "COUNT", "1",
						               "GROUP", m_options.group_name, m_options.name,
						               "STREAMS", "ars", ">", m_options.session_name});
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				}
		});*/

		while (m_sig_end == 0) {
			std::cout << "Processing count: " << m_processing_count << std::endl;
			if (m_processing_count <= m_options.max_concurrency) {
				std::cout << "Group" << m_options.group_name << "Name" << m_options.name << m_options.session_name << std::endl;
				xreadgroup_args_t args = {m_options.group_name,
				                          m_options.name,
				                          {{m_options.session_name}, {">"}},
				                          1, // Count
				                          0, // block milli
				                          false, // no ack
				};
				m_client.xreadgroup(args,
				                    [](const reply &message) {
						                    std::cout << "SOmthing here" << message << std::endl;
				                    });
				/*m_client.xreadgroup({"XREADGROUP",
				               "BLOCK", "0",
				               "COUNT", "1",
				               "GROUP", m_options.group_name, m_options.name,
				               "STREAMS", m_options.session_name, ">"}, [](const reply& message){
					std::cout << "SOmthing here" << message << std::endl;
				});*/
			}
			commit();
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
		__CPP_REDIS_LOG(info, "cpp_redis::consumer subscribed to session_name " + session_name);

		return *this;
	}

	void
	consumer::unprotected_read() {

		m_client.send({"XREADGROUP",
		               "BLOCK", "0",
		               "COUNT", "1",
		               "GROUP", m_group_name, m_name,
		               "STREAMS", "ars", ">", m_session_name});
	}

	consumer &
	consumer::unsubscribe(const std::string &session_name, const std::string &group_name) {
		__CPP_REDIS_LOG(debug, "cpp_redis::consumer attempts to unsubscribe from session_name " + session_name);

		m_client.send({"DELCONSUMER", session_name, group_name, m_name});
		__CPP_REDIS_LOG(info, "cpp_redis::consumer unsubscribed from session_name " + session_name);

		return *this;
	}

	consumer &
	consumer::commit() {
		try {
			__CPP_REDIS_LOG(debug, "cpp_redis::consumer attempts to send pipelined commands");
			m_client.commit();
			__CPP_REDIS_LOG(info, "cpp_redis::consumer sent pipelined commands");
		}
		catch (const cpp_redis::redis_error &) {
			__CPP_REDIS_LOG(error, "cpp_redis::consumer could not send pipelined commands");
			throw;
		}

		return *this;
	}

	void
	consumer::call_acknowledgement_callback(const std::string &session_name,
	                                        const std::map<std::string, callback_holder> &sessions,
	                                        std::mutex &channels_mtx, int64_t nb_sessions) {
		std::lock_guard<std::mutex> lock(channels_mtx);

		auto it = sessions.find(session_name);
		if (it == sessions.end())
			return;

		if (it->second.acknowledgement_callback) {
			__CPP_REDIS_LOG(debug, "cpp_redis::consumer executes acknowledgement callback for session_name " + session_name);
			it->second.acknowledgement_callback(nb_sessions);
		}
	}

	void
	consumer::handle_acknowledgement_reply(const std::vector<reply> &reply) {
		if (reply.size() != 3)
			return;

		const auto &title = reply[0];
		const auto &channel = reply[1];
		const auto &nb_sessions = reply[2];

		if (!title.is_string()
		    || !channel.is_string()
		    || !nb_sessions.is_integer())
			return;

		//if (title.as_string() == "subscribe")
		//	call_acknowledgement_callback(channel.as_string(), m_subscribed_sessions, m_subscribed_sessions_mutex, nb_sessions.as_integer());
	}

	void
	consumer::read_reply_handler(const std::vector<reply> &reply) {
		std::map<std::string, std::string> res;

		std::string id = reply[0].as_string();

		if (reply.size() > 1 && reply[1].is_array()) {
			res = reply[1].as_str_map();
			res["id"] = id;
		}

		m_dispatch_queue.dispatch([&]() {
				try {
					m_processing_count++;
					m_read_callback(id, res);
					m_processing_count--;
					std::unique_lock<std::mutex> cq_lock(m_completion_queue_mutex);
					m_completion_queue.push(id);
				} catch (const std::exception &exc) {
					m_processing_count--;
				}

		});

		return;

		/*std::lock_guard<std::mutex> lock(m_subscribed_sessions_mutex);

		auto it = m_subscribed_sessions.find(channel.as_string());
		if (it == m_subscribed_sessions.end())
			return;*/

		__CPP_REDIS_LOG(debug, "cpp_redis::consumer executes subscribe callback for channel " + channel.as_string());
		//it->second.read_callback(channel.as_string(), message.as_string());
	}

	void
	consumer::connection_receive_handler(network::redis_connection &, reply &reply) {
		__CPP_REDIS_LOG(info, "cpp_redis::consumer received reply");

		//! always return an array
		//! otherwise, if auth was defined, this should be the AUTH reply
		//! any other replies from the server are considered as unexpected
		if (!reply.is_array()) {
			if (m_auth_reply_callback) {
				__CPP_REDIS_LOG(debug, "cpp_redis::consumer executes auth callback");

				m_auth_reply_callback(reply);
				m_auth_reply_callback = nullptr;
			}

			return;
		}

		auto &array = reply.as_array();

		//! Array size of 3 -> SUBSCRIBE if array[2] is a string
		//! Array size of 3 -> AKNOWLEDGEMENT if array[2] is an integer
		//! Array size of 4 -> PSUBSCRIBE
		//! Otherwise -> unexpected reply
		if (array.size() == 3 && array[2].is_integer())
			handle_acknowledgement_reply(array);
		else if (array.size() == 3 && array[2].is_string())
			read_reply_handler(array);
	}

	void
	consumer::connection_disconnection_handler(network::redis_connection &) {
		//! leave right now if we are already dealing with reconnection
		if (is_reconnecting()) {
			return;
		}

		//! initiate reconnection process
		m_reconnecting = true;
		m_current_reconnect_attempts = 0;

		__CPP_REDIS_LOG(warn, "cpp_redis::consumer has been disconnected");

		if (m_connect_callback) {
			m_connect_callback(m_redis_server, m_redis_port, connect_state::dropped);
		}

		//! Lock the callbacks mutex of the base class to prevent more consumer commands from being issued until our reconnect has completed.
		//std::lock_guard<std::mutex> sub_lock_callback(m_subscribed_sessions_mutex);

		while (should_reconnect()) {
			sleep_before_next_reconnect_attempt();
			reconnect();
		}

		/*if (!is_connected()) {
			clear_subscriptions();

			//! Tell the user we gave up!
			if (m_connect_callback) {
				m_connect_callback(m_redis_server, m_redis_port, connect_state::stopped);
			}
		}*/

		//! terminate reconnection
		m_reconnecting = false;
	}

	void
	consumer::sleep_before_next_reconnect_attempt() {
		if (m_reconnect_interval_msecs <= 0) {
			return;
		}

		if (m_connect_callback) {
			m_connect_callback(m_redis_server, m_redis_port, connect_state::sleeping);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(m_reconnect_interval_msecs));
	}

	bool
	consumer::should_reconnect() const {
		return !is_connected() && !m_cancel && (m_max_reconnects == -1 || m_current_reconnect_attempts < m_max_reconnects);
	}

	void
	consumer::reconnect() {
		//! increase the number of attempts to reconnect
		++m_current_reconnect_attempts;

		//! We rely on the sentinel to tell us which redis server is currently the master.
		if (!m_master_name.empty() &&
		    !m_sentinel.get_master_addr_by_name(m_master_name, m_redis_server, m_redis_port, true)) {
			if (m_connect_callback) {
				m_connect_callback(m_redis_server, m_redis_port, connect_state::lookup_failed);
			}
			return;
		}

		//! Try catch block because the redis consumer throws an error if connection cannot be made.
		try {
			connect(m_redis_server, m_redis_port, m_connect_callback, m_connect_timeout_msecs, m_max_reconnects,
			        m_reconnect_interval_msecs);
		}
		catch (...) {
		}

		if (!is_connected()) {
			if (m_connect_callback) {
				m_connect_callback(m_redis_server, m_redis_port, connect_state::failed);
			}
			return;
		}

		//! notify end
		if (m_connect_callback) {
			m_connect_callback(m_redis_server, m_redis_port, connect_state::ok);
		}

		__CPP_REDIS_LOG(info, "client reconnected ok");

		re_auth();
		re_subscribe();
		commit();
	}

	void
	consumer::re_subscribe() {
		unprotected_read();
	}

	void
	consumer::re_auth() {
		if (m_password.empty()) {
			return;
		}

		auth(m_password, [&](cpp_redis::reply &reply) {
				if (reply.is_string() && reply.as_string() == "OK") {
					__CPP_REDIS_LOG(warn, "consumer successfully re-authenticated");
				} else {
					__CPP_REDIS_LOG(warn, std::string("consumer failed to re-authenticate: " + reply.as_string()).c_str());
				}
		});
	}

} // namespace cpp_redis

