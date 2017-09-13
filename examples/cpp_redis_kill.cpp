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
#include <sstream>

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

  cpp_redis::client client;

  client.connect("127.0.0.1", 6379, [](const std::string& host, std::size_t port, cpp_redis::client::connect_state status) {
    if (status == cpp_redis::client::connect_state::dropped) {
      std::cout << "client disconnected from " << host << ":" << port << std::endl;
    }
  });

  //! client kill ip:port
  client.client_list([&client](cpp_redis::reply& reply) {
    std::string addr;
    std::stringstream ss(reply.as_string());

    ss >> addr >> addr;

    std::string host = std::string(addr.begin() + addr.find('=') + 1, addr.begin() + addr.find(':'));
    int port         = std::stoi(std::string(addr.begin() + addr.find(':') + 1, addr.end()));

    client.client_kill(host, port, [](cpp_redis::reply& reply) {
      std::cout << reply << std::endl; //! OK
    });

    client.commit();
  });

  client.sync_commit();
  std::this_thread::sleep_for(std::chrono::seconds(1));

  if (!client.is_connected()) {
    client.connect("127.0.0.1", 6379, [](const std::string& host, std::size_t port, cpp_redis::client::connect_state status) {
      if (status == cpp_redis::client::connect_state::dropped) {
        std::cout << "client disconnected from " << host << ":" << port << std::endl;
      }
    });
  }

  //! client kill filter
  client.client_list([&client](cpp_redis::reply& reply) {
    std::string id_str;
    std::stringstream ss(reply.as_string());

    ss >> id_str;

    uint64_t id = std::stoi(std::string(id_str.begin() + id_str.find('=') + 1, id_str.end()));
    client.client_kill(id, false, cpp_redis::client::client_type::normal, [](cpp_redis::reply& reply) {
      std::cout << reply << std::endl; //! 1
    });

    client.commit();
  });

  client.sync_commit();
  std::this_thread::sleep_for(std::chrono::seconds(1));

#ifdef _WIN32
  WSACleanup();
#endif /* _WIN32 */

  return 0;
}
