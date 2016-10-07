#pragma once

#include <string>
#include <memory>

namespace cpp_redis {

//! logger_iface
//! should be inherited by any class intended to be used for logging
class logger_iface {
public:
  //! ctor & dtor
  logger_iface(void) = default;
  virtual ~logger_iface(void) = default;

  //! copy ctor & assignment operator
  logger_iface(const logger_iface&) = default;
  logger_iface& operator=(const logger_iface&) = default;

public:
  virtual void debug(const std::string& msg, const std::string& file, unsigned int line) = 0;
  virtual void info(const std::string& msg, const std::string& file, unsigned int line) = 0;
  virtual void warn(const std::string& msg, const std::string& file, unsigned int line) = 0;
  virtual void error(const std::string& msg, const std::string& file, unsigned int line) = 0;
};

//! default logger class provided by the library
class logger : public logger_iface {
public:
  //! ctor & dtor
  logger(void) = default;
  ~logger(void) = default;

  //! copy ctor & assignment operator
  logger(const logger&) = default;
  logger& operator=(const logger&) = default;

public:
  void debug(const std::string& msg, const std::string& file, unsigned int line);
  void info(const std::string& msg, const std::string& file, unsigned int line);
  void warn(const std::string& msg, const std::string& file, unsigned int line);
  void error(const std::string& msg, const std::string& file, unsigned int line);
};

//! variable containing the current logger
//! by default, not set (no logs)
extern std::unique_ptr<logger_iface> active_logger;

//! convenience functions used internaly to call the logger
void debug(const std::string& msg, const std::string& file, unsigned int line);
void info(const std::string& msg, const std::string& file, unsigned int line);
void warn(const std::string& msg, const std::string& file, unsigned int line);
void error(const std::string& msg, const std::string& file, unsigned int line);

//! convenience macro to log with file and line information
#define _CPP_REDIS_LOG(level, msg) cpp_redis::level(msg, __FILE__, __LINE__);

} //! cpp_redis
