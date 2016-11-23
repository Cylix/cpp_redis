#include <cpp_redis/network/io_service.hpp>

#ifdef _WIN32
#include <cpp_redis/network/windows_impl/io_service.hpp>
#else
#include <cpp_redis/network/unix_impl/io_service.hpp>
#endif /* _WIN32 */

namespace cpp_redis {

namespace network {

static std::shared_ptr<network::io_service> global_instance = nullptr;

const std::shared_ptr<network::io_service>&
io_service::get_global_instance(void) {
  if (!global_instance)
    global_instance = create_io_service();

  return global_instance;
}

void
io_service::set_global_instance(const std::shared_ptr<network::io_service>& io_service) {
  global_instance = io_service;
}

io_service::io_service(std::size_t nb_workers)
: m_nb_workers(nb_workers) {}

std::size_t
io_service::get_nb_workers(void) const {
  return m_nb_workers;
}

std::shared_ptr<network::io_service>
create_io_service(std::size_t nb_workers) {
#ifdef _WIN32
  return std::make_shared<windows_impl::io_service>(nb_workers);
#else
  return std::make_shared<unix_impl::io_service>(nb_workers);
#endif /* _WIN32 */
}

} //! network

} //! cpp_redis
