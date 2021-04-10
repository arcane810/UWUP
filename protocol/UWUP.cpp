#include "UWUP.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <random>
#include <sys/types.h>
#include <unistd.h>

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
        int next_seq_exp = -1;
        int tries = 8, timeout = 100;
        while (tries--) {
            char msg1[] = "SYN packet";
            Packet syn_packet(0, this->seq, SYN, 0, msg1, sizeof(msg1));
            port_handler->sendPacketTo(syn_packet, peer_address, peer_port);
            try {
                Packet syn_ack_packet = port_handler->recvPacketFrom(
                    peer_address, peer_port, timeout);
                std::cout << syn_ack_packet << std::endl;
                if ((syn_ack_packet.flags & (SYN | ACK)) ||
                    syn_ack_packet.ack_number != this->seq) {
                    next_seq_exp = syn_ack_packet.seq_number + 1;
                    break;
                }
            } catch (timeout_exception e) {
                std::cerr << "SYN timeout in " << timeout << " ms."
                          << std::endl;
                timeout *= 2;
            } catch (const std::exception &e) {
                std::cerr << e.what() << std::endl;
            }
        }
        char msg2[] = "ACK packet";
        Packet ackPacket(next_seq_exp, ++this->seq, ACK, 0, msg2, sizeof(msg2));
        port_handler->sendPacketTo(ackPacket, peer_address, peer_port);
    } catch (const std::runtime_error &e) {
        port_handler->deleteAddressQueue(peer_address, peer_port);

        std::cerr << e.what() << std::endl;
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

        std::cerr << e.what() << std::endl;
    }
    return UWUPSocket(sockfd, cli_addr, cli_port, port_handler, seq_no);
}

std::ostream &operator<<(std::ostream &os, UWUPSocket const &sock) {
    os << "{\n"
       << "sockfd:" << sock.sockfd << "\npeer:" << sock.peer_address << ':'
       << sock.peer_port << "\n}" << std::endl;
    return os;
}

void UWUPSocket::send(char *data, int len) {
    while (len > 0) {
        int max_size = std::min(len, MAX_PACKET_SIZE);
        // wait till queue has space

        send_queue.push(Packet(data, max_size));
        data += max_size;
        len -= max_size;
    }
}