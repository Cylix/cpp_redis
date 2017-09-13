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

#include <iostream>

#include <cpp_redis/misc/logger.hpp>

namespace cpp_redis {

std::unique_ptr<logger_iface> active_logger = nullptr;

static const char black[]  = {0x1b, '[', '1', ';', '3', '0', 'm', 0};
static const char red[]    = {0x1b, '[', '1', ';', '3', '1', 'm', 0};
static const char yellow[] = {0x1b, '[', '1', ';', '3', '3', 'm', 0};
static const char blue[]   = {0x1b, '[', '1', ';', '3', '4', 'm', 0};
static const char normal[] = {0x1b, '[', '0', ';', '3', '9', 'm', 0};

logger::logger(log_level level)
: m_level(level) {}

void
logger::debug(const std::string& msg, const std::string& file, std::size_t line) {
  if (m_level >= log_level::debug) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::cout << "[" << black << "DEBUG" << normal << "][cpp_redis][" << file << ":" << line << "] " << msg << std::endl;
  }
}

void
logger::info(const std::string& msg, const std::string& file, std::size_t line) {
  if (m_level >= log_level::info) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::cout << "[" << blue << "INFO " << normal << "][cpp_redis][" << file << ":" << line << "] " << msg << std::endl;
  }
}

void
logger::warn(const std::string& msg, const std::string& file, std::size_t line) {
  if (m_level >= log_level::warn) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::cout << "[" << yellow << "WARN " << normal << "][cpp_redis][" << file << ":" << line << "] " << msg << std::endl;
  }
}

void
logger::error(const std::string& msg, const std::string& file, std::size_t line) {
  if (m_level >= log_level::error) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::cerr << "[" << red << "ERROR" << normal << "][cpp_redis][" << file << ":" << line << "] " << msg << std::endl;
  }
}

void
debug(const std::string& msg, const std::string& file, std::size_t line) {
  if (active_logger)
    active_logger->debug(msg, file, line);
}

void
info(const std::string& msg, const std::string& file, std::size_t line) {
  if (active_logger)
    active_logger->info(msg, file, line);
}

void
warn(const std::string& msg, const std::string& file, std::size_t line) {
  if (active_logger)
    active_logger->warn(msg, file, line);
}

void
error(const std::string& msg, const std::string& file, std::size_t line) {
  if (active_logger)
    active_logger->error(msg, file, line);
}

} //! cpp_redis
