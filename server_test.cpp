#include "./protocol/PortHandler.hpp"
#include "./protocol/UWUP.hpp"

#include <arpa/inet.h>
#include <chrono>
#include <iostream>
#include <netdb.h>
#include <sys/types.h>
#include <thread>

int main() {
    // Init socket
    int sockfd;
    int my_port = 8080;
    UWUPSocket socket;
    socket.listen(my_port);
    // return 1;
    int i = 0;
    while (1) {
        UWUPSocket newSock = socket.accept();
        std::cout << newSock << std::endl;
        std::cout << "Number of clients " << i << std::endl;
    }

    // start recv thread
    // portHandler.recvThreadFunction();
    // std::thread sr_thread(&PortHandler::recvThreadFunction, &portHandler);
    // sr_thread.join();
    // recv
    std::this_thread::sleep_for(std::chrono::seconds(60));
    return 0;
}