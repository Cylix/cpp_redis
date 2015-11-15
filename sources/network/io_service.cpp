#include "cpp_redis/network/io_service.hpp"

namespace cpp_redis {

namespace network {

io_service::io_service(void)
: m_work(m_io_service) {}

io_service::~io_service(void) {
    if (m_io_service_thread.joinable()) {
        m_io_service.stop();
        m_io_service_thread.join();
    }
}

void
io_service::run(void) {
    if (not m_io_service_thread.joinable())
        m_io_service_thread = std::thread([this]() { m_io_service.run(); });
}

void
io_service::post(const std::function<void()>& fct) {
    m_io_service.post(fct);
}

boost::asio::io_service&
io_service::get(void) {
    return m_io_service;
}

} //! network

} //! cpp_redis
