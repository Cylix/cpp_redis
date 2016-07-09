#include <cpp_redis/cpp_redis>

#include <signal.h>
#include <iostream>

cpp_redis::redis_client client;

int main(int, char**) {
  client.connect();

  int size = 50;

  for(auto i=1;i<=size;i++) {
    std::cout << "Deleting hash cta:"+std::to_string(i) << std::endl;
    client.send({"HDEL ", "cta:"+std::to_string(i)});
  }

  std::cout << "creating values default" << std::endl;

  std::vector<std::string> arg;
  arg.push_back("HMSET");
  arg.push_back("cta:");
  for(auto i=1;i<=size;i++)
    arg.push_back("v" + std::to_string(i)+ " -1");

  for(auto i=1;i<=size;i++) {
    arg[1] = "cta:"+std::to_string(i);
    std::cout << arg[1] << std::endl;
    client.send(arg, [](cpp_redis::reply& reply){
      std::cout << reply.as_string() << std::endl;
    });
  }

  client.disconnect();

  return 0;
}
