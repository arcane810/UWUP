#pragma once
#include "Packet.hpp"
#include <map>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <utility>


const int MAX_WAITING_REQUEST = 5;
/**
 * A class to handle
 */
class PortHandler {
    const int MAX_PKT_SIZE = 1024;
    int sockfd;
    /// Data Structure mapping received packets to an address, port pair.
    std::map<std::pair<std::string, int>, std::queue<Packet>> address_map;
    std::map<std::pair<std::string, int>, std::condition_variable> cv_address_map_queue_isEmpty;
    std::queue<std::pair<std::pair<std::string, int>, Packet>> connect_queue;
    std::thread recvThread;
    std::thread sendThread;
    bool threadEnd;
    std::mutex m_connect_queue;
    std::condition_variable cv_connect_queue_isEmpty;
    std::mutex m_send_queue;
    std::condition_variable cv_send_queue_isEmpty;
    std::mutex m_address_map;
    std::queue<std::pair<Packet, std::pair<std::string, int>>> send_queue;
    void recvThreadFunction();
    void sendThreadFunction();

  public:
    PortHandler(int sockfd);
    ~PortHandler();
    /**
     * Returns packets received from passed address and port. Throws exception
     * if no packets to return.
     */
    Packet recvPacketFrom(std::string address, int port);
    Packet recvPacketFrom(std::string address, int port,int time_ms);
    void sendPacketTo(Packet packet, std::string address, int port);
    void makeAddressConnected(std::string address, int port);
    void deleteAddressQueue(std::string address, int port);

    std::pair<std::pair<std::string, int>, Packet> getNewConnection();
};