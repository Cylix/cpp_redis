#pragma once

#include <string>
#include <stdexcept>
#include <memory>
#include <deque>

#include "cpp_redis/replies/reply.hpp"
#include "cpp_redis/builders/builder_iface.hpp"

namespace cpp_redis {

namespace builders {

class reply_builder {
public:
    //! ctor & dtor
    reply_builder(void);
    ~reply_builder(void) = default;

    //! copy ctor & assignment operator
    reply_builder(const reply_builder&) = delete;
    reply_builder& operator=(const reply_builder&) = delete;

public:
    //! add data to reply builder
    reply_builder& operator<<(const std::string& data);

    //! get reply
    void operator>>(std::shared_ptr<replies::reply>& reply);
    std::shared_ptr<replies::reply> get_reply(void);

    //! returns whether a reply is available
    bool reply_available(void);

private:
    //! build reply. Return whether the reply has been fully built or not
    bool build_reply(void);

private:
    std::string m_buffer;
    std::unique_ptr<builder_iface> m_builder;
    std::deque<std::shared_ptr<replies::reply>> m_available_replies;
};

} //! builders

} //! cpp_redis
