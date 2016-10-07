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

io_service::io_service(void)
: m_should_stop(false)
{
  if (pipe(m_notif_pipe_fds) == -1)
    throw cpp_redis::redis_error("Could not init cpp_redis::io_service, pipe() failure");

  int flags = fcntl(m_notif_pipe_fds[1], F_GETFL, 0);
  if (flags == -1 or fcntl(m_notif_pipe_fds[1], F_SETFL, flags | O_NONBLOCK) == -1)
    throw cpp_redis::redis_error("Could not init cpp_redis::io_service, fcntl() failure");

  m_worker = std::thread(&io_service::listen, this);
}

io_service::~io_service(void) {
  m_should_stop = true;
  notify_select();

  m_worker.join();

  close(m_notif_pipe_fds[0]);
  close(m_notif_pipe_fds[1]);
}

unsigned int
io_service::init_sets(struct pollfd* fds) {
  fds[0].fd = m_notif_pipe_fds[0];
  fds[0].events = POLLIN;

  std::lock_guard<std::recursive_mutex> lock(m_fds_mutex);
  unsigned int nfds = 1;
  for (const auto& fd : m_fds) {
    fds[nfds].fd = fd.first;
    fds[nfds].events = 0;

    if (fd.second.async_read)
      fds[nfds].events |= POLLIN;

    if (fd.second.async_write)
      fds[nfds].events |= POLLOUT;

    ++nfds;
  }

  return nfds;
}

io_service::callback_t
io_service::read_fd(int fd) {
  std::lock_guard<std::recursive_mutex> lock(m_fds_mutex);

  auto fd_it = m_fds.find(fd);
  if (fd_it == m_fds.end())
    return nullptr;

  auto& buffer = *fd_it->second.read_buffer;
  int original_buffer_size = buffer.size();
  buffer.resize(original_buffer_size + fd_it->second.read_size);

  int nb_bytes_read = recv(fd_it->first, buffer.data() + original_buffer_size, fd_it->second.read_size, 0);
  fd_it->second.async_read = false;

  if (nb_bytes_read <= 0) {
    buffer.resize(original_buffer_size);
    fd_it->second.disconnection_handler(*this);
    m_fds.erase(fd_it);

    return nullptr;
  }
  else {
    buffer.resize(original_buffer_size + nb_bytes_read);

    return std::bind(fd_it->second.read_callback, nb_bytes_read);
  }
}

io_service::callback_t
io_service::write_fd(int fd) {
  std::lock_guard<std::recursive_mutex> lock(m_fds_mutex);

  auto fd_it = m_fds.find(fd);
  if (fd_it == m_fds.end())
    return nullptr;

  int nb_bytes_written = send(fd_it->first, fd_it->second.write_buffer.data(), fd_it->second.write_size, 0);
  fd_it->second.async_write = false;

  if (nb_bytes_written <= 0) {
    fd_it->second.disconnection_handler(*this);
    m_fds.erase(fd_it);

    return nullptr;
  }
  else
    return std::bind(fd_it->second.write_callback, nb_bytes_written);
}

void
io_service::process_sets(struct pollfd* fds, unsigned int nfds) {
  std::vector<int> fds_to_read;
  std::vector<int> fds_to_write;

  //! Quickly fetch fds that are readable/writeable
  //! This reduce lock time and avoid possible deadlock in callbacks
  {
    std::lock_guard<std::recursive_mutex> lock(m_fds_mutex);

    for (unsigned int i = 0; i < nfds; ++i) {
      if (fds[i].revents & POLLIN && m_fds[fds[i].fd].async_read)
        fds_to_read.push_back(fds[i].fd);

      if (fds[i].revents & POLLOUT && m_fds[fds[i].fd].async_write)
        fds_to_write.push_back(fds[i].fd);
    }
  }

  for (int fd : fds_to_read) {
    auto callback = read_fd(fd);
    if (callback)
      callback();
  }
  for (int fd : fds_to_write) {
    auto callback = write_fd(fd);
    if (callback)
      callback();
  }

  if (fds[0].revents & POLLIN) {
    char buf[1024];
    (void)read(m_notif_pipe_fds[0], buf, 1024);
  }
}

void
io_service::listen(void) {
  struct pollfd fds[1024];

  while (not m_should_stop) {
    unsigned int nfds = init_sets(fds);

    if (poll(fds, nfds, -1) > 0)
      process_sets(fds, nfds);
  }
}

void
io_service::track(int fd, const disconnection_handler_t& handler) {
  std::lock_guard<std::recursive_mutex> lock(m_fds_mutex);

  auto& info = m_fds[fd];
  info.async_read = false;
  info.async_write = false;
  info.disconnection_handler = handler;

  notify_select();
}

void
io_service::untrack(int fd) {
  std::lock_guard<std::recursive_mutex> lock(m_fds_mutex);
  m_fds.erase(fd);
}

bool
io_service::async_read(int fd, std::vector<char>& buffer, std::size_t read_size, const read_callback_t& callback) {
  std::lock_guard<std::recursive_mutex> lock(m_fds_mutex);

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
  std::lock_guard<std::recursive_mutex> lock(m_fds_mutex);

  auto reg_fd_it = m_fds.find(fd);
  if (reg_fd_it == m_fds.end())
    return false;

  auto& reg_fd = reg_fd_it->second;
  bool expected = false;
  if (not reg_fd.async_write.compare_exchange_strong(expected, true))
      return false;

  reg_fd.write_buffer = buffer;
  reg_fd.write_size = write_size;
  reg_fd.write_callback = callback;

  notify_select();

  return true;
}

void
io_service::notify_select(void) {
  (void)write(m_notif_pipe_fds[1], "a", 1);
}

} //! network

} //! cpp_redis
