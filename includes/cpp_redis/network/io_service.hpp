#pragma once

#include <thread>
#include <atomic>
#include <unordered_map>
#include <vector>
#include <mutex>

#ifndef _MSC_VER
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#else
#include <WinSock2.h>

#define MAX_BUFF_SIZE      CPP_REDIS_READ_SIZE
#define MAX_WORKER_THREADS 32
#endif

namespace cpp_redis {

namespace network {

typedef enum _enIoOperation {
    //IO_OP_ACCEPT,
    IO_OP_READ,
    IO_OP_WRITE
  } enIoOperation;


class io_service {
public:
  //! instance getter (singleton pattern)
  static io_service& get_instance(void);
  io_service(size_t max_worker_threads = MAX_WORKER_THREADS);
  ~io_service(void);

  void shutdown();

private:
  //! copy ctor & assignment operator
  io_service(const io_service&) = delete;
  io_service& operator=(const io_service&) = delete;

public:
  //! disconnection handler declaration
  typedef std::function<void(io_service&)> disconnection_handler_t;

  //! add or remove a given socket from the io service
  //! untrack should never be called from inside a callback
  void track(SOCKET sock, const disconnection_handler_t& handler);
  void untrack(SOCKET sock);

  //! asynchronously read read_size bytes and append them to the given buffer
  //! on completion, call the read_callback to notify of the success or failure of the operation
  //! return false if another async_read operation is in progress or socket is not registered
  typedef std::function<void(std::size_t)> read_callback_t;
  bool async_read(SOCKET socket, std::vector<char>& buffer, std::size_t read_size, const read_callback_t& callback);

  //! asynchronously write write_size bytes from buffer to the specified fd
  //!on completion, call the write_callback to notify of the success or failure of the operation
  //! return false if another async_write operation is in progress or fd is not registered
  typedef std::function<void(std::size_t)> write_callback_t;
  bool async_write(SOCKET socket, const std::vector<char>& buffer, std::size_t write_size, const write_callback_t& callback);

private:

#ifdef _WIN32
  struct io_context_info {
    WSAOVERLAPPED      overlapped;
    enIoOperation      eOperation;
  };
#endif

//! simple struct to keep track of ongoing operations on a given sockeet
  struct sock_info {
#ifdef _WIN32
    SOCKET             hsock;
    std::size_t        sent_bytes;

    //Must protect the members of our structure from access by multiple threads during IO Completion 
    std::recursive_mutex sock_info_mutex;
#endif
    disconnection_handler_t disconnection_handler;

    std::atomic_bool   async_read;
    std::vector<char>* read_buffer;
    std::size_t        read_size;
    read_callback_t    read_callback;

    std::atomic_bool   async_write;
    std::vector<char>  write_buffer;
    std::size_t        write_size;
    write_callback_t   write_callback;

#ifdef _WIN32
    io_context_info    io_info;
#endif
  };

typedef std::function<void()> callback_t;

#ifndef _WIN32
private:
  //! wait for incoming events and notify
  void wait_for_events(void);

  //! notify the select call so that it can wake up to process new events
  void notify_select(void);

private:
  //! select fds sets handling (init, rd/wr handling)
  int init_sets(fd_set* rd_set, fd_set* wr_set);
  void process_sets(fd_set* rd_set, fd_set* wr_set);

  callback_t read_sock(SOCKET sock);
  callback_t write_sock(SOCKET sock);
#else
  //! wait for incoming events and notify
  int process_io(void);

  HANDLE                   m_completion_port;
  unsigned int             m_worker_thread_pool_size;
  std::vector<std::thread> m_worker_threads;  //vector containing all the threads we start to service our i/o requests
#endif

private:
  //! whether the worker should terminate or not
  std::atomic_bool m_should_stop;

  //! tracked sockets
  std::unordered_map<SOCKET, sock_info> m_sockets;

#ifndef _WIN32
  //! socket associated used to wake up the select call
  SOCKET m_notify_socket;
#endif

  //! mutex to protect m_notify_socket access against race condition
  //!
  //! specific mutex for untrack: we dont want someone to untrack a socket while we process it
  //! this behavior could cause some issues when executing callbacks in another thread
  //! for example, obj is destroyed, in its dtor it untracks the socket, but at the same time
  //! a callback is executed from within another thread: the untrack mutex avoid this without being costly
  std::recursive_mutex m_socket_mutex;
};

} //! network

} //! cpp_redis
