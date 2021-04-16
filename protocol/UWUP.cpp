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

    rng = std::mt19937(
        std::chrono::steady_clock::now().time_since_epoch().count());
}

// _Please_ yeet
UWUPSocket::UWUPSocket(int sockfd, std::string peer_address, int peer_port,
                       PortHandler *port_handler, uint32_t base_seq,
                       uint32_t peer_seq, uint32_t send_window_size)
    : sockfd(sockfd), peer_address(peer_address), peer_port(peer_port),
      port_handler(port_handler), current_seq(base_seq), base_seq(base_seq),
      peer_seq(peer_seq), send_window_size(send_window_size) {
    thread_end = false;
    receive_thread = std::thread(&UWUPSocket::selectiveRepeatReceive, this);
    send_thread = std::thread(&UWUPSocket::selectiveRepeatSend, this);
}

UWUPSocket::~UWUPSocket() {
    thread_end = true;
    send_thread.join();
    receive_thread.join();
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
    peer_seq = sock.peer_seq;
    send_window_size = sock.send_window_size;
}

void UWUPSocket::selectiveRepeatReceive() {
    std::vector<Packet> buffer;
    int ASSUMED_WINDOW = MAX_SEND_WINDOW / 2;
    // Should I make this larger?
    buffer.resize(ASSUMED_WINDOW + 1, Packet(0, 0, 0, 0, nullptr, 0));
    while (1) {
        if (thread_end)
            break;
        Packet new_packet =
            port_handler->recvPacketFrom(peer_address, peer_port);
        // std::cout << "Got  packet " << new_packet << std::endl;

        if (new_packet.flags & ACK) {

            int pk_no = (new_packet.ack_number - base_seq) % MAX_SEND_WINDOW;
            // std::cout << "Got ack packet " << new_packet << pk_no <<
            // std::endl;
            // Lock for `last_confirmed_seq`
            std::unique_lock<std::mutex> send_window_lock(m_send_window);
            // drop duplicate acks
            if (new_packet.ack_number <= last_confirmed_seq) {
                std::cout << "Duplicate ACK " << new_packet.ack_number
                          << " LCS " << last_confirmed_seq << std::endl;
                send_window_lock.unlock();
                continue;
            }
            send_window[pk_no].first.status = ACKED;
            send_window_lock.unlock();
        } else if (new_packet.flags & (FIN | ACK)) {
            thread_end = true;
            std::unique_lock<std::mutex> send_window_lock(m_send_window,
                                                          std::defer_lock);
            std::unique_lock<std::mutex> send_queue_lock(m_send_queue,
                                                         std::defer_lock);
            std::lock(send_window_lock, send_queue_lock);

            // Prevent possible infinite wait in selectiveRepeatSend
            cv_send_queue_isEmpty.notify_all();

            int window_size = windowSize();
            bool inflight_packets = false;
            for (int i = last_confirmed_seq + 1;
                 i < last_confirmed_seq + window_size + 1; i++) {
                int current_position = i % MAX_SEND_WINDOW;
                if (send_window[current_position].first.status == NOT_ACKED) {
                    inflight_packets = true;
                }
            }
            if (!send_queue.empty() || inflight_packets) {
                throw connection_exception(
                    "Unexpected Error: Peer closed connection, but unsent "
                    "packets present!");
            }
            send_window_lock.unlock();
            send_queue_lock.unlock();
        } else if ((new_packet.flags & SYN)) {
            std::cout << "Unexpected packet\n" << new_packet << std::endl;
        } else {

            // TODO: Add locks for windowSize()
            // int r = rng() % 10;
            // std::cout << "Rand " << r << std::endl;
            // if (r < 5) {
            //     std::cout << "Dropping " << new_packet.seq_number <<
            //     std::endl; continue;
            // }
            // std::cout << "Sending " << new_packet.seq_number << std::endl;

            char msg[] = "ACK packet";
            Packet resp_packet(new_packet.seq_number, 0, ACK, 20, msg,
                               strlen(msg));
            port_handler->sendPacketTo(resp_packet, peer_address, peer_port);
            if (new_packet.seq_number < peer_seq + windowSize()) {
                new_packet.status = RECEIVED;
                buffer[new_packet.seq_number % MAX_SEND_WINDOW] = new_packet;
            }
            while (buffer[peer_seq % MAX_SEND_WINDOW].status == RECEIVED) {
                std::unique_lock<std::mutex> receive_queue_lock(
                    m_receive_queue);
                std::cout << "Received "
                          << buffer[peer_seq % MAX_SEND_WINDOW].seq_number
                          << std::endl;
                receive_queue.push(buffer[peer_seq % MAX_SEND_WINDOW]);

                receive_queue_lock.unlock();

                buffer[peer_seq % MAX_SEND_WINDOW].status = INACTIVE;
                peer_seq++;
            }
            // receive_window_lock.unlock();
            // std::cout << "Non-Flagged Packet " << new_packet << std::endl;
        }
    }
}

int UWUPSocket::windowSize() {
    std::unique_lock<std::mutex> window_size_lock(m_window_size);
    return send_window_size;
}

void UWUPSocket::setWindowSize(uint32_t window_size) {
    std::unique_lock<std::mutex> window_size_lock(m_window_size);
    send_window_size = std::min(DEFALT_WINDOW_SIZE, window_size);
    window_size_lock.unlock();
}

void UWUPSocket::selectiveRepeatSend() {
    send_window.resize(MAX_SEND_WINDOW,
                       std::make_pair(Packet(0, 0, 0, 0, nullptr, 0), 0));

    int base = 0;
    int front = 0;
    int num_packets = 0;

    std::cout << "Starting SR" << std::endl;
    while (1) {
        if (thread_end)
            break;
        std::unique_lock<std::mutex> send_window_lock(m_send_window);
        // Remove packets that have been ACKd from window
        while (send_window[base].first.status == ACKED) {
            std::cout << "ACKED!" << send_window[base].first.seq_number
                      << std::endl;
            last_confirmed_seq = send_window[base].first.seq_number;
            send_window[base].first.status = INACTIVE;
            base = (base + 1) % MAX_SEND_WINDOW;
            num_packets--;
        }
        send_window_lock.unlock();

        std::unique_lock<std::mutex> send_queue_lock(m_send_queue);
        // If we have no packets that need to be serviced, wait till
        // application layer sends packets
        if (num_packets == 0 && send_queue.empty()) {
            std::cout << "Waiting for Packets to send" << std::endl;
            cv_send_queue_isEmpty.wait(send_queue_lock);
            std::cout << "Got Packets to send" << std::endl;
            if (thread_end)
                break;
        }
        int window_size = windowSize();

        send_window_lock.lock();
        // Add packets to window while we have packets from application
        // layer and window isn't full
        while (!send_queue.empty() && num_packets < window_size) {
            if (num_packets >= window_size - 2) {
                break;
            }
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
                std::cout << base << ' ' << front << std::endl;
                // Should never reach here if window size is correct
                throw std::runtime_error(
                    "Packet count doesn't match window status");
            }
        }
        send_window_lock.unlock();
        send_queue_lock.unlock();
        // If seq number not updated, packet is till sent.
        send_window_lock.lock();
        for (int i = base; i < base + window_size; i++) {
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

                // packet.seq_number = current_seq++;
                port_handler->sendPacketTo(packet, peer_address, peer_port);
            } else if (packet.status == NOT_ACKED) {
                // std::cout << "Packet Not Acked\n" << packet << std::endl;
                int64_t time_since_in_ms = (time_now - time_sent) / 1'000'000;
                if (time_since_in_ms > TIMEOUT) {
                    std::cout << "TIMEOUT!" << packet.seq_number << std::endl;

                    send_window[window_position].second = time_now;
                    port_handler->sendPacketTo(packet, peer_address, peer_port);
                }
            } else if (packet.status == ACKED) {
                // num_packets--;
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
        this->base_seq = rand() % 100;
        int next_seq_exp = -1;
        int tries = 8, timeout = 100;
        while (tries--) {
            char msg1[] = "SYN packet";
            Packet syn_packet(0, this->base_seq, SYN, DEFALT_WINDOW_SIZE, msg1,
                              sizeof(msg1));
            port_handler->sendPacketTo(syn_packet, peer_address, peer_port);
            try {
                Packet syn_ack_packet = port_handler->recvPacketFrom(
                    peer_address, peer_port, timeout);
                // Wat this Akool
                if ((syn_ack_packet.flags & (SYN | ACK)) ||
                    syn_ack_packet.ack_number != this->base_seq) {
                    peer_seq = syn_ack_packet.seq_number + 1;
                    next_seq_exp = syn_ack_packet.seq_number + 1;
                    setWindowSize(syn_ack_packet.rwnd);
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
        base_seq++;
        char msg2[] = "ACK packet";
        Packet ackPacket(next_seq_exp, base_seq, ACK, 0, msg2, sizeof(msg2));
        port_handler->sendPacketTo(ackPacket, peer_address, peer_port);
    } catch (const std::runtime_error &e) {
        port_handler->deleteAddressQueue(peer_address, peer_port);

        std::cerr << e.what() << std::endl;
    }
    receive_thread = std::thread(&UWUPSocket::selectiveRepeatReceive, this);
    send_thread = std::thread(&UWUPSocket::selectiveRepeatSend, this);
}

UWUPSocket *UWUPSocket::accept() {
    std::pair<std::pair<std::string, int>, Packet> new_connection =
        port_handler->getNewConnection();

    std::cout << "Connection Request :" << new_connection.first.first << ' '
              << new_connection.first.second << std::endl;
    std::string cli_addr = new_connection.first.first;
    int cli_port = new_connection.first.second;

    Packet synPacket = new_connection.second;
    if (!(synPacket.flags & SYN))
        throw connection_exception("invalid handshake packet");
    uint32_t peer_seq_no = synPacket.seq_number;
    uint32_t window = synPacket.rwnd;
    char msg[] = "SYN | ACK packet";
    port_handler->makeAddressConnected(cli_addr, cli_port);

    uint32_t seq_no = std::uniform_int_distribution<uint32_t>(0, 99)(rng);
    // Retry if packet is dropped.
    int tries = 8, timeout = 100;
    while (tries--) {
        Packet synAckPacket = Packet(synPacket.seq_number, seq_no++, SYN | ACK,
                                     DEFALT_WINDOW_SIZE, msg, sizeof(msg));
        port_handler->sendPacketTo(synAckPacket, cli_addr, cli_port);

        try {
            Packet ackpacket =
                port_handler->recvPacketFrom(cli_addr, cli_port, timeout);
            if ((ackpacket.flags & ACK)) {
                peer_seq_no++;
                break;
            }

        } catch (timeout_exception e) {
            std::cerr << "SYN|ACK timeout in " << timeout << " ms."
                      << std::endl;
            timeout *= 2;
        } catch (const std::exception &e) {
            port_handler->deleteAddressQueue(cli_addr, cli_port);
            std::cerr << e.what() << std::endl;
        }
    }
    UWUPSocket *socket = new UWUPSocket(
        sockfd, cli_addr, cli_port, port_handler, seq_no, peer_seq_no, window);
    return socket;
}

std::ostream &operator<<(std::ostream &os, UWUPSocket const &sock) {
    os << "{\n"
       << "sockfd:" << sock.sockfd << "\nseq:" << sock.base_seq
       << "\npeer:" << sock.peer_address << ':' << sock.peer_port << "\n}"
       << std::endl;
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

// Fix the busy wait here
int UWUPSocket::recv(char *data, int len) {
    std::unique_lock<std::mutex> receive_queue_lock(m_receive_queue,
                                                    std::defer_lock);
    int added = 0;
    while (1) {
        receive_queue_lock.lock();
        if (receive_queue.empty()) {
            receive_queue_lock.unlock();
            continue;
        }
        memcpy(data, receive_queue.front().packet_struct->data,
               receive_queue.front().data_len);
        // std::cout << "PACKET: " << receive_queue.front() << std::endl;
        data += receive_queue.front().data_len;
        added += receive_queue.front().data_len;
        receive_queue.pop();
        break;
    }
    std::cout << "DONE RECV: " << added << std::endl;
    return added;
}