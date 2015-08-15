#include "cpp_redis/redis_client.hpp"
#include "cpp_redis/redis_error.hpp"
#include "cpp_redis/redis_subscriber.hpp"

#include <signal.h>
#include <iostream>

bool should_exit = false;
cpp_redis::redis_client client;
cpp_redis::redis_subscriber sub;

void sigint_handler(int) {
    std::cout << "disconnected (sigint handler)" << std::endl;
    client.disconnect();
    sub.disconnect();
}

int main(void) {
    client.set_disconnection_handler([] (cpp_redis::redis_client&) {
        std::cout << "client disconnected (disconnection handler)" << std::endl;
        should_exit = true;
    });
    sub.set_disconnection_handler([] (cpp_redis::redis_subscriber&) {
        std::cout << "sub disconnected (disconnection handler)" << std::endl;
        should_exit = true;
    });

    try {
        client.connect();
        sub.connect();
    }
    catch (const cpp_redis::redis_error& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    std::cout << "Connected" << std::endl;

    client.send({"SET", "hello", "world"});
    client.send({"GET", "hello"});
    client.send({"GET", "helloz"});
    client.send({"LLEN", "test"});
    client.send({"dfkl"});

    sub.subscribe("some_chan", [] (const std::string& chan, const std::string& msg) {
        std::cout << "MESSAGE " << chan << ": " << msg << std::endl;
    });
    sub.psubscribe("*", [] (const std::string& chan, const std::string& msg) {
        std::cout << "PMESSAGE " << chan << ": " << msg << std::endl;
    });

    signal(SIGINT, &sigint_handler);
    while (not should_exit);

    return 0;
}
