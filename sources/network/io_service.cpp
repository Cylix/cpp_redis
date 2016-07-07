#include "cpp_redis/network/io_service.hpp"
#include "cpp_redis/redis_error.hpp"

namespace cpp_redis {

namespace network {

io_service&
io_service::get_instance(void) {
  static io_service instance;

  return instance;
}

io_service::io_service(void)
: m_worker(&io_service::listen, this)
, m_should_stop(false)
{
  if (pipe(m_notif_pipe_fds) == -1)
    throw cpp_redis::redis_error("Could not cpp_redis::io_service, pipe() failure");
}

io_service::~io_service(void) {
  m_should_stop = true;
  m_worker.join();
}

int
io_service::init_sets(fd_set* rd_set, fd_set* wr_set) {
  int max_fd = m_notif_pipe_fds[0];

  FD_ZERO(rd_set);
  FD_ZERO(wr_set);

  //! set fd for tracked fds
  for (const auto& fd : m_fds) {
    if (fd.second.async_read)
      FD_SET(fd.first, rd_set);

    if (fd.second.async_write)
      FD_SET(fd.first, wr_set);

    if ((fd.second.async_read or fd.second.async_write) and fd.first > max_fd)
      max_fd = fd.first;
  }

  //! set notif pipe fd
  FD_SET(m_notif_pipe_fds[0], rd_set);

  return max_fd;
}

void
io_service::read_fd(std::pair<const int, fd_info>& fd) {
  auto& buffer = *fd.second.read_buffer;
  int original_buffer_size = buffer.size();
  buffer.resize(original_buffer_size + fd.second.read_size);

  int nb_bytes_read = recv(fd.first, buffer.data() + original_buffer_size, fd.second.read_size, 0);
  if (nb_bytes_read == -1) {
    buffer.resize(original_buffer_size);
    fd.second.read_callback(false, 0);
  }
  else {
    buffer.resize(original_buffer_size + nb_bytes_read);
    fd.second.read_callback(true, nb_bytes_read);
  }

  fd.second.async_read = false;
}

void
io_service::write_fd(std::pair<const int, fd_info>& fd) {
  const auto& buffer = *fd.second.write_buffer;
  int nb_bytes_written = send(fd.first, buffer.data(), fd.second.write_size, 0);

  if (nb_bytes_written == -1)
    fd.second.write_callback(false, 0);
  else
    fd.second.write_callback(true, nb_bytes_written);

  fd.second.async_write = false;
}

void
io_service::process_sets(fd_set* rd_set, fd_set* wr_set) {
  for (auto& fd : m_fds) {
    if (fd.second.async_read and FD_ISSET(fd.first, rd_set))
      read_fd(fd);

    if (fd.second.async_write and FD_ISSET(fd.first, wr_set))
      write_fd(fd);
  }

  if (FD_ISSET(m_notif_pipe_fds[0], rd_set)) {
    char buf[1024];
    read(m_notif_pipe_fds[0], buf, 1024);
  }
}

void
io_service::listen(void) {
  fd_set rd_set;
  fd_set wr_set;

  while (not m_should_stop) {
    int max_fd = init_sets(&rd_set, &wr_set);

    //! call select to watch the given fds
    if (select(max_fd + 1, &rd_set, &wr_set, nullptr, nullptr) == -1)
      throw cpp_redis::redis_error("cpp_redis::io_service error, select() failure");

    process_sets(&rd_set, &wr_set);
  }
}

void
io_service::track(int fd) {
  auto& info = m_fds[fd];

  info.async_read = false;
  info.async_write = false;

  notify_select();
}

void
io_service::untrack(int fd) {
  m_fds.erase(fd);
}

bool
io_service::async_read(int fd, std::vector<char>& buffer, std::size_t read_size, const read_callback_t& callback) {
  auto reg_fd_it = m_fds.find(fd);
  if (reg_fd_it == m_fds.end())
    return false;

  auto& reg_fd = reg_fd_it->second;
  bool expected = false;
  if (not reg_fd.async_read.compare_exchange_strong(expected, true))
    return false;

  reg_fd.read_buffer = &buffer;
  reg_fd.read_size = read_size;
  reg_fd.read_callback = callback;

  notify_select();

  return true;
}

bool
io_service::async_write(int fd, const std::vector<char>& buffer, std::size_t write_size, const write_callback_t& callback) {
  auto reg_fd_it = m_fds.find(fd);
  if (reg_fd_it == m_fds.end())
    return false;

  auto& reg_fd = reg_fd_it->second;
  bool expected = false;
  if (not reg_fd.async_write.compare_exchange_strong(expected, true))
    return false;

  reg_fd.write_buffer = &buffer;
  reg_fd.write_size = write_size;
  reg_fd.write_callback = callback;

  notify_select();

  return true;
}

void
io_service::notify_select(void) {
  write(m_notif_pipe_fds[1], "a", 1);
}

} //! network

} //! cpp_redis
