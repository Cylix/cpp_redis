#pragma once

#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <WinSock2.h>

#include <cpp_redis/network/io_service.hpp>

namespace cpp_redis {

namespace network {

namespace windows {

typedef enum _enIoOperation {
  //IO_OP_ACCEPT,
  IO_OP_READ,
  IO_OP_WRITE
} enIoOperation;

class io_service : public network::io_service {
public:
  //! ctor & dtor
  io_service(std::size_t nb_workers);
  ~io_service(void);

private:
  //! copy ctor & assignment operator
  io_service(const io_service&) = delete;
  io_service& operator=(const io_service&) = delete;

public:
  void track(_sock_t sock, const disconnection_handler_t& handler) override;
  void untrack(_sock_t sock) override;

  bool async_read(_sock_t socket, std::vector<char>& buffer, std::size_t read_size, const read_callback_t& callback) override;
  bool async_write(_sock_t socket, const std::vector<char>& buffer, std::size_t write_size, const write_callback_t& callback) override;

private:
  //! wait for incoming events and notify
  void process_io(void) override;

  //! shutdown
  void shutdown(void);

private:
  struct io_context_info : OVERLAPPED {
    WSAOVERLAPPED overlapped;
    enIoOperation eOperation;
  };

private:
  //! simple struct to keep track of ongoing operations on a given sockeet
  class sock_info {
  public:
    sock_info(void) = default;
    virtual ~sock_info(void) {
      std::lock_guard<std::recursive_mutex> socklock(sock_info_mutex);
      for (auto it = io_contexts_pool.begin(); it != io_contexts_pool.end(); it++)
        delete *it;

      io_contexts_pool.clear();
    }

    SOCKET hsock;
    std::size_t sent_bytes;

    //! Must protect the members of our structure from access by multiple threads during IO Completion
    std::recursive_mutex sock_info_mutex;

    //! We keep a simple vector of io_context_info structs to reuse for overlapped WSARecv and WSASend operations
    //! Since each must have its OWN struct if we issue them at the same time.
    //! othewise things get tangled up and borked.
    std::vector<io_context_info*> io_contexts_pool;

    disconnection_handler_t disconnection_handler;

    std::vector<char>* read_buffer;
    read_callback_t read_callback;

    std::vector<char> write_buffer;
    std::size_t write_size;
    write_callback_t write_callback;

    io_context_info*
    get_pool_io_context() {
      io_context_info* pInfo = NULL;
      std::lock_guard<std::recursive_mutex> socklock(sock_info_mutex);
      if (!io_contexts_pool.empty()) {
        pInfo = io_contexts_pool.back();
        io_contexts_pool.pop_back();
      }
      if (!pInfo)
        pInfo = new io_context_info();
      //MUST clear the overlapped structure between IO calls!
      memset(&pInfo->overlapped, 0, sizeof(OVERLAPPED));
      return pInfo;
    }

    void
    return_pool_io_context(io_context_info* p_io) {
      std::lock_guard<std::recursive_mutex> socklock(sock_info_mutex);
      io_contexts_pool.push_back(p_io);
    }
  };

private:
  //! completion port
  HANDLE m_completion_port;

  //! vector containing all the threads we start to service our i/o requests
  std::vector<std::thread> m_worker_threads;

private:
  //! whether the worker should terminate or not
  std::atomic_bool m_should_stop;

  //! tracked sockets
  std::unordered_map<SOCKET, sock_info> m_sockets;

  //! mutex to protect m_notify_socket access against race condition
  //!
  //! specific mutex for untrack: we dont want someone to untrack a socket while we process it
  //! this behavior could cause some issues when executing callbacks in another thread
  //! for example, obj is destroyed, in its dtor it untracks the socket, but at the same time
  //! a callback is executed from within another thread: the untrack mutex avoid this without being costly
  std::recursive_mutex m_socket_mutex;
};

} //! windows

} //! network

} //! cpp_redis
