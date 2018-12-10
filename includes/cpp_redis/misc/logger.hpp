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

/**
 * logger_iface
 * should be inherited by any class intended to be used for logging
 *
 */
class logger_iface {
public:
/**
 * ctor
 *
 */
  logger_iface() = default;
/**
 * dtor
 *
 */
  virtual ~logger_iface() = default;

/**
 * copy ctor
 *
 */
  logger_iface(const logger_iface&) = default;
/**
 * assignment operator
 *
 */
  logger_iface& operator=(const logger_iface&) = default;

public:
/**
 * debug logging
 *
 * @param msg message to be logged
 * @param file file from which the message is coming
 * @param line line in the file of the message
 *
 */
  virtual void debug(const std::string& msg, const std::string& file, std::size_t line) = 0;

/**
 * info logging
 *
 * @param msg message to be logged
 * @param file file from which the message is coming
 * @param line line in the file of the message
 *
 */
  virtual void info(const std::string& msg, const std::string& file, std::size_t line) = 0;

/**
 * warn logging
 *
 * @param msg message to be logged
 * @param file file from which the message is coming
 * @param line line in the file of the message
 *
 */
  virtual void warn(const std::string& msg, const std::string& file, std::size_t line) = 0;

/**
 * error logging
 *
 * @param msg message to be logged
 * @param file file from which the message is coming
 * @param line line in the file of the message
 *
 */
  virtual void error(const std::string& msg, const std::string& file, std::size_t line) = 0;
};

/**
 * default logger class provided by the library
 *
 */
class logger : public logger_iface {
public:
/**
 * log level
 *
 */
  enum class log_level {
    error = 0,
    warn  = 1,
    info  = 2,
    debug = 3
  };

public:
/**
 * ctor
 *
 */
    explicit logger(log_level level = log_level::info);
/**
 * dtor
 *
 */
  ~logger() override = default;

/**
 * copy ctor
 *
 */
  logger(const logger&) = default;
/**
 * assignment operator
 *
 */
  logger& operator=(const logger&) = default;

public:
/**
 * debug logging
 *
 * @param msg message to be logged
 * @param file file from which the message is coming
 * @param line line in the file of the message
 *
 */
  void debug(const std::string& msg, const std::string& file, std::size_t line) override;

/**
 * info logging
 *
 * @param msg message to be logged
 * @param file file from which the message is coming
 * @param line line in the file of the message
 *
 */
  void info(const std::string& msg, const std::string& file, std::size_t line) override;

/**
 * warn logging
 *
 * @param msg message to be logged
 * @param file file from which the message is coming
 * @param line line in the file of the message
 *
 */
  void warn(const std::string& msg, const std::string& file, std::size_t line) override;

/**
 * error logging
 *
 * @param msg message to be logged
 * @param file file from which the message is coming
 * @param line line in the file of the message
 *
 */
  void error(const std::string& msg, const std::string& file, std::size_t line) override;

private:
/**
 * current log level in use
 *
 */
  log_level m_level;

/**
 * mutex used to serialize logs in multi-threaded environment
 *
 */
  std::mutex m_mutex;
};

/**
 * variable containing the current logger
 * by default, not set (no logs)
 *
 */
extern std::unique_ptr<logger_iface> active_logger;

/**
 * debug logging
 * convenience function used internally to call the logger
 *
 * @param msg message to be logged
 * @param file file from which the message is coming
 * @param line line in the file of the message
 *
 */
void debug(const std::string& msg, const std::string& file, std::size_t line);

/**
 * info logging
 * convenience function used internally to call the logger
 *
 * @param msg message to be logged
 * @param file file from which the message is coming
 * @param line line in the file of the message
 *
 */
void info(const std::string& msg, const std::string& file, std::size_t line);

/**
 * warn logging
 * convenience function used internally to call the logger
 *
 * @param msg message to be logged
 * @param file file from which the message is coming
 * @param line line in the file of the message
 *
 */
void warn(const std::string& msg, const std::string& file, std::size_t line);

/**
 * error logging
 * convenience function used internally to call the logger
 *
 * @param msg message to be logged
 * @param file file from which the message is coming
 * @param line line in the file of the message
 *
 */
void error(const std::string& msg, const std::string& file, std::size_t line);

/**
 * convenience macro to log with file and line information
 *
 */
#ifdef __CPP_REDIS_LOGGING_ENABLED
#define __CPP_REDIS_LOG(level, msg) cpp_redis::level(msg, __FILE__, __LINE__);
#else
#define __CPP_REDIS_LOG(level, msg)
#endif /* __CPP_REDIS_LOGGING_ENABLED */

} // namespace cpp_redis
