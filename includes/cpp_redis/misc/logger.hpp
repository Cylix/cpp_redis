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

#include <memory>
#include <mutex>
#include <string>

namespace cpp_redis {

//! logger_iface
//! should be inherited by any class intended to be used for logging
class logger_iface {
public:
  //! ctor & dtor
  logger_iface(void)          = default;
  virtual ~logger_iface(void) = default;

  //! copy ctor & assignment operator
  logger_iface(const logger_iface&) = default;
  logger_iface& operator=(const logger_iface&) = default;

public:
  virtual void debug(const std::string& msg, const std::string& file, std::size_t line) = 0;
  virtual void info(const std::string& msg, const std::string& file, std::size_t line)  = 0;
  virtual void warn(const std::string& msg, const std::string& file, std::size_t line)  = 0;
  virtual void error(const std::string& msg, const std::string& file, std::size_t line) = 0;
};

//! default logger class provided by the library
class logger : public logger_iface {
public:
  //! log level
  enum class log_level {
    error = 0,
    warn  = 1,
    info  = 2,
    debug = 3
  };

public:
  //! ctor & dtor
  logger(log_level level = log_level::info);
  ~logger(void)          = default;

  //! copy ctor & assignment operator
  logger(const logger&) = default;
  logger& operator=(const logger&) = default;

public:
  void debug(const std::string& msg, const std::string& file, std::size_t line);
  void info(const std::string& msg, const std::string& file, std::size_t line);
  void warn(const std::string& msg, const std::string& file, std::size_t line);
  void error(const std::string& msg, const std::string& file, std::size_t line);

private:
  log_level m_level;
  std::mutex m_mutex;
};

//! variable containing the current logger
//! by default, not set (no logs)
extern std::unique_ptr<logger_iface> active_logger;

//! convenience functions used internaly to call the logger
void debug(const std::string& msg, const std::string& file, std::size_t line);
void info(const std::string& msg, const std::string& file, std::size_t line);
void warn(const std::string& msg, const std::string& file, std::size_t line);
void error(const std::string& msg, const std::string& file, std::size_t line);

//! convenience macro to log with file and line information
#ifdef __CPP_REDIS_LOGGING_ENABLED
#define __CPP_REDIS_LOG(level, msg) cpp_redis::level(msg, __FILE__, __LINE__);
#else
#define __CPP_REDIS_LOG(level, msg)
#endif /* __CPP_REDIS_LOGGING_ENABLED */

} //! cpp_redis
