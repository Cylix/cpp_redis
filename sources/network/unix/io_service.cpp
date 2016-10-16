#include <fcntl.h>

#include <cpp_redis/logger.hpp>
#include <cpp_redis/network/unix/io_service.hpp>
#include <cpp_redis/redis_error.hpp>

namespace cpp_redis {

namespace network {

namespace unix {

io_service::io_service(std::size_t nb_workers)
: network::io_service(nb_workers)
, m_should_stop(false)
, m_notif_pipe_fds{1, 1} {
  if (pipe(m_notif_pipe_fds) == -1) {
    __CPP_REDIS_LOG(error, "cpp_redis::network::io_service could not create pipe");
    throw cpp_redis::redis_error("Could not init cpp_redis::io_service, pipe() failure");
  }

  int flags = fcntl(m_notif_pipe_fds[1], F_GETFL, 0);
  if (flags == -1 || fcntl(m_notif_pipe_fds[1], F_SETFL, flags | O_NONBLOCK) == -1) {
    __CPP_REDIS_LOG(error, "cpp_redis::network::io_service could not configure pipe");
    throw cpp_redis::redis_error("Could not init cpp_redis::io_service, fcntl() failure");
  }

  m_worker = std::thread(&io_service::process_io, this);

  __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service created");
}

io_service::~io_service(void) {
  m_should_stop = true;
  notify_poll();

  __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service waiting for worker completion");
  m_worker.join();
  __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service finished to wait for worker completion");

  if (m_notif_pipe_fds[0] != -1)
    close(m_notif_pipe_fds[0]);
  if (m_notif_pipe_fds[1] != -1)
    close(m_notif_pipe_fds[1]);

  __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service destroyed");
}

std::size_t
io_service::init_sets(struct pollfd* fds) {
  fds[0].fd     = m_notif_pipe_fds[0];
  fds[0].events = POLLIN;

  std::lock_guard<std::recursive_mutex> lock(m_fds_mutex);
  std::size_t nfds = 1;
  for (const auto& fd : m_fds) {
    fds[nfds].fd     = fd.first;
    fds[nfds].events = 0;

    if (fd.second.async_read)
      fds[nfds].events |= POLLIN;

    if (fd.second.async_write)
      fds[nfds].events |= POLLOUT;

    ++nfds;
  }

  return nfds;
}

void
io_service::read_fd(int fd) {
  std::unique_lock<std::recursive_mutex> lock(m_fds_mutex);

  __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service non-blocking read available for fd #" + std::to_string(fd));

  auto fd_it = m_fds.find(fd);
  if (fd_it == m_fds.end()) {
    __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service does not track fd #" + std::to_string(fd));
    return;
  }

  auto& buffer             = *fd_it->second.read_buffer;
  int original_buffer_size = buffer.size();
  buffer.resize(original_buffer_size + fd_it->second.read_size);

  __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service reading data for fd #" + std::to_string(fd));
  int nb_bytes_read        = recv(fd_it->first, buffer.data() + original_buffer_size, fd_it->second.read_size, 0);
  fd_it->second.async_read = false;

  if (nb_bytes_read <= 0) {
    __CPP_REDIS_LOG(error, "cpp_redis::network::io_service read error for fd #" + std::to_string(fd));
    buffer.resize(original_buffer_size);
    fd_it->second.disconnection_handler(*this);
    m_fds.erase(fd_it);
  }
  else {
    buffer.resize(original_buffer_size + nb_bytes_read);

    __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service calling read callback for fd #" + std::to_string(fd));

    fd_it->second.callback_running = true;
    lock.unlock();
    fd_it->second.read_callback(nb_bytes_read);
    fd_it->second.callback_running = false;
    fd_it->second.callback_notification.notify_all();
  }
}

void
io_service::write_fd(int fd) {
  std::unique_lock<std::recursive_mutex> lock(m_fds_mutex);

  __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service non-blocking write available for fd #" + std::to_string(fd));

  auto fd_it = m_fds.find(fd);
  if (fd_it == m_fds.end()) {
    __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service does not track fd #" + std::to_string(fd));
    return;
  }

  __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service writing data for fd #" + std::to_string(fd));
  int nb_bytes_written      = send(fd_it->first, fd_it->second.write_buffer.data(), fd_it->second.write_size, 0);
  fd_it->second.async_write = false;

  if (nb_bytes_written <= 0) {
    __CPP_REDIS_LOG(error, "cpp_redis::network::io_service write error for fd #" + std::to_string(fd));
    fd_it->second.disconnection_handler(*this);
    m_fds.erase(fd_it);
  }
  else {
    __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service calling write callback for fd #" + std::to_string(fd));

    fd_it->second.callback_running = true;
    lock.unlock();
    fd_it->second.write_callback(nb_bytes_written);
    fd_it->second.callback_running = false;
    fd_it->second.callback_notification.notify_all();
  }
}

void
io_service::process_sets(struct pollfd* fds, std::size_t nfds) {
  std::vector<int> fds_to_read;
  std::vector<int> fds_to_write;

  //! Quickly fetch fds that are readable/writeable
  //! This reduce lock time and avoid possible deadlock in callbacks
  {
    std::lock_guard<std::recursive_mutex> lock(m_fds_mutex);

    for (std::size_t i = 0; i < nfds; ++i) {
      if (fds[i].revents & POLLIN && m_fds[fds[i].fd].async_read)
        fds_to_read.push_back(fds[i].fd);

      if (fds[i].revents & POLLOUT && m_fds[fds[i].fd].async_write)
        fds_to_write.push_back(fds[i].fd);
    }
  }

  for (int fd : fds_to_read) { read_fd(fd); }
  for (int fd : fds_to_write) { write_fd(fd); }

  if (fds[0].revents & POLLIN) {
    char buf[1024];
    (void) read(m_notif_pipe_fds[0], buf, 1024);
  }
}

void
io_service::process_io(void) {
  struct pollfd fds[_CPP_REDIS_MAX_NB_FDS];

  __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service starts poll loop in worker thread");

  while (!m_should_stop) {
    std::size_t nfds = init_sets(fds);

    if (poll(fds, nfds, -1) > 0) {
      __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service woke up by poll");
      process_sets(fds, nfds);
    }
    else {
      __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service woke up by poll, but nothing to process");
    }
  }

  __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service ends poll loop in worker thread");
}

void
io_service::track(_sock_t fd, const disconnection_handler_t& handler) {
  std::lock_guard<std::recursive_mutex> lock(m_fds_mutex);

  auto& info                 = m_fds[fd];
  info.async_read            = false;
  info.async_write           = false;
  info.disconnection_handler = handler;

  __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service now tracks fd #" + std::to_string(fd));

  notify_poll();
}

void
io_service::untrack(_sock_t fd) {
  std::unique_lock<std::recursive_mutex> lock(m_fds_mutex);

  __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service requests to untrack fd #" + std::to_string(fd));

  auto fd_it = m_fds.find(fd);
  if (fd_it == m_fds.end()) {
    __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service does not track fd #" + std::to_string(fd));
    return;
  }

  if (fd_it->second.callback_running) {
    __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service waits for callbacks to complete before untracking fd #" + std::to_string(fd));
    fd_it->second.callback_notification.wait(lock, [=] { return !fd_it->second.callback_running; });
  }

  m_fds.erase(fd_it);

  __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service now untracks fd #" + std::to_string(fd));
}

bool
io_service::async_read(_sock_t fd, std::vector<char>& buffer, std::size_t read_size, const read_callback_t& callback) {
  std::lock_guard<std::recursive_mutex> lock(m_fds_mutex);

  __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service is requested async_read for fd #" + std::to_string(fd));

  auto reg_fd_it = m_fds.find(fd);
  if (reg_fd_it == m_fds.end()) {
    __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service does not track fd #" + std::to_string(fd));
    return false;
  }

  auto& reg_fd  = reg_fd_it->second;
  bool expected = false;
  if (!reg_fd.async_read.compare_exchange_strong(expected, true)) {
    __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service already doing async_read for fd #" + std::to_string(fd));
    return false;
  }

  reg_fd.read_buffer   = &buffer;
  reg_fd.read_size     = read_size;
  reg_fd.read_callback = callback;

  notify_poll();

  return true;
}

bool
io_service::async_write(_sock_t fd, const std::vector<char>& buffer, std::size_t write_size, const write_callback_t& callback) {
  std::lock_guard<std::recursive_mutex> lock(m_fds_mutex);

  __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service is requested async_write for fd #" + std::to_string(fd));

  auto reg_fd_it = m_fds.find(fd);
  if (reg_fd_it == m_fds.end()) {
    __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service does not track fd #" + std::to_string(fd));
    return false;
  }

  auto& reg_fd  = reg_fd_it->second;
  bool expected = false;
  if (!reg_fd.async_write.compare_exchange_strong(expected, true)) {
    __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service already doing async_write for fd #" + std::to_string(fd));
    return false;
  }

  reg_fd.write_buffer   = buffer;
  reg_fd.write_size     = write_size;
  reg_fd.write_callback = callback;

  notify_poll();

  return true;
}

void
io_service::notify_poll(void) {
  __CPP_REDIS_LOG(debug, "cpp_redis::network::io_service notifies poll to wake up");
  (void) write(m_notif_pipe_fds[1], "a", 1);
}

} //! unix

} //! network

} //! cpp_redis
