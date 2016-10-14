#include <cpp_redis/tools/thread_pool.hpp>

namespace cpp_redis {

namespace tools {

const std::shared_ptr<thread_pool>&
thread_pool::get_instance(size_t nb_threads) {
  static std::shared_ptr<thread_pool> instance = std::shared_ptr<thread_pool>{ new thread_pool(nb_threads) };

  return instance;
}

thread_pool::thread_pool(size_t nb_thread)
: m_is_running(true)
, m_workers(nb_thread) {
  for (auto& worker : m_workers)
    worker = std::thread(std::bind(&thread_pool::run, this));
}

thread_pool::~thread_pool(void) {
  m_is_running = false;

  for (auto& worker : m_workers)
    worker.join();
}

bool
thread_pool::is_running(void) const {
  return m_is_running;
}

size_t
thread_pool::get_nb_threads(void) const {
  return m_workers.size();
}

void
thread_pool::add_task(const task_t& task) {
  std::lock_guard<std::mutex> lock(m_tasks_mutex);

  m_tasks.push(task);
}

void
thread_pool::operator<<(const task_t& task) {
  add_task(task);
}

thread_pool::task_t
thread_pool::fetch_task(void) {
  std::unique_lock<std::mutex> lock(m_tasks_mutex);

  while (m_is_running && m_tasks.empty())
    m_tasks_condvar.wait(lock);

  if (m_tasks.size()) {
    task_t task = m_tasks.front();
    m_tasks.pop();
    return task;
  }

  return nullptr;
}

void
thread_pool::run(void) {
  while (m_is_running) {
    auto task = fetch_task();

    if (task)
      task();
  }
}

} //! tools

} //! cpp_redis
