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

#ifndef CPP_REDIS_CORE_TYPES_HPP
#define CPP_REDIS_CORE_TYPES_HPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <chrono>
#include <cpp_redis/core/reply.hpp>
#include <functional>
#include <cpp_redis/impl/types.hpp>


namespace cpp_redis {
	typedef std::int64_t ms;
/**
 * @brief first array is the session name, second is ids
 *
 */
	typedef std::pair<std::vector<std::string>, std::vector<std::string>> streams_t;

	/**
	 * @brief Options
	 */

	typedef struct xread_options {
			streams_t Streams;
			std::int64_t Count;
			std::int64_t Block;
	} xread_options_t;

	typedef struct xreadgroup_options {
			std::string Group;
			std::string Consumer;
			streams_t Streams;
			std::int64_t Count;
			std::int64_t Block;
			bool NoAck;
	} xreadgroup_options_t;

	typedef struct range_options {
			std::string Start;
			std::string Stop;
			std::int64_t Count;
	} range_options_t;

	typedef struct xclaim_options {
			std::int64_t Idle;
			std::time_t *Time;
			std::int64_t RetryCount;
			bool Force;
			bool JustId;
	} xclaim_options_t;

	typedef struct xpending_options {
			range_options_t Range;
			std::string Consumer;
	} xpending_options_t;

	/**
	 * @brief Replies
	 */

	class xmessage : public message_type {
	public:
			xmessage();

			explicit xmessage(const reply_t &data);

			friend std::ostream &operator<<(std::ostream &os, const xmessage &xm);
	};

	typedef xmessage xmessage_t;

	class xstream {
	public:
			explicit xstream(const reply_t &data);

			friend std::ostream &operator<<(std::ostream &os, const xstream &xs);

			std::string Stream;
			std::vector<xmessage_t> Messages;
	};

	typedef xstream xstream_t;

	class xinfo_reply {
	public:
			explicit xinfo_reply(const cpp_redis::reply &data);

			std::int64_t Length;
			std::int64_t RadixTreeKeys;
			std::int64_t RadixTreeNodes;
			std::int64_t Groups;
			std::string LastGeneratedId;
			xmessage_t FirstEntry;
			xmessage_t LastEntry;
	};

	class xstream_reply : public std::vector<xstream_t> {
	public:
			explicit xstream_reply(const reply_t &data);

			friend std::ostream &operator<<(std::ostream &os, const xstream_reply &xs);

	bool is_null() const {
		if (empty())
			return true;
		for (auto &v : *this) {
			if (v.Messages.empty())
				return true;
		}
		return false;
	}
	};

	typedef xstream_reply xstream_reply_t;

	/**
	 * @brief Callbacks
	 */

/**
 * acknowledgment callback called whenever a subscribe completes
 * takes as parameter the int returned by the redis server (usually the number of channels you are subscribed to)
 *
 */
	typedef std::function<void(const int64_t &)> acknowledgement_callback_t;

/**
 * high availability (re)connection states
 *  * dropped: connection has dropped
 *  * start: attempt of connection has started
 *  * sleeping: sleep between two attempts
 *  * ok: connected
 *  * failed: failed to connect
 *  * lookup failed: failed to retrieve master sentinel
 *  * stopped: stop to try to reconnect
 *
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

/**
 * connect handler, called whenever a new connection even occurred
 *
 */
	typedef std::function<void(const std::string &host, std::size_t port, connect_state status)> connect_callback_t;

	typedef std::function<void(const cpp_redis::message_type&)> message_callback_t;
} // namespace cpp_redis


#endif //CPP_REDIS_TYPES_HPP
