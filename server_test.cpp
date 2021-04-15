#include "./protocol/PortHandler.hpp"
#include "./protocol/UWUP.hpp"

#include <arpa/inet.h>
#include <chrono>
#include <cstring>
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
    int i = 50;
    UWUPSocket newSock = socket.accept();
    std::cout << newSock << std::endl;
    while (i--) {

        // std::string s = std::to_string(i);
        // char *msg = (char *)s.c_str();
        // // sprintf(msg1, "Initial SYN packet, client %d", i);
        // Packet packet(i, i, i, i, msg, s.length());
        // std::cout << packet << std::endl;

        // std::this_thread::sleep_for(std::chrono::seconds(5));
        // char msg[] = "Test Message";
        std::string message = "Test Message";
        message += std::to_string(i);
        char msg[message.length()];
        strcpy(msg, message.c_str());
        Packet ackPacket(i, i, 0, 0, msg, sizeof(msg));

        // newSock.port_handler->sendPacketTo(ackPacket, newSock.peer_address,
        //                                    newSock.peer_port);
        newSock.send(msg, sizeof(msg));
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::cout << "Send one packet" << std::endl;
    }
    // start recv thread
    // portHandler.recvThreadFunction();
    // std::thread sr_thread(&PortHandler::recvThreadFunction, &portHandler);
    // sr_thread.join();
    // recv
    std::this_thread::sleep_for(std::chrono::seconds(60));
    return 0;
}