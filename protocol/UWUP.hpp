#pragma once
#include "PortHandler.hpp"
#include <queue>
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

/**
 * Socket Class for the UWUP
 */
class UWUPSocket {
    /// socket descriptor ( a UDP socket)

    int sockfd;
    /// flag to check if a connection has been established on the socket
    int my_port;
    /// port of the peer to which the connection had been established to
    bool is_listen;
    /// the sequence number of the next socket
    int seq;
    /// the peer address
    std::string peer_address;
    /// the peer port
    int peer_port;
    /// Port Handler
    PortHandler *port_handler;
    /// Send Window
    std::vector<Packet> send_window;
    /// Receive Window
    std::vector<Packet> receive_window;
    /// Send Queue
    std::queue<Packet> send_queue;

    friend std::ostream &operator<<(std::ostream &os, UWUPSocket const &sock);

  public:
    /**
     * Constructor for client socket
     */
    UWUPSocket();

    /**
     * Constructor to create a duplicate socket for a new client
     */
    UWUPSocket(int sockfd, std::string peer_address, int peer_port,
               PortHandler *port_handler, int seq);

    /**
     * A function that accepts a connection and returns a connected socket
     */
    UWUPSocket accept();

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
};
