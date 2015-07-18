#include "cpp_redis/network/tcp_client.hpp"

#include <signal.h>
#include <iostream>

bool should_exit = false;
cpp_redis::network::tcp_client client;
cpp_redis::network::tcp_client client2;

void sigint_handler(int) {
    std::cout << "disconnected (sigint handler)" << std::endl;
    client.disconnect();
    should_exit = true;
}

int main(void) {
    client.set_receive_handler([] (cpp_redis::network::tcp_client&, const std::vector<char>& msg) {
        std::cout << "recv: " << std::string(msg.data()) << std::endl;
        client.send(msg);
    });

    client.set_disconnection_handler([] (cpp_redis::network::tcp_client&) {
        std::cout << "disconnected (disconnection handler)" << std::endl;
        should_exit = true;
    });

    if (not client.connect("0.0.0.0", 3000)) {
        std::cerr << "fail to connect" << std::endl;
        return -1;
    }

    std::cout << "Connected" << std::endl;

    signal(SIGINT, &sigint_handler);
    while (not should_exit);

    return 0;
}
