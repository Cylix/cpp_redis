#include "cpp_redis/builders/array_builder.hpp"

namespace cpp_redis {

namespace builders {

builder_iface&
array_builder::operator<<(std::string&) {
    return *this;
}

bool
array_builder::reply_ready(void) const {
}

const std::shared_ptr<reply>&
array_builder::get_reply(void) const {
}

} //! builders

} //! cpp_redis
