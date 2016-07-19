#include "cpp_redis/network/io_service.hpp"
#include "cpp_redis/redis_error.hpp"

#include <fcntl.h>

namespace cpp_redis {

namespace network {

io_service&
io_service::get_instance(void) {
  static io_service instance;
  return instance;
}

io_service::io_service(void) {}

io_service::~io_service(void) {}

int
io_service::init_sets(fd_set* rd_set, fd_set* wr_set) {}

void
io_service::read_fd(int fd) {}

void
io_service::write_fd(int fd) {}

void
io_service::process_sets(fd_set* rd_set, fd_set* wr_set) {}

void
io_service::listen(void) {}

void
io_service::track(int fd, const disconnection_handler_t& handler) {}

void
io_service::untrack(int fd) {}

void
io_service::untrack_no_lock(int fd) {}

void
io_service::untrack_and_notify(int fd) {}

bool
io_service::async_read(int fd, std::vector<char>& buffer, std::size_t read_size, const read_callback_t& callback) {}

bool
io_service::async_write(int fd, const std::vector<char>& buffer, std::size_t write_size, const write_callback_t& callback) {}

void
io_service::notify_select(void) {}

} //! network

} //! cpp_redis
