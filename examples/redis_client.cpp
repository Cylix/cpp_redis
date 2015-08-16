#include "cpp_redis/cpp_redis.hpp"

#include <signal.h>
#include <iostream>

bool should_exit = false;
cpp_redis::redis_client client;

void
sigint_handler(int) {
    std::cout << "disconnected (sigint handler)" << std::endl;
    client.disconnect();
}

int
main(void) {
    client.set_disconnection_handler([] (cpp_redis::redis_client&) {
        std::cout << "client disconnected (disconnection handler)" << std::endl;
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

    client.send({"SET", "hello", "world"});
    client.send({"GET", "hello"}, [] (const std::shared_ptr<cpp_redis::reply>& reply) {
        std::cout << std::dynamic_pointer_cast<cpp_redis::bulk_string_reply>(reply)->get_bulk_string() << std::endl;
    });

    signal(SIGINT, &sigint_handler);
    while (not should_exit);

    return 0;
}
