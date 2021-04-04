#include "PortHandler.hpp"

#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <sys/types.h>

Packet PortHandler::recvPacketFrom(std::string address, int port) {
    std::unique_lock<std::mutex> am_lock(m_address_map);
    // if the message accesses are getting messed up, then I shouldn't wait on this one mutex.
    if(address_map[{address, port}].empty())
        cv_address_map_queue_isEmpty[{address, port}].wait(am_lock);   
    Packet p = address_map[{address, port}].front();
    address_map[{address, port}].pop();
    return p;
}

Packet PortHandler::recvPacketFrom(std::string address, int port, int time_ms) {
    std::unique_lock<std::mutex> am_lock(m_address_map);
    // if the message accesses are getting messed up, then I shouldn't wait on this one mutex.
    if(address_map[{address, port}].empty()){
        if(cv_address_map_queue_isEmpty[{address, port}].wait_for(am_lock, std::chrono::milliseconds(time_ms)) == std::cv_status::timeout){
            throw "Timeout Exceeded";
        }
    }
    Packet p = address_map[{address, port}].front();
    address_map[{address, port}].pop();
    return p;
}

void PortHandler::sendPacketTo(Packet packet, std::string address, int port) {
    std::unique_lock<std::mutex> sq_lock(m_send_queue);
    send_queue.push({packet, {address, port}});
    sq_lock.unlock();
    cv_send_queue_isEmpty.notify_all();
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
        int port = htons(src_addr.sin_port);
        std::string address = inet_ntoa(src_addr.sin_addr);
        // Could be ntohs
        std::unique_lock<std::mutex> am_lock(m_address_map);

        if (address_map.find({address, port}) != address_map.end()) {
            address_map[{address, port}].push(Packet(buff, len));
            am_lock.unlock();
            cv_address_map_queue_isEmpty[{address, port}].notify_one();
        } else {
            am_lock.unlock();
            // Locks when constructor is called.
            std::unique_lock<std::mutex> cq_lock(m_connect_queue);
            if(connect_queue.size() >= MAX_WAITING_REQUEST){
                // Reject Client
            }
            connect_queue.push({{address, port}, Packet(buff, len)});
            cq_lock.unlock();
            cv_connect_queue_isEmpty.notify_all();
        }

        if (threadEnd) {
            break;
        }
    }
}

void PortHandler::sendThreadFunction() {
    while (1) {
        std::unique_lock<std::mutex> sq_lock(m_send_queue);
        if(send_queue.empty()){
            cv_send_queue_isEmpty.wait(sq_lock);
        }


        char *data = (char *)send_queue.front().first.packet_struct;
        int len = send_queue.front().first.packet_length;
        std::string to_address = send_queue.front().second.first;
        int to_port = send_queue.front().second.second;
        send_queue.pop();
        sq_lock.unlock();
        sockaddr_in dest_addr;
        dest_addr.sin_family = AF_INET;
        // dest_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        inet_pton(AF_INET, to_address.c_str(), &dest_addr.sin_addr);
        dest_addr.sin_port = htons(to_port);
        Packet tempPack(data, len);
        std::cout << "Sent Packet" << to_address << ' ' << to_port << tempPack << std::endl;
        sendto(sockfd, data, len, 0, (sockaddr *)&dest_addr,
                (socklen_t)sizeof(dest_addr));
        // This may not run. Use another condition variable?
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

void PortHandler::makeAddressConnected(std::string address, int port) {
    std::unique_lock<std::mutex> am_lock(m_address_map);
    address_map[{address, port}] = std::queue<Packet>();

}

void PortHandler::deleteAddressQueue(std::string address, int port){
    std::unique_lock<std::mutex> am_lock(m_address_map);
    address_map.erase({address, port});
}

// Make this blocking somehow? Maybe have a mutex on the length of the queue?
std::pair<std::pair<std::string, int>, Packet> PortHandler::getNewConnection() {
    std::unique_lock<std::mutex> cq_lock(m_connect_queue);
    while(connect_queue.empty())
        cv_connect_queue_isEmpty.wait(cq_lock);
    std::pair<std::pair<std::string, int>, Packet> new_connection =
    connect_queue.front();
    connect_queue.pop();
    cq_lock.unlock();
    return new_connection;
}