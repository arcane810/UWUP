#include "UWUP.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <netdb.h>
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

    rng = new std::mt19937(
        std::chrono::steady_clock::now().time_since_epoch().count());
}

UWUPSocket::UWUPSocket(int sockfd, std::string peer_address, int peer_port,
                       PortHandler *port_handler, uint32_t base_seq)
    : sockfd(sockfd), peer_address(peer_address), peer_port(peer_port),
      current_seq(base_seq), base_seq(base_seq), port_handler(port_handler) {
    thread_end = false;
    receive_thread = std::thread(&UWUPSocket::selectiveRepeatReceive, this);
    send_thread = std::thread(&UWUPSocket::selectiveRepeatSend, this);
}

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

UWUPSocket::UWUPSocket(const UWUPSocket &sock) {
    sockfd = sock.sockfd;
    peer_address = sock.peer_address;
    peer_port = sock.peer_port;
    port_handler = sock.port_handler;
    base_seq = sock.base_seq;
}

UWUPSocket::~UWUPSocket() {
    thread_end = true;
    send_thread.join();
    receive_thread.join();
}

void UWUPSocket::selectiveRepeatReceive() {
    receive_window.resize(MAX_SEND_WINDOW, Packet(0, 0, 0, 0, nullptr, 0));
    while (1) {
        if (thread_end) {
            break;
        }
        Packet new_packet =
            port_handler->recvPacketFrom(peer_address, peer_port);
        if (new_packet.flags & ACK) {
            std::unique_lock<std::mutex> send_window_lock(m_send_window);
            while (base_seq < new_packet.ack_number) {
                send_window[base_seq % MAX_SEND_WINDOW].first.status = INACTIVE;
                base_seq++;
                num_packets--;
            }
            send_window_lock.unlock();
        } else if ((new_packet.flags & SYN) || (new_packet.flags & FIN)) {
            std::cout << "Unexpected packet\n" << new_packet << std::endl;
        } else {
            std::unique_lock<std::mutex> receive_window_lock(m_receive_window);

            // TODO: Add locks for windowSize()
            if (new_packet.seq_number < next_seq_exp + windowSize()) {
                new_packet.status = RECIEVED;
                receive_window[new_packet.seq_number % MAX_SEND_WINDOW] =
                    new_packet;
            }
            while (receive_window[next_seq_exp % MAX_SEND_WINDOW].status ==
                   RECIEVED) {
                std::unique_lock<std::mutex> receive_queue_lock(
                    m_receive_queue);

                receive_queue.push(
                    receive_window[next_seq_exp % MAX_SEND_WINDOW]);

                receive_queue_lock.unlock();

                receive_window[next_seq_exp % MAX_SEND_WINDOW].status =
                    INACTIVE;
                next_seq_exp++;
            }
            receive_window_lock.unlock();
            std::cout << "Non-Flagged Packet " << new_packet << std::endl;
            char msg[] = "ACK packet";
            Packet resp_packet(next_seq_exp, 0, ACK, 20, msg, strlen(msg));
            port_handler->sendPacketTo(resp_packet, peer_address, peer_port);
        }
    }
}

int UWUPSocket::windowSize() {
    // std::unique_lock<std::mutex> window_size_lock(m_window_size);
    int size = 5;
    // window_size_lock.unlock();
    return size;
}

void UWUPSocket::selectiveRepeatSend() {
    send_window.resize(MAX_SEND_WINDOW,
                       std::make_pair(Packet(0, 0, 0, 0, nullptr, 0), 0));

    int front = base_seq % MAX_SEND_WINDOW;
    num_packets = 0;

    std::cout << "Starting SR" << std::endl;
    while (1) {
        if (thread_end) {
            break;
        }

        std::unique_lock<std::mutex> send_queue_lock(m_send_queue);
        // If we have no packets that need to be serviced, wait till
        // application layer sends packets
        if (num_packets == 0 && send_queue.empty()) {
            std::cout << "Waiting for Packets to send" << std::endl;
            cv_send_queue_isEmpty.wait(send_queue_lock);
        }

        std::unique_lock<std::mutex> window_size_lock(m_window_size,
                                                      std::defer_lock);
        std::unique_lock<std::mutex> send_window_lock(m_send_window,
                                                      std::defer_lock);
        std::lock(send_window_lock, window_size_lock);
        int window_size = windowSize();
        // Add packets to window while we have packets from application
        // layer and window isn't full
        while (!send_queue.empty() && num_packets < window_size) {
            if (send_window[front].first.status == INACTIVE) {
                Packet new_packet = send_queue.front();
                // std::cout << "Packet from queue " << new_packet << base << '
                // '
                //           << front << std::endl;
                new_packet.status = NOT_SENT;
                send_queue.pop();
                send_window[front] = {new_packet, 0};
                num_packets++;
                front = (front + 1) % MAX_SEND_WINDOW;
            } else {
                std::cout << base_seq << ' ' << front << std::endl;
                // Should never reach here if window size is correct
                throw std::runtime_error(
                    "Packet count doesn't match window status");
            }
        }
        send_window_lock.unlock();
        window_size_lock.unlock();
        send_queue_lock.unlock();

        send_window_lock.lock();
        for (int i = base_seq; i < base_seq + window_size; i++) {
            int window_position = i % MAX_SEND_WINDOW;
            Packet packet = send_window[window_position].first;
            int64_t time_sent = send_window[window_position].second;
            int64_t time_now =
                std::chrono::steady_clock::now().time_since_epoch().count();
            if (packet.status == INACTIVE) {
                break;
            } else if (packet.status == NOT_SENT) {
                send_window[window_position].second = time_now;
                send_window[window_position].first.status = NOT_ACKED;
                // IDK why, but *sometimes* the value doesn't get set if I do
                // this. SEQ number set in send for now

                packet.seq_number = current_seq++;
                port_handler->sendPacketTo(packet, peer_address, peer_port);
            } else if (packet.status == NOT_ACKED) {
                // std::cout << "Packet Not Acked\n" << packet << std::endl;
                int64_t time_since_in_ms = (time_now - time_sent) / 1'000'000;
                if (time_since_in_ms > TIMEOUT) {
                    std::cout << "TIMEOUT!" << packet.seq_number << std::endl;

                    send_window[window_position].second = time_now;
                    port_handler->sendPacketTo(packet, peer_address, peer_port);
                }
            }
        }
        send_window_lock.unlock();
    }
}

/// init the sockaddr_in struct and send handshake.
void UWUPSocket::connect(std::string peer_address, int peer_port) {
    this->peer_address = peer_address;
    this->peer_port = peer_port;
    port_handler->makeAddressConnected(peer_address, peer_port);

    try {
        this->base_seq = std::uniform_int_distribution<int>(0, 99)(*rng);
        next_seq_exp = -1;
        int tries = 8, timeout = 100;
        while (tries--) {
            char msg1[] = "SYN packet";
            Packet syn_packet(0, this->base_seq, SYN, 0, msg1, sizeof(msg1));
            port_handler->sendPacketTo(syn_packet, peer_address, peer_port);
            try {
                Packet syn_ack_packet = port_handler->recvPacketFrom(
                    peer_address, peer_port, timeout);
                // std::cout << syn_ack_packet << std::endl;
                if ((syn_ack_packet.flags & (SYN | ACK)) ||
                    syn_ack_packet.ack_number != this->base_seq) {
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
        if (next_seq_exp == -1) {
            throw std::runtime_error("Couldn't connect!");
        }
        char msg2[] = "ACK packet";
        Packet ackPacket(next_seq_exp, ++this->base_seq, ACK, 0, msg2,
                         sizeof(msg2));
        port_handler->sendPacketTo(ackPacket, peer_address, peer_port);
    } catch (const std::runtime_error &e) {
        port_handler->deleteAddressQueue(peer_address, peer_port);

        std::cerr << e.what() << std::endl;
    }
    receive_thread = std::thread(&UWUPSocket::selectiveRepeatReceive, this);
    send_thread = std::thread(&UWUPSocket::selectiveRepeatSend, this);
}

UWUPSocket *UWUPSocket::accept() {
    std::cout << "Accept Called\n";
    std::pair<std::pair<std::string, int>, Packet> new_connection =
        port_handler->getNewConnection();

    std::cout << "Connection Request :" << new_connection.first.first << ' '
              << new_connection.first.second << std::endl;
    std::string cli_addr = new_connection.first.first;
    int cli_port = new_connection.first.second;
    next_seq_exp = new_connection.second.seq_number + 1;
    Packet synPacket = new_connection.second;
    if (!(synPacket.flags & SYN))
        throw connection_exception("invalid handshake packet");
    char msg[] = "SYN | ACK packet";
    port_handler->makeAddressConnected(cli_addr, cli_port);

    uint32_t seq_no = rand() % 100;
    // Retry if packet is dropped.
    Packet synAckPacket =
        Packet(synPacket.seq_number, seq_no++, SYN | ACK, 0, msg, sizeof(msg));
    port_handler->sendPacketTo(synAckPacket, cli_addr, cli_port);

    try {
        Packet ackpacket =
            port_handler->recvPacketFrom(cli_addr, cli_port, 10000);
    } catch (const std::exception &e) {
        port_handler->deleteAddressQueue(cli_addr, cli_port);
        std::cerr << e.what() << std::endl;
    }
    // Use new here?
    UWUPSocket *sock =
        new UWUPSocket(sockfd, cli_addr, cli_port, port_handler, seq_no);
    return sock;
}

std::ostream &operator<<(std::ostream &os, UWUPSocket const &sock) {
    os << "{\n"
       << "sockfd:" << sock.sockfd << "\npeer:" << sock.peer_address << ':'
       << sock.peer_port << "\n}" << std::endl;
    return os;
}
/// @Todo wait if queue is full.
void UWUPSocket::send(char *data, int len) {
    while (len > 0) {
        int max_size = std::min(len, MAX_PACKET_SIZE);
        // wait till queue has space
        std::unique_lock<std::mutex> send_queue_lock(m_send_queue);
        send_queue.push(Packet(0, current_seq, 0, 0, data, max_size));
        current_seq++;
        cv_send_queue_isEmpty.notify_one();
        data += max_size;
        len -= max_size;
    }
}