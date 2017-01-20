#include <cpp_redis/cpp_redis>

#include <iostream>

int
main(void) {
  //! Enable logging
  cpp_redis::active_logger = std::unique_ptr<cpp_redis::logger>(new cpp_redis::logger);

  cpp_redis::future_client client;

  client.connect("127.0.0.1", 6379, [](cpp_redis::redis_client&) {
    std::cout << "client disconnected (disconnection handler)" << std::endl;
  });

  //! Set a value
  auto tok = client.set("hello", "42");
  std::cout << "set 'hello' 42: " << tok.get() << std::endl;

  //! Decrement the value
  cpp_redis::reply r = client.decrby("hello", 12).get();
  if (r.is_integer())
    std::cout << "After 'hello' decrement by 12: " << r.as_integer() << std::endl;

  //! Read the value again
  tok = client.get("hello");
  std::cout << "get 'hello': " << tok.get() << std::endl;

  return 0;
}
