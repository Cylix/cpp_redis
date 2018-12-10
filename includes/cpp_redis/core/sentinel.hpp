// The MIT License (MIT)
//
// Copyright (c) 2015-2017 Simon Ninon <simon.ninon@gmail.com>
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

#pragma once

#include <atomic>
#include <condition_variable>
#include <queue>
#include <utility>
#include <vector>

#include <cpp_redis/misc/logger.hpp>
#include <cpp_redis/network/redis_connection.hpp>

namespace cpp_redis {

/**
 * cpp_redis::sentinel is the class providing sentinel configuration.
 * It is meant to be used for sending sentinel-related commands to the remote server and receiving its replies.
 * It is also meant to be used with cpp_redis::client and cpp_redis::subscriber for high availability (automatic failover if reconnection is enabled).
 *
 */
	class sentinel {
	public:
/**
 * ctor & dtor
 *
 */
#ifndef __CPP_REDIS_USE_CUSTOM_TCP_CLIENT

			/**
			 * default ctor
			 *
			 */
			sentinel();

#endif /* __CPP_REDIS_USE_CUSTOM_TCP_CLIENT */

			/**
			 * custom ctor to specify custom tcp_client
			 *
			 * @param tcp_client tcp client to be used for network communications
			 *
			 */
			explicit sentinel(const std::shared_ptr<network::tcp_client_iface> &tcp_client);

			/**
			 * dtor
			 *
			 */
			~sentinel();

			/**
			 * copy ctor
			 *
			 */
			sentinel(const sentinel &) = delete;

			/**
			 * assignment operator
			 *
			 */
			sentinel &operator=(const sentinel &) = delete;

	public:
			/**
			 * callback to be called whenever a reply has been received
			 *
			 */
			typedef std::function<void(reply &)> reply_callback_t;

			/**
			 * send the given command
			 * the command is actually pipelined and only buffered, so nothing is sent to the network
			 * please call commit() to flush the buffer
			 *
			 * @param sentinel_cmd command to be sent
			 * @param callback callback to be called when reply is received for this command
			 * @return current instance
			 *
			 */
			sentinel &send(const std::vector<std::string> &sentinel_cmd, const reply_callback_t &callback = nullptr);

			/**
			 * commit pipelined transaction
			 * that is, send to the network all commands pipelined by calling send()
			 *
			 * @return current instance
			 *
			 */
			sentinel &commit();

			/**
			 * same as commit(), but synchronous
			 * will block until all pending commands have been sent and that a reply has been received for each of them and all underlying callbacks completed
			 *
			 * @return current instance
			 *
			 */
			sentinel &sync_commit();

			/**
			 * same as sync_commit, but with a timeout
			 * will simply block until it completes or timeout expires
			 *
			 * @return current instance
			 *
			 */
			template<class Rep, class Period>
			sentinel &
			sync_commit(const std::chrono::duration<Rep, Period> &timeout) {
				try_commit();

				std::unique_lock<std::mutex> lock_callback(m_callbacks_mutex);
				__CPP_REDIS_LOG(debug, "cpp_redis::sentinel waiting for callbacks to complete");
				if (!m_sync_condvar.wait_for(lock_callback, timeout, [=] {
						return m_callbacks_running == 0 && m_callbacks.empty();
				})) {
					__CPP_REDIS_LOG(debug, "cpp_redis::sentinel finished waiting for callback");
				} else {
					__CPP_REDIS_LOG(debug, "cpp_redis::sentinel timed out waiting for callback");
				}
				return *this;
			}

	public:
			/**
			 * add a sentinel definition. Required for connect() or get_master_addr_by_name() when autoconnect is enabled.
			 *
			 * @param host sentinel host
			 * @param port sentinel port
			 * @param timeout_ms maximum time to connect
			 * @return current instance
			 *
			 */
			sentinel &add_sentinel(const std::string &host, std::size_t port, std::uint32_t timeout_ms = 0);

			/**
			 * clear all existing sentinels.
			 *
			 */
			void clear_sentinels();

	public:
			/**
			 * disconnect from redis server
			 *
			 * @param wait_for_removal when sets to true, disconnect blocks until the underlying TCP client has been effectively removed from the io_service and that all the underlying callbacks have completed.
			 *
			 */
			void disconnect(bool wait_for_removal = false);

			/**
			 * @return whether we are connected to the redis server or not
			 *
			 */
			bool is_connected();

			/**
			 * handlers called whenever disconnection occurred
			 * function takes the sentinel current instance as parameter
			 *
			 */
			typedef std::function<void(sentinel &)> sentinel_disconnect_handler_t;

			/**
			 * Connect to 1st active sentinel we find. Requires add_sentinel() to be called first
			 * will use timeout set for each added sentinel independently
			 *
			 * @param disconnect_handler handler to be called whenever disconnection occurs
			 *
			 */
			void connect_sentinel(const sentinel_disconnect_handler_t &disconnect_handler = nullptr);

			/**
			 * Connect to named sentinel
			 *
			 * @param host host to be connected to
			 * @param port port to be connected to
			 * @param timeout_ms maximum time to connect
			 * @param disconnect_handler handler to be called whenever disconnection occurs
			 *
			 */
			void connect(
					const std::string &host,
					std::size_t port,
					const sentinel_disconnect_handler_t &disconnect_handler = nullptr,
					std::uint32_t timeout_ms = 0);

			/**
			 * Used to find the current redis master by asking one or more sentinels. Use high availability.
			 * Handles connect() and disconnect() automatically when autoconnect=true
			 * This method is synchronous. No need to call sync_commit() or process a reply callback.
			 * Call add_sentinel() before using when autoconnect==true
			 *
			 * @param name sentinel name
			 * @param host sentinel host
			 * @param port sentinel port
			 * @param autoconnect  autoconnect we loop through and connect/disconnect as necessary to sentinels that were added using add_sentinel().
			 * 											Otherwise we rely on the call to connect to a sentinel before calling this method.
			 * @return true if a master was found and fills in host and port output parameters, false otherwise
			 */
			bool get_master_addr_by_name(
					const std::string &name,
					std::string &host,
					std::size_t &port,
					bool autoconnect = true);

	public:
			sentinel &ckquorum(const std::string &name, const reply_callback_t &reply_callback = nullptr);

			sentinel &failover(const std::string &name, const reply_callback_t &reply_callback = nullptr);

			sentinel &flushconfig(const reply_callback_t &reply_callback = nullptr);

			sentinel &master(const std::string &name, const reply_callback_t &reply_callback = nullptr);

			sentinel &masters(const reply_callback_t &reply_callback = nullptr);

			sentinel &monitor(const std::string &name, const std::string &ip, std::size_t port, std::size_t quorum,
			                  const reply_callback_t &reply_callback = nullptr);

			sentinel &ping(const reply_callback_t &reply_callback = nullptr);

			sentinel &remove(const std::string &name, const reply_callback_t &reply_callback = nullptr);

			sentinel &reset(const std::string &pattern, const reply_callback_t &reply_callback = nullptr);

			sentinel &sentinels(const std::string &name, const reply_callback_t &reply_callback = nullptr);

			sentinel &set(const std::string &name, const std::string &option, const std::string &value,
			              const reply_callback_t &reply_callback = nullptr);

			sentinel &slaves(const std::string &name, const reply_callback_t &reply_callback = nullptr);

	public:
			/**
			 * store informations related to a sentinel
			 * typically, host, port and connection timeout
			 *
			 */
			class sentinel_def {
			public:
					/**
					 * ctor
					 *
					 */
					sentinel_def(std::string host, std::size_t port, std::uint32_t timeout_ms)
							: m_host(std::move(host)), m_port(port), m_timeout_ms(timeout_ms) {}

					/**
					 * dtor
					 *
					 */
					~sentinel_def() = default;

			public:
					/**
					 * @return sentinel host
					 *
					 */
					const std::string &
					get_host() const { return m_host; }

					/**
					 * @return sentinel port
					 *
					 */
					size_t
					get_port() const { return m_port; }

					/**
					 * @return timeout for sentinel
					 *
					 */
					std::uint32_t
					get_timeout_ms() const { return m_timeout_ms; }

					/**
					 * set connect timeout for sentinel in ms
					 * @param timeout_ms new value
					 *
					 */
					void
					set_timeout_ms(std::uint32_t timeout_ms) { m_timeout_ms = timeout_ms; }

			private:
					/**
					 * sentinel host
					 *
					 */
					std::string m_host;

					/**
					 * sentinel port
					 *
					 */
					std::size_t m_port;

					/**
					 * connect timeout config
					 *
					 */
					std::uint32_t m_timeout_ms;
			};

	public:
			/**
			 * @return sentinels
			 *
			 */
			const std::vector<sentinel_def> &get_sentinels() const;

			/**
			 * @return sentinels (non-const version)
			 *
			 */
			std::vector<sentinel_def> &get_sentinels();

	private:
			/**
			 * redis connection receive handler, triggered whenever a reply has been read by the redis connection
			 *
			 * @param connection redis_connection instance
			 * @param reply parsed reply
			 *
			 */
			void connection_receive_handler(network::redis_connection &connection, reply &reply);

			/**
			 * redis_connection disconnection handler, triggered whenever a disconnection occurred
			 *
			 * @param connection redis_connection instance
			 *
			 */
			void connection_disconnect_handler(network::redis_connection &connection);

			/**
			 * Call the user-defined disconnection handler
			 *
			 */
			void call_disconnect_handler();

/**
 * reset the queue of pending callbacks
 *
 */
			void clear_callbacks();

/**
 * try to commit the pending pipelined
 * if client is disconnected, will throw an exception and clear all pending callbacks (call clear_callbacks())
 *
 */
			void try_commit();

	private:
			/**
			 * A pool of 1 or more sentinels we ask to determine which redis server is the master.
			 *
			 */
			std::vector<sentinel_def> m_sentinels;

			/**
			 * tcp client for redis sentinel connection
			 *
			 */
			network::redis_connection m_client;

			/**
			 * queue of callback to process
			 *
			 */
			std::queue<reply_callback_t> m_callbacks;

			/**
			 * user defined disconnection handler to be called on disconnection
			 *
			 */
			sentinel_disconnect_handler_t m_disconnect_handler;

			/**
			 * callbacks thread safety
			 *
			 */
			std::mutex m_callbacks_mutex;

			/**
			 * condvar for callbacks updates
			 *
			 */
			std::condition_variable m_sync_condvar;

			/**
			 * number of callbacks currently being running
			 *
			 */
			std::atomic<unsigned int> m_callbacks_running;
	};

} // namespace cpp_redis
