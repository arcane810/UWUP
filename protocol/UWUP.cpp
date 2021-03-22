#include "UWUP.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <random>
#include <unistd.h>

UWUPSocket::UWUPSocket() : peer_address(peer_address), peer_port(peer_port) {
    is_listen = false;
    addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof hints);
    int rv;
    int optval;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(NULL, NULL, &hints, &servinfo)) != 0) {
        throw("getaddrinfo: " + std::string(gai_strerror(rv)));
    }
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
            -1) {
            continue;
        }
    }
    if (p == NULL) {
        throw("Error Creating Socket");
    }

    std::cout << "Socket Created\n";
    freeaddrinfo(servinfo);

    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
               sizeof(int));
}

UWUPSocket::UWUPSocket(int peer_port) : peer_port(peer_port) {
    is_listen = true;
    addrinfo hints, *servinfo, *p;
    struct sockaddr_in bind_addr, sendto_addr;
    memset(&hints, 0, sizeof hints);
    int rv;
    int optval;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, std::to_string(peer_port).c_str(), &hints,
                          &servinfo)) != 0) {
        throw("getaddrinfo: " + std::string(gai_strerror(rv)));
    }
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
            -1) {
            continue;
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            throw("Bind Error");
            continue;
        }
    }
    if (p == NULL) {
        throw("Error Creating or Binding Socket");
    }

    std::cout << "Socket Created\n";
    freeaddrinfo(servinfo);

    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
               sizeof(int));
}

int UWUPSocket::connect(std::string peer_address, int peer_port) {
    this->peer_address = peer_address;
    this->peer_port = peer_port;
    std::mt19937 rng(
        std::chrono::steady_clock::now().time_since_epoch().count());
    seq_number = rng() % 100000000;
    // Initiate Handshake
    // TODO
}

UWUPSocket UWUPSocket::accept() {}