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

#ifndef CPP_REDIS_CONSUMER_HPP
#define CPP_REDIS_CONSUMER_HPP

#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include <cpp_redis/core/sentinel.hpp>
#include <cpp_redis/misc/error.hpp>
#include <cpp_redis/network/redis_connection.hpp>
#include <cpp_redis/network/tcp_client_iface.hpp>
#include <cpp_redis/core/client.hpp>
#include <cpp_redis/helpers/generate_rand.hpp>
#include <cpp_redis/core/dispatch_queue.hpp>

namespace cpp_redis {

	typedef struct consumer_options {
			std::string name = generate_rand();
			std::string group_name;
			std::string session_name;
			std::int32_t max_concurrency = std::thread::hardware_concurrency();
			std::uint32_t read_delay_msecs = 100;
			std::uint32_t reconnect_interval_msecs = 0;
	} consumer_options_t;

	class consumer {
	public:
#ifndef __CPP_REDIS_USE_CUSTOM_TCP_CLIENT

			//! ctor
			consumer();
			consumer(consumer_options_t options);

#endif /* __CPP_REDIS_USE_CUSTOM_TCP_CLIENT */

			/**
			 * @brief custom ctor to specify custom tcp_client
			 * @param tcp_client tcp client to be used for network communications
			 */
			explicit consumer(const std::shared_ptr<network::tcp_client_iface> &tcp_client,
			                  const consumer_options_t &options);

			//! dtor
			~consumer();

			//! copy ctor
			consumer(const consumer &) = delete;

			//! assignment operator
			consumer &operator=(const consumer &) = delete;

	public:

			/**
			 * @brief high availability (re)connection states
			 * \* dropped: connection has dropped \* start: attempt of connection has started
			 * \* sleeping: sleep between two attempts
			 * \* ok: connected
			 * \* failed: failed to connect
			 * \* lookup failed: failed to retrieve master sentinel
			 * \* stopped: stop to try to reconnect
			 */
			enum class connect_state {
					dropped,
					start,
					sleeping,
					ok,
					failed,
					lookup_failed,
					stopped
			};

			enum class completion_state {
					not_started,
					failed,
					complete,
					running
			};

	public:
			/**
			 * @brief connect handler, called whenever a new connection even occurred
			 */
			typedef std::function<void(const std::string &host, std::size_t port, connect_state status)> connect_callback_t;

			/**
			 * @brief Connect to redis server
			 * @param host host to be connected to
			 * @param port port to be connected to
			 * @param connect_callback connect handler to be called on connect events (may be null)
			 * @param timeout_msecs maximum time to connect
			 * @param max_reconnects maximum attempts of reconnection if connection dropped
			 * @param reconnect_interval_msecs time between two attempts of reconnection
			 */
			void connect(
					const std::string &host = "127.0.0.1",
					std::size_t port = 6379,
					const connect_callback_t &connect_callback = nullptr,
					std::uint32_t timeout_msecs = 0,
					std::int32_t max_reconnects = 0,
					std::uint32_t reconnect_interval_msecs = 0);

			/**
			 * @brief Connect to redis server
			 * @param name sentinel name
			 * @param connect_callback connect handler to be called on connect events (may be null)
			 * @param timeout_msecs maximum time to connect
			 * @param max_reconnects maximum attempts of reconnection if connection dropped
			 * @param reconnect_interval_msecs time between two attempts of reconnection
			 */
			void connect(
					const std::string &name,
					const connect_callback_t &connect_callback = nullptr,
					std::uint32_t timeout_msecs = 0,
					std::int32_t max_reconnects = 0,
					std::uint32_t reconnect_interval_msecs = 0);

			/**
			 * @brief determines client connectivity
			 * @return whether we are connected to the redis server
			 */
			bool is_connected() const;

			/**
			 * @brief disconnect from redis server
			 * @param wait_for_removal when set to true, disconnect blocks until the underlying TCP client has been effectively removed from the io_service and that all the underlying callbacks have completed.
			 */
			void disconnect(bool wait_for_removal = false);

			/**
			 * @brief determines if reconnect is in progress
			 * @return whether an attempt to reconnect is in progress
			 */
			bool is_reconnecting() const;

			/**
			 * @brief stop any reconnect in progress
			 */
			void cancel_reconnect();

	public:
			/**
			 * @brief reply callback called whenever a reply is received, takes as parameter the received reply
			 */
			typedef std::function<void(reply &)> reply_callback_t;

			/**
			 * @brief ability to authenticate on the redis server if necessary
			 * this method should not be called repeatedly as the storage of reply_callback is NOT thread safe (only one reply callback is stored for the consumer client)
			 * calling repeatedly auth() is undefined concerning the execution of the associated callbacks
			 * @param password password to be used for authentication
			 * @param reply_callback callback to be called on auth completion (nullable)
			 * @return current instance
			 */
			consumer &auth(const std::string &password, const reply_callback_t &reply_callback = nullptr);

			/**
			 * @brief subscribe callback, called whenever a new message is published on a subscribed channel
			 * takes as parameter the channel and the message
			 */
			typedef std::function<void(const std::string &, const std::map<std::string, std::string> &)> read_callback_t;

			/**
			 * @brief acknowledgment callback called whenever a subscribe completes: takes as parameter the int returned by the redis server (usually the number of channels you are subscribed to)
			 */
			typedef std::function<void(int64_t)> acknowledgement_callback_t;

			/**
			 * @brief Subscribes to the given channel and:
			 *     \Acknowledge calls acknowledgement_callback once the server has acknowledged about the subscription.
			 *     \Callback calls subscribe_callback each time a message is published on this channel.
			 * \Note The command is not effectively sent immediately but stored in an internal buffer until commit() is called.
			 * @param session_name channel to subscribe
			 * @param callback callback to be called whenever a message is received for this channel
			 * @param acknowledgement_callback callback to be called on subscription completion (nullable)
			 * @return current instance
			 */
			consumer &subscribe(const std::string &session_name,
			                    const std::string &group_name,
			                    const read_callback_t &callback,
			                    const acknowledgement_callback_t &acknowledgement_callback = nullptr);

			/**
			 * @brief unsubscribe from the given channel
			 * \Note The command is not effectively sent immediately, but stored inside an internal buffer until commit() is called.
			 * @param session_name channel to unsubscribe from
			 * @return current instance
			 */
			consumer &unsubscribe(const std::string &session_name, const std::string &group_name);

			/**
			 * @brief commit pipelined transaction, that is, send to the network all commands pipelined by calling send() / subscribe() / ...
			 * @return current instance
			 */
			consumer &commit();

	public:
			/**
			 * add a sentinel definition. Required for connect() or get_master_addr_by_name() when autoconnect is enabled.
			 * @param host sentinel host
			 * @param port sentinel port
			 * @param timeout_msecs maximum time to connect
			 */
			void add_sentinel(const std::string &host, std::size_t port, std::uint32_t timeout_msecs = 0);

			/**
			 * retrieve sentinel for current client
			 * @return sentinel associated to current client
			 */
			const sentinel &get_sentinel() const;

			/**
			 * retrieve sentinel for current client
			 * non-const version
			 * @return sentinel associated to current client
			 */
			sentinel &get_sentinel();

			/**
			 * clear all existing sentinels.
			 */
			void clear_sentinels();

	private:
			/**
			 * struct to hold callbacks (read and ack) for a given group, consumer or stream
			 */
			struct callback_holder {
					read_callback_t read_callback;
					acknowledgement_callback_t acknowledgement_callback;
			};

	private:
			/**
			 * redis connection receive handler, triggered whenever a reply has been read by the redis connection
			 * @param connection redis_connection instance
			 * @param reply parsed reply
			 */
			void connection_receive_handler(network::redis_connection &connection, reply &reply);

			/**
			 * redis_connection disconnection handler, triggered whenever a disconnection occurred
			 * @param connection redis_connection instance
			 */
			void connection_disconnection_handler(network::redis_connection &connection);

			/**
			 * trigger the ack callback for matching channel/pattern
			 * check if reply is valid
			 * @param reply received reply
			 */
			void handle_acknowledgement_reply(const std::vector<reply> &reply);

			void read_reply_handler(network::redis_connection &connection, reply &reply);

			/**
			 * trigger the sub callback for all matching channels/patterns
			 * check if reply is valid
			 * @param reply received reply
			 */
			void read_reply_handler(const std::vector<reply> &reply);

			/**
			 * @brief find channel or pattern that is associated to the reply and call its ack callback
			 * @param session_name the name of the session that caused the issuance of this reply
			 * @param sessions list of channels or patterns to be searched for the received channel
			 * @param channels_mtx channels or patterns mtx to be locked for race condition
			 * @param nb_sessions redis server ack reply
			 */
			void
			call_acknowledgement_callback(const std::string &session_name,
			                              const std::map<std::string, callback_holder> &sessions,
			                              std::mutex &channels_mtx, int64_t nb_sessions);

	private:
			/**
			 * reconnect to the previously connected host
			 * automatically re authenticate and resubscribe to subscribed channel in case of success
			 */
			void reconnect();

			/**
			 * re authenticate to redis server based on previously used password
			 */
			void re_auth();

			/**
			 * @brief resubscribe (sub and psub) to previously subscribed channels/patterns
			 */
			void re_subscribe();

			/**
			 * @return whether a reconnection attempt should be performed
			 */
			bool should_reconnect() const;

			/**
			 * @brief sleep between two reconnect attempts if necessary
			 */
			void sleep_before_next_reconnect_attempt();

	private:
			/**
			 * @brief unprotected sub
			 * same as subscribe, but without any mutex lock
			 * @param session_name channel to subscribe
			 */
			void
			unprotected_read();

	private:
			consumer_options_t m_options;
			/**
			 * @brief server we are connected to
			 */
			std::string m_redis_server;
			/**
			 * @brief port we are connected to
			 */
			std::size_t m_redis_port = 0;
			/**
			 * @brief master name (if we are using sentinel) we are connected to
			 */
			std::string m_master_name;
			/**
			 * @brief password used to authenticate
			 */
			std::string m_password;
			/**
			 * @brief Name of the consumer
			 * Redis sessions use this as a reference for claims
			 */
			std::string m_name;
			std::string m_group_name;
			std::string m_session_name;

			/**
			 * @brief tcp client for redis connection
			 */
			client m_client;

			/**
			 * @brief redis sentinel
			 */
			cpp_redis::sentinel m_sentinel;

			/**
			 * @brief max time to connect
			 */
			std::uint32_t m_connect_timeout_msecs = 0;
			/**
			 * @brief max number of reconnection attempts
			 */
			std::int32_t m_max_reconnects = 0;
			/**
			 * @brief max thread pool size
			 */
			std::int32_t max_concurrency = std::thread::hardware_concurrency();
			/**
			 * @brief current number of attempts to reconnect
			 */
			std::int32_t m_current_reconnect_attempts = 0;
			/**
			 * @brief time between two reconnection attempts
			 */
			std::uint32_t m_reconnect_interval_msecs = 0;

			/**
			 * reconnection status
			 */
			std::atomic_bool m_reconnecting;
			/**
			 * to force cancel reconnection
			 */
			std::atomic_bool m_cancel;
			/**
			 * @brief Signal to stop all processing in case of failure or completion
			 */
			int32_t m_sig_end = 0;

			dispatch_queue m_dispatch_queue;

			/**
			 * @brief read result handler
			 */
			read_callback_t m_read_callback;

			/**
			 * @brief handler to be processed upon completion of the task
			 */
			acknowledgement_callback_t m_acknowledgement_callback;

			/**
			 * connect handler
			 */
			connect_callback_t m_connect_callback;

			/**
			 * @brief queue for storing new task payloads
			 */
			std::queue<std::string> m_completion_queue;

			/**
			 * @brief total number of tasks currently being processed
			 */
			std::atomic<int> m_processing_count;

			/**
			 * @brief completion queue thread safety
			 */
			std::mutex m_completion_queue_mutex;

			/**
			 * auth reply callback
			 */
			reply_callback_t m_auth_reply_callback;
	};

} // namespace cpp_redis

#endif //CPP_REDIS_CONSUMER_HPP
