#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cpp_redis/network/io_service.hpp>

#ifndef _CPP_REDIS_MAX_NB_FDS
#define _CPP_REDIS_MAX_NB_FDS 1024
#endif /* _CPP_REDIS_MAX_NB_FDS */

namespace cpp_redis {

namespace network {

namespace unix {

class io_service : public network::io_service {
public:
  //! ctor & dtor
  io_service(std::size_t nb_workers);
  ~io_service(void);

  //! copy ctor & assignment operator
  io_service(const io_service&) = delete;
  io_service& operator=(const io_service&) = delete;

public:
  void track(_sock_t fd, const disconnection_handler_t& handler) override;
  void untrack(_sock_t fd) override;

  bool async_read(_sock_t fd, std::vector<char>& buffer, std::size_t read_size, const read_callback_t& callback) override;
  bool async_write(_sock_t fd, const std::vector<char>& buffer, std::size_t write_size, const write_callback_t& callback) override;

private:
  //! simple struct to keep track of ongoing operations on a given fd
  struct fd_info {
    disconnection_handler_t disconnection_handler;

    std::atomic_bool async_read;
    std::vector<char>* read_buffer;
    std::size_t read_size;
    read_callback_t read_callback;

    std::atomic_bool async_write;
    std::vector<char> write_buffer;
    std::size_t write_size;
    write_callback_t write_callback;

    std::atomic_bool callback_running;
    std::condition_variable_any callback_notification;
  };

private:
  //! listen for incoming events and notify
  void process_io(void) override;

  //! notify the poll call so that it can wake up to process new events
  void notify_poll(void);

private:
  //! poll fds sets handling (init, rd/wr handling)
  std::size_t init_sets(struct pollfd* fds);
  void process_sets(struct pollfd* fds, std::size_t nfds);

  void read_fd(int fd);
  void write_fd(int fd);

private:
  //! whether the worker should terminate or not
  std::atomic_bool m_should_stop;

  //! worker in the background, listening for events
  std::thread m_worker;

  //! tracked fds
  std::unordered_map<int, fd_info> m_fds;

  //! fd associated to the pipe used to wake up the poll call
  int m_notif_pipe_fds[2];

  //! mutex to protect m_fds access against race condition
  //!
  //! specific mutex for untrack: we dont want someone to untrack a fd while we process it
  //! this behavior could cause some issues when executing callbacks in another thread
  //! for example, obj is destroyed, in its dtor it untracks the fd, but at the same time
  //! a callback is executed from within another thread: the untrack mutex avoid this without being costly
  std::recursive_mutex m_fds_mutex;
};

} //! unix

} //! network

} //! cpp_redis
