#include "cpp_redis/redis_client.hpp"
#include "cpp_redis/redis_error.hpp"

#include <signal.h>
#include <iostream>

bool should_exit = false;
cpp_redis::redis_client client;

void sigint_handler(int) {
    std::cout << "disconnected (sigint handler)" << std::endl;
    client.disconnect();
}

int main(void) {
    client.set_disconnection_handler([] (cpp_redis::redis_client&) {
        std::cout << "disconnected (disconnection handler)" << std::endl;
        should_exit = true;
    });

    try {
        client.connect();
    }
    catch (const cpp_redis::redis_error& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    std::cout << "Connected" << std::endl;

    client.send({"SET", "hello", "world"}, [](const std::shared_ptr<cpp_redis::reply>& reply){
        std::cout << (int)reply->get_type() << std::endl;
    });
    client.send({"GET", "hello"}, [](const std::shared_ptr<cpp_redis::reply>& reply){
        std::cout << (int)reply->get_type() << std::endl;
    });
    client.send({"GET", "helloz"}, [](const std::shared_ptr<cpp_redis::reply>& reply){
        std::cout << (int)reply->get_type() << std::endl;
    });
    client.send({"LLEN", "test"}, [](const std::shared_ptr<cpp_redis::reply>& reply){
        std::cout << (int)reply->get_type() << std::endl;
    });
    client.send({"dfkl"}, [](const std::shared_ptr<cpp_redis::reply>& reply){
        std::cout << (int)reply->get_type() << std::endl;
    });
    client.send({"SUBSCRIBE", "Hello"}, [](const std::shared_ptr<cpp_redis::reply>& reply){
        std::cout << (int)reply->get_type() << std::endl;
    });

    signal(SIGINT, &sigint_handler);
    while (not should_exit);

    return 0;
}
