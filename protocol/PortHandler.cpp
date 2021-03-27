#include "PortHandler.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>

Packet PortHandler::recvPacketFrom(std::string address, int port) {
    if (!address_map[{address, port}].empty()) {
        m_address_map.lock();
        Packet p = address_map[{address, port}].front();
        address_map[{address, port}].pop();
        m_address_map.unlock();
        return p;
    } else {
        throw("No Packet in Queue");
    }
}

void PortHandler::sendPacketTo(Packet packet, std::string address, int port) {
    m_send_queue.lock();
    send_queue.push({packet, {address, port}});
    m_send_queue.unlock();
}

void PortHandler::recvThreadFunction() {
    sockaddr_in src_addr;
    int src_len = sizeof(src_addr);
    src_addr.sin_family = AF_INET;
    char buff[MAX_PKT_SIZE];
    while (1) {
        // Flag could be MSG_WAITALL
        int len = recvfrom(sockfd, buff, MAX_PKT_SIZE, 0, (sockaddr *)&src_addr,
                           (socklen_t *)&src_len);
        if (len < 0)
            continue;
        std::string address = inet_ntoa(src_addr.sin_addr);
        // Could be ntohs
        int port = htons(src_addr.sin_port);
        m_address_map.lock();
        address_map[{address, port}].push(Packet(buff, len));
        m_address_map.unlock();

        if (threadEnd) {
            break;
        }
    }
}

void PortHandler::sendThreadFunction() {
    while (1) {
        m_send_queue.lock();
        if (!send_queue.empty()) {
            char *data = (char *)send_queue.front().first.packet_struct;
            int len = send_queue.front().first.packet_length;
            std::string to_address = send_queue.front().second.first;
            int to_port = send_queue.front().second.second;
            send_queue.pop();
            m_send_queue.unlock();
            sockaddr_in dest_addr;
            int dest_len = sizeof(dest_addr);
            dest_addr.sin_family = AF_INET;
            inet_pton(AF_INET, to_address.c_str(), &dest_addr.sin_addr);
            dest_addr.sin_port = htons(to_port);
            sendto(sockfd, data, len, 0, (sockaddr *)&dest_addr,
                   (socklen_t)sizeof(dest_addr));
        } else {
            m_send_queue.unlock();
        }
        if (threadEnd) {
            break;
        }
    }
}

PortHandler::PortHandler(int sockfd) : sockfd(sockfd), threadEnd(false) {
    recvThread = std::thread(&PortHandler::recvThreadFunction, this);
    sendThread = std::thread(&PortHandler::sendThreadFunction, this);
}

PortHandler::~PortHandler() {
    threadEnd = true;
    recvThread.join();
    sendThread.join();
}