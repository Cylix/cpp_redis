#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#define __CPP_REDIS_DEFAULT_NB_THREADS 16

namespace cpp_redis {

namespace tools {

class thread_pool {
public:
  //! singleton pattern: instance getter
  //! first parameter defines the number of threads to use
  //! the parameter is only processed for the first call, and is ignored on future calls
  static const std::shared_ptr<thread_pool>& get_instance(size_t nb_threads = __CPP_REDIS_DEFAULT_NB_THREADS);

  //! dtor
  ~thread_pool(void);

private:
  //! ctor
  thread_pool(size_t nb_threads);

  //! copy ctor & assignment operator
  thread_pool(const thread_pool&) = delete;
  thread_pool& operator=(const thread_pool&) = delete;

public:
  //! whether the thread_pool is running or not
  bool is_running(void) const;

  //! return number of threads
  size_t get_nb_threads(void) const;

  //! add tasks
  typedef std::function<void()> task_t;
  void add_task(const task_t& task);
  void operator<<(const task_t& task);

private:
  //! workers processing loop
  void run(void);

  //! retrieve available task
  task_t fetch_task(void);

private:
  //! whether the thread_pool is running or not
  std::atomic_bool m_is_running;

  //! workers
  std::vector<std::thread> m_workers;

  //! taks
  std::queue<task_t> m_tasks;

  //! thread safety
  std::mutex m_tasks_mutex;
  std::condition_variable m_tasks_condvar;
};

} //! tools

} //! cpp_redis
