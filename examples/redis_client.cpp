#include "cpp_redis/cpp_redis.hpp"

#include <signal.h>
#include <iostream>

volatile std::atomic_bool should_exit(false);
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

    client.connect();

    client.send({"SET", "hello", "world"});
    client.send({"GET", "hello"}, [] (cpp_redis::reply& reply) {
        std::cout << reply.as_string() << std::endl;
    });

    signal(SIGINT, &sigint_handler);
    while (not should_exit);

    return 0;
}
