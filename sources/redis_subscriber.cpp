#include <cpp_redis/redis_subscriber.hpp>
#include <cpp_redis/redis_error.hpp>
#include <cpp_redis/logger.hpp>

namespace cpp_redis {

redis_subscriber::redis_subscriber(const std::shared_ptr<network::io_service> pIO) :
  m_client(pIO) {
  __CPP_REDIS_LOG(debug, "cpp_redis::redis_subscriber created");
}

redis_subscriber::~redis_subscriber(void) {
  __CPP_REDIS_LOG(debug, "cpp_redis::redis_subscriber destroyed");
}

void
redis_subscriber::connect(const std::string& host, unsigned int port,
                          const disconnection_handler_t& client_disconnection_handler)
{
  __CPP_REDIS_LOG(debug, "cpp_redis::redis_subscriber attempts to connect");

  auto disconnection_handler = std::bind(&redis_subscriber::connection_disconnection_handler, this, std::placeholders::_1);
  auto receive_handler = std::bind(&redis_subscriber::connection_receive_handler, this, std::placeholders::_1, std::placeholders::_2);
  m_client.connect(host, port, disconnection_handler, receive_handler);

  __CPP_REDIS_LOG(info, "cpp_redis::redis_subscriber connected");

  m_disconnection_handler = client_disconnection_handler;
}

void
redis_subscriber::disconnect(void) {
  __CPP_REDIS_LOG(debug, "cpp_redis::redis_subscriber attempts to disconnect");
  m_client.disconnect();
  __CPP_REDIS_LOG(info, "cpp_redis::redis_subscriber disconnected");
}

bool
redis_subscriber::is_connected(void) {
  return m_client.is_connected();
}

redis_subscriber&
redis_subscriber::subscribe(const std::string& channel, const subscribe_callback_t& callback) {
  std::lock_guard<std::mutex> lock(m_subscribed_channels_mutex);

  __CPP_REDIS_LOG(debug, "cpp_redis::redis_subscriber attemps to subscribe to channel " + channel);
  m_subscribed_channels[channel] = callback;
  m_client.send({ "SUBSCRIBE", channel });
  __CPP_REDIS_LOG(info, "cpp_redis::redis_subscriber subscribed to channel " + channel);

  return *this;
}

redis_subscriber&
redis_subscriber::psubscribe(const std::string& pattern, const subscribe_callback_t& callback) {
  std::lock_guard<std::mutex> lock(m_psubscribed_channels_mutex);

  __CPP_REDIS_LOG(debug, "cpp_redis::redis_subscriber attemps to psubscribe to channel " + pattern);
  m_psubscribed_channels[pattern] = callback;
  m_client.send({ "PSUBSCRIBE", pattern });
  __CPP_REDIS_LOG(info, "cpp_redis::redis_subscriber psubscribed to channel " + pattern);

  return *this;
}

redis_subscriber&
redis_subscriber::unsubscribe(const std::string& channel) {
  std::lock_guard<std::mutex> lock(m_subscribed_channels_mutex);

  __CPP_REDIS_LOG(debug, "cpp_redis::redis_subscriber attemps to unsubscribe from channel " + channel);
  auto it = m_subscribed_channels.find(channel);
  if (it == m_subscribed_channels.end()) {
    __CPP_REDIS_LOG(debug, "cpp_redis::redis_subscriber was not subscribed to channel " + channel);
    return *this;
  }

  m_client.send({ "UNSUBSCRIBE", channel });
  m_subscribed_channels.erase(it);
  __CPP_REDIS_LOG(info, "cpp_redis::redis_subscriber unsubscribed from channel " + channel);

  return *this;
}

redis_subscriber&
redis_subscriber::punsubscribe(const std::string& pattern) {
  std::lock_guard<std::mutex> lock(m_psubscribed_channels_mutex);

  __CPP_REDIS_LOG(debug, "cpp_redis::redis_subscriber attemps to punsubscribe from channel " + pattern);
  auto it = m_psubscribed_channels.find(pattern);
  if (it == m_psubscribed_channels.end()) {
    __CPP_REDIS_LOG(debug, "cpp_redis::redis_subscriber was not psubscribed to channel " + pattern);
    return *this;
  }

  m_client.send({ "PUNSUBSCRIBE", pattern });
  m_psubscribed_channels.erase(it);
  __CPP_REDIS_LOG(info, "cpp_redis::redis_subscriber punsubscribed from channel " + pattern);

  return *this;
}

redis_subscriber&
redis_subscriber::commit(void) {
  try {
    __CPP_REDIS_LOG(debug, "cpp_redis::redis_subscriber attempts to send pipelined commands");
    m_client.commit();
    __CPP_REDIS_LOG(info, "cpp_redis::redis_subscriber sent pipelined commands");
  }
  catch (const cpp_redis::redis_error& e) {
    __CPP_REDIS_LOG(error, "cpp_redis::redis_subscriber could not send pipelined commands");
    throw e;
  }

  return *this;
}

void
redis_subscriber::handle_subscribe_reply(const std::vector<reply>& reply) {
  if (reply.size() != 3)
      return ;

  const auto& title = reply[0];
  const auto& channel = reply[1];
  const auto& message = reply[2];

  if (!title.is_string()
   || !channel.is_string()
   || !message.is_string())
    return ;

  if (title.as_string() != "message")
    return ;

  std::lock_guard<std::mutex> lock(m_subscribed_channels_mutex);

  auto it = m_subscribed_channels.find(channel.as_string());
  if (it == m_subscribed_channels.end())
    return ;

  __CPP_REDIS_LOG(debug, "cpp_redis::redis_subscriber executes subscribe callback for channel " + channel.as_string());
  it->second(channel.as_string(), message.as_string());
}

void
redis_subscriber::handle_psubscribe_reply(const std::vector<reply>& reply) {
  if (reply.size() != 4)
      return ;

  const auto& title = reply[0];
  const auto& pchannel = reply[1];
  const auto& channel = reply[2];
  const auto& message = reply[3];

  if (!title.is_string()
      || !pchannel.is_string()
      || !channel.is_string()
      || !message.is_string())
    return ;

  if (title.as_string() != "pmessage")
    return ;

  std::lock_guard<std::mutex> lock(m_psubscribed_channels_mutex);

  auto it = m_psubscribed_channels.find(pchannel.as_string());
  if (it == m_psubscribed_channels.end())
    return ;

  __CPP_REDIS_LOG(debug, "cpp_redis::redis_subscriber executes psubscribe callback for channel " + channel.as_string());
  it->second(channel.as_string(), message.as_string());
}

void
redis_subscriber::connection_receive_handler(network::redis_connection&, reply& reply) {
  __CPP_REDIS_LOG(info, "cpp_redis::redis_subscriber received reply");

  //! alaway return an array
  if (!reply.is_array())
    return ;

  auto& array = reply.as_array();

  //! Array size of 3 -> SUBSCRIBE
  //! Array size of 4 -> PSUBSCRIBE
  //! Otherwise -> unexepcted reply
  if (array.size() == 3)
    handle_subscribe_reply(array);
  else if (array.size() == 4)
    handle_psubscribe_reply(array);
}

void
redis_subscriber::connection_disconnection_handler(network::redis_connection&) {
  __CPP_REDIS_LOG(warn, "cpp_redis::redis_subscriber has been disconnected");

  if (m_disconnection_handler) {
    __CPP_REDIS_LOG(info, "cpp_redis::redis_subscriber calls disconnection handler");
    m_disconnection_handler(*this);
  }
}

} //! cpp_redis
