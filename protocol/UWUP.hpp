#pragma once
#include "PortHandler.hpp"
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <random>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

class connection_exception : public std::runtime_error {
  public:
    connection_exception() : runtime_error("connection exception") {}
    connection_exception(const std::string &msg)
        : runtime_error("connection exception:" + msg) {}
};

const int32_t TIMEOUT = 1000;
const int32_t MAX_SEND_WINDOW = 10;

/**
 * Socket Class for the UWUP
 */
class UWUPSocket {
    /// Thread handling sending ops for selective repeat
    std::thread send_thread;
    /// Thread handling receive operations for selective repeat
    std::thread receive_thread;
    /// socket descriptor ( a UDP socket)
    int sockfd;
    /// flag to check if a connection has been established on the socket
    int my_port;
    /// port of the peer to which the connection had been established to
    bool is_listen;
    /// thread end flag
    bool thread_end;
    /// the sequence number of the next socket
    uint32_t base_seq;
    /// next seq no of ongoing SR
    uint32_t current_seq;
    /// Last Confirmed seq. The window has moved past this packet. Acquire
    /// m_send_window to use.
    uint32_t last_confirmed_seq = 0;
    /// peer's seq no. Established during handshake.
    uint32_t peer_seq;
    /// Send Window
    std::vector<std::pair<Packet, int64_t>> send_window;
    /// Receive Window
    std::vector<Packet> receive_window;
    /// Send Queue
    std::queue<Packet> send_queue;
    /// mutex for receive queue access
    std::mutex m_receive_queue;
    /// Receive Queue
    std::queue<Packet> receive_queue;
    /// mutex for send queue access
    std::mutex m_send_queue;
    /// mutex for send queue access
    std::mutex m_send_window;
    /// mutex for recieve window
    std::mutex m_window_size;
    /// CV for send queue
    std::condition_variable cv_send_queue_isEmpty;
    /// CV for recv queue
    std::condition_variable cv_receive_queue_isFull;
    /// Random object for starting sequence numbers
    std::mt19937 *rng;

    friend std::ostream &operator<<(std::ostream &os, UWUPSocket const &sock);

    void selectiveRepeatSend();
    void selectiveRepeatReceive();

    int windowSize();

  public:
    /// Port Handler
    PortHandler *port_handler;
    /// the peer address
    std::string peer_address;
    /// the peer port
    int peer_port;
    /**
     * Constructor for client socket
     */
    UWUPSocket();
    /**
     * Copy Constructor for client socket since we utilize unmovable/uncopyable
     * members
     */
    UWUPSocket(const UWUPSocket &sock);
    /**
     * Destructor
     */
    ~UWUPSocket();
    /**
     * Constructor to create a duplicate socket for a new client
     */
    UWUPSocket(int sockfd, std::string peer_address, int peer_port,
               PortHandler *port_handler, uint32_t current_seq,
               uint32_t peer_seq);

    /**
     * A function that accepts a connection and returns a connected socket
     */
    UWUPSocket *accept();

    /**
     * A function that binds to the local address and marks socket as passive.
     */
    void listen(int my_port);

    /**
     * A function that starts a connection with a peer
     * @param peer_address ip of the
     */
    void connect(std::string peer_address, int peer_port);

    /**
     * Function to send the packet to the connected peer
     */
    void send(char *data, int len);
    /**
     * Function to send the packet to the connected peer
     */
    int recv(char *data, int len);
};
