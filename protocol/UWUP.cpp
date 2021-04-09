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

UWUPSocket::UWUPSocket() {

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
                       PortHandler *port_handler, int seq)
    : sockfd(sockfd), peer_address(peer_address), peer_port(peer_port),
      port_handler(port_handler), seq(seq) {}

void UWUPSocket::listen(int port) {
    my_port = port;
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

/// init the sockaddr_in struct and send handshake.
void UWUPSocket::connect(std::string peer_address, int peer_port) {
    this->peer_address = peer_address;
    this->peer_port = peer_port;
    port_handler->makeAddressConnected(peer_address, peer_port);

    try {
        this->seq = rand() % 100;
        char msg1[] = "SYN packet";
        Packet synPacket(0, this->seq, SYN, 0, msg1, sizeof(msg1));
        port_handler->sendPacketTo(synPacket, peer_address, peer_port);
        Packet synAckPacket =
            port_handler->recvPacketFrom(peer_address, peer_port, 10000);
        std::cout << synAckPacket << std::endl;
        if (!(synAckPacket.flags & (SYN | ACK)) ||
            synAckPacket.ack_number != this->seq)
            throw connection_exception("invalid handshake packet");
        char msg2[] = "ACK packet";
        Packet ackPacket(synAckPacket.seq_number, ++this->seq, ACK, 0, msg2,
                         sizeof(msg2));
        port_handler->sendPacketTo(ackPacket, peer_address, peer_port);
    } catch (const std::runtime_error &e) {
        port_handler->deleteAddressQueue(peer_address, peer_port);

        std::cerr << e.what() << '\n';
    }
}

UWUPSocket UWUPSocket::accept() {
    std::pair<std::pair<std::string, int>, Packet> new_connection =
        port_handler->getNewConnection();

    std::cout << new_connection.first.first << ' '
              << new_connection.first.second << new_connection.second
              << std::endl;
    std::string cli_addr = new_connection.first.first;
    int cli_port = new_connection.first.second;

    Packet synPacket = new_connection.second;
    if (!(synPacket.flags & SYN))
        throw connection_exception("invalid handshake packet");
    char msg[] = "SYN | ACK packet";
    port_handler->makeAddressConnected(cli_addr, cli_port);

    int seq_no = rand() % 100;
    Packet synAckPacket =
        Packet(synPacket.seq_number, seq_no++, SYN | ACK, 0, msg, sizeof(msg));
    port_handler->sendPacketTo(synAckPacket, cli_addr, cli_port);

    try {
        Packet ackpacket =
            port_handler->recvPacketFrom(cli_addr, cli_port, 10000);
        std::cout << ackpacket << std::endl;
    } catch (const std::exception &e) {
        port_handler->deleteAddressQueue(cli_addr, cli_port);

        std::cerr << e.what() << '\n';
    }
    return UWUPSocket(sockfd, cli_addr, cli_port, port_handler, seq_no);
}

std::ostream &operator<<(std::ostream &os, UWUPSocket const &sock) {
    os << "{\n"
       << "sockfd:" << sock.sockfd << "\npeer:" << sock.peer_address << ':'
       << sock.peer_port << "\n}" << std::endl;
    return os;
}
