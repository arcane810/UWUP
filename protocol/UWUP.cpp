#include "UWUP.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <random>
#include <sys/types.h>
#include <unistd.h>

// UWUPSocket::UWUPSocket(int my_port) : my_port(my_port) {
//     is_listen = true;

//     if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
//         throw("Error creating socket");
//     }

//     std::cout << "Socket Created\n";

//     int optval = 1;
//     setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
//                sizeof(int));
//     sockaddr_in servaddr;

//     servaddr.sin_family = AF_INET;
//     servaddr.sin_addr.s_addr = INADDR_ANY;
//     servaddr.sin_port = htons(my_port);

//     if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) <
//         0) {
//         throw("Error binding socket");
//     }
//     this->port_handler = new PortHandler(sockfd);
// }

UWUPSocket::UWUPSocket(int my_port) : my_port(my_port) {

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        throw("Error creating socket");
    }

    std::cout << "Socket Created\n";

    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
               sizeof(int));

    this->port_handler = new PortHandler(sockfd);
}

UWUPSocket::UWUPSocket(int sockfd, std::string peer_address, int peer_port,
                       PortHandler *port_handler)
    : sockfd(sockfd), peer_address(peer_address), peer_port(peer_port),
      port_handler(port_handler) {}


/// init the sockaddr_in struct and send handshake.
void UWUPSocket::connect(std::string peer_address, int peer_port) {
    this->peer_address = peer_address;
    this->peer_port = peer_port;
}

void UWUPSocket::listen() {

    is_listen = true;
    struct sockaddr_in servaddr;

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(my_port);

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) <
        0) {
        throw("Error binding socket");
    }
}


UWUPSocket UWUPSocket::accept() {
    std::pair<std::pair<std::string, int>, Packet> new_connection =
        port_handler->getNewConnection();

    std::cout << new_connection.first.first << ' ' << new_connection.first.second << new_connection.second << std::endl;

    // UWUPSocket new_socket =
    //     UWUPSocket(sockfd, new_connection.first.first,
    //                new_connection.first.second, port_handler);
}
