#include "Packet.hpp"
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <utility>

class PortHandler {
    const int MAX_PKT_SIZE = 1024;
    int sockfd;
    /// Data Structure mapping received packets to an address, port pair.
    std::map<std::pair<std::string, int>, std::queue<Packet>> address_map;
    std::thread recvThread;
    std::thread sendThread;
    bool threadEnd;
    std::mutex m_send_queue;
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
    void sendPacketTo(Packet packet, std::string address, int port);
};