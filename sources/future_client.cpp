#include <cpp_redis/future_client.hpp>
#include <cpp_redis/redis_error.hpp>

namespace cpp_redis {

future_client::future_client(const std::shared_ptr<network::io_service>& io_service)
: m_client(io_service)
{
  __CPP_REDIS_LOG(debug, "cpp_redis::future_client created");
}

future_client::~future_client(void) {
  __CPP_REDIS_LOG(debug, "cpp_redis::future_client destroyed");
}


} //! cpp_redis

