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
#include <signal.h>

#ifdef _WIN32
#include <Winsock2.h>
#endif /* _WIN32 */

volatile std::atomic<bool> should_exit = ATOMIC_VAR_INIT(false);

void
sigint_handler(int) {
  should_exit = true;
}

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

  cpp_redis::redis_subscriber sub;

  sub.connect("127.0.0.1", 6379, [](cpp_redis::redis_subscriber&) {
    std::cout << "sub disconnected (disconnection handler)" << std::endl;
    should_exit = true;
  });

  //! authentication if server-server requires it
  // sub.auth("some_password", [](const cpp_redis::reply& reply) {
  //   if (reply.is_error()) { std::cerr << "Authentication failed: " << reply.as_string() << std::endl; }
  //   else {
  //     std::cout << "successful authentication" << std::endl;
  //   }
  // });

  sub.subscribe("some_chan", [](const std::string& chan, const std::string& msg) {
    std::cout << "MESSAGE " << chan << ": " << msg << std::endl;
  });
  sub.psubscribe("*", [](const std::string& chan, const std::string& msg) {
    std::cout << "PMESSAGE " << chan << ": " << msg << std::endl;
  });
  sub.commit();

  signal(SIGINT, &sigint_handler);
  while (!should_exit) {}

#ifdef _WIN32
  WSACleanup();
#endif /* _WIN32 */

  return 0;
}
