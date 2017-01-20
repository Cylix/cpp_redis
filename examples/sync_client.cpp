#include <cpp_redis/cpp_redis>

#include <iostream>

int
main(void) {
  //! Enable logging
  cpp_redis::active_logger = std::unique_ptr<cpp_redis::logger>(new cpp_redis::logger);

  cpp_redis::sync_client client;

  client.connect("127.0.0.1", 6379, [](cpp_redis::redis_client&) {
    std::cout << "client disconnected (disconnection handler)" << std::endl;
  });

  cpp_redis::reply r = client.set("hello", "42");
  std::cout << "set 'hello' 42: " << r << std::endl;

  r = client.decrby("hello", 12);
  std::cout << "decrby 'hello' 12: " << r << std::endl;

  r = client.get("hello");
  std::cout << "get 'hello': " << r << std::endl;

  return 0;
}
