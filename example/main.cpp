#include "cpp_redis/redis_client.hpp"
#include "cpp_redis/redis_error.hpp"

#include <signal.h>
#include <iostream>

bool should_exit = false;
cpp_redis::redis_client client;

void sigint_handler(int) {
    std::cout << "disconnected (sigint handler)" << std::endl;
    client.disconnect();
    should_exit = true;
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

    client.send({"SET", "hello", "world"}, [](bool, const std::string&){});
    client.send({"GET", "hello"}, [](bool, const std::string&){});

    signal(SIGINT, &sigint_handler);
    while (not should_exit);

    return 0;
}
