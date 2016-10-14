#pragma once

#include <functional>
#include <memory>
#include <vector>

#define __CPP_REDIS_DEFAULT_NB_IO_SERVICE_WORKERS 1

namespace cpp_redis {

namespace network {

class io_service {
public:
  //! get default global instance
  static const std::shared_ptr<network::io_service>& get_global_instance(void);
  //! set default global instance
  static void set_global_instance(const std::shared_ptr<network::io_service>& instance);

public:
  //! ctor & dtor
  io_service(void) = default;
  virtual ~io_service(void) = default;

  //! copy ctor & assignment operator
  io_service(const io_service&) = default;
  io_service& operator=(const io_service&) = default;

public:
  //! disconnection handler declaration
  typedef std::function<void(io_service&)> disconnection_handler_t;

  //! add or remove a given fd from the io service
  //! untrack should never be called from inside a callback
  virtual void track(int fd, const disconnection_handler_t& handler) = 0;
  virtual void untrack(int fd) = 0;

  //! asynchronously read read_size bytes and append them to the given buffer
  //! on completion, call the read_callback to notify of the success or failure of the operation
  //! return false if another async_read operation is in progress or fd is not registered
  typedef std::function<void(std::size_t)> read_callback_t;
  virtual bool async_read(int fd, std::vector<char>& buffer, std::size_t read_size, const read_callback_t& callback) = 0;

  //! asynchronously write write_size bytes from buffer to the specified fd
  //! on completion, call the write_callback to notify of the success or failure of the operation
  //! return false if another async_write operation is in progress or fd is not registered
  typedef std::function<void(std::size_t)> write_callback_t;
  virtual bool async_write(int fd, const std::vector<char>& buffer, std::size_t write_size, const write_callback_t& callback) = 0;

private:
  //! listen for incoming events and notify
  virtual void process_io(void) = 0;
};

//! multi-platform instance builder
std::shared_ptr<network::io_service> create_io_service(void);

} //! network

} //! cpp_redis
