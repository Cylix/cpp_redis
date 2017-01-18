#include <cpp_redis/sync_client.hpp>
#include <cpp_redis/redis_error.hpp>

namespace cpp_redis {

sync_client::sync_client(const std::shared_ptr<network::io_service>& io_service)
: m_client(io_service)
{
  __CPP_REDIS_LOG(debug, "cpp_redis::sync_client created");
}

sync_client::~sync_client(void) {
  __CPP_REDIS_LOG(debug, "cpp_redis::sync_client destroyed");
}


} //! cpp_redis

