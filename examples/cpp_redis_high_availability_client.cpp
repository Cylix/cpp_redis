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

#include <cpp_redis/cpp_redis>

#include <iostream>

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

  client.connect("127.0.0.1", 6379, [](const std::string& host, std::size_t port, cpp_redis::client::connect_state status) {
    if (status == cpp_redis::client::connect_state::dropped) {
      std::cout << "client disconnected from " << host << ":" << port << std::endl;
    }
  },
    0, -1, 5000);

  // same as client.send({ "SET", "hello", "42" }, ...)
  client.set("hello", "42", [](cpp_redis::reply& reply) {
    std::cout << "set hello 42: " << reply << std::endl;
    // if (reply.is_string())
    //   do_something_with_string(reply.as_string())
  });

  while (true) {
    // same as client.send({ "DECRBY", "hello", 12 }, ...)
    client.incrby("hello", 12, [](cpp_redis::reply& reply) {
      std::cout << "incrby hello 12: " << reply << std::endl;
      // if (reply.is_integer())
      //   do_something_with_integer(reply.as_integer())
    });

    // same as client.send({ "GET", "hello" }, ...)
    client.get("hello", [](cpp_redis::reply& reply) {
      std::cout << "get hello: " << reply << std::endl;
      // if (reply.is_string())
      //   do_something_with_string(reply.as_string())
    });

    // commands are pipelined and only sent when client.commit() is called
    // client.commit();

    // synchronous commit, no timeout
    client.sync_commit();
    std::cout << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  }

// synchronous commit, timeout
// client.sync_commit(std::chrono::milliseconds(100));

#ifdef _WIN32
  WSACleanup();
#endif /* _WIN32 */

  return 0;
}
