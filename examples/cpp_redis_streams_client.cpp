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
#include <string>
#include <cpp_redis/cpp_redis>

#ifdef _WIN32
#include <Winsock2.h>
#endif /* _WIN32 */

int
main(void) {
#ifdef _WIN32
	//! Windows netword DLL init
	WORD version = MAKEWORD(2, 2);
	WSADATA data;

	if (WSAStartup(version, &data) != 0) {
		std::cerr << "WSAStartup() failure" << std::endl;
		return -1;
	}
#endif /* _WIN32 */

	//! Enable logging
	cpp_redis::active_logger = std::unique_ptr<cpp_redis::logger>(new cpp_redis::logger);

	cpp_redis::client client;

	client.connect("127.0.0.1", 6379,
	               [](const std::string &host, std::size_t port, cpp_redis::connect_state status) {
			               if (status == cpp_redis::connect_state::dropped) {
				               std::cout << "client disconnected from " << host << ":" << port << std::endl;
			               }
	               });

	auto reply_cmd = [](cpp_redis::reply &reply) {
			std::cout << "response: " << reply << std::endl;
	};

	std::string message_id;

	const std::string group_name = "groupone";
	const std::string session_name = "sessone";
	const std::string consumer_name = "ABCD";

	std::multimap<std::string, std::string> ins;
	ins.insert(std::pair<std::string, std::string>{"message", "hello"});
	ins.insert(std::pair<std::string, std::string>{"result", "a result"});

	client.xtrim(session_name, 10, reply_cmd);

	client.xgroup_create(session_name, group_name, reply_cmd);

	client.xadd(session_name, "*", {{"message", "hello"},
	                                {"details", "some details"}}, [&](cpp_redis::reply &reply) {
			std::cout << "response: " << reply << std::endl;
			message_id = reply.as_string();
			std::cout << "message id: " << message_id << std::endl;
	});

	client.sync_commit();

	std::cout << "message id after: " << message_id << std::endl;

	client.xack(session_name, group_name, {message_id}, reply_cmd);
	client.xinfo_stream(session_name, [](cpp_redis::reply &reply) {
			//std::cout << reply << std::endl;
			cpp_redis::xinfo_reply x(reply);
			std::cout << "Len: " << x.Length << std::endl;
	});
	//client.xadd(session_name, message_id, {{"final", "finished"}}, reply_cmd);

	client.sync_commit(std::chrono::milliseconds(100));

	client.xread({Streams: {{session_name},
	                        {message_id}},
			             Count: 10,
			             Block: 100}, reply_cmd);

	client.sync_commit(std::chrono::milliseconds(100));

	client.xrange(session_name, {"-", "+", 10}, reply_cmd);

	client.xreadgroup({group_name,
	                   "0",
	                   {{session_name}, {">"}},
	                   1, -1, false // count, block, no_ack
	                  }, [](cpp_redis::reply &reply) {
			cpp_redis::xstream_reply msg(reply);
			std::cout << msg << std::endl;
	});

	client.sync_commit(std::chrono::milliseconds(100));

	client.xreadgroup({group_name,
	                   consumer_name,
	                   {{session_name}, {">"}},
	                   1, 0, false // count, block, no_ack
	                  }, [](cpp_redis::reply &reply) {
			cpp_redis::xstream_reply msg(reply);
			std::cout << msg << std::endl;
	});

	// commands are pipelined and only sent when client.commit() is called
	// client.commit();

	// synchronous commit, no timeout
	client.sync_commit(std::chrono::milliseconds(100));

	// synchronous commit, timeout
	// client.sync_commit(std::chrono::milliseconds(100));

#ifdef _WIN32
	WSACleanup();
#endif /* _WIN32 */

	return 0;
}
