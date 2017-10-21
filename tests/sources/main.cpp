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

#include <gtest/gtest.h>

#ifdef _WIN32
#include <Winsock2.h>
#endif /* _WIN32 */

//! For debugging purpose, uncomment
// #include <cpp_redis/cpp_redis>
// #include <memory>
// #include <tacopie/tacopie>

int
main(int argc, char** argv) {
//! For debugging purpose, uncomment
// cpp_redis::active_logger = std::unique_ptr<cpp_redis::logger>(new cpp_redis::logger(cpp_redis::logger::log_level::debug));
// tacopie::active_logger   = std::unique_ptr<tacopie::logger>(new tacopie::logger(tacopie::logger::log_level::debug));

#ifdef _WIN32
  //! Windows network DLL init
  WORD version = MAKEWORD(2, 2);
  WSADATA data;

  if (WSAStartup(version, &data) != 0) {
    std::cerr << "WSAStartup() failure" << std::endl;
    return -1;
  }
#endif /* _WIN32 */

  ::testing::InitGoogleTest(&argc, argv);

  int ret = RUN_ALL_TESTS();

#ifdef _WIN32
  WSACleanup();
#endif /* _WIN32 */

  return ret;
}
