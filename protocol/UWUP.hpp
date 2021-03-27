#pragma once
#include <queue>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

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
    int peer_address;

  public:
    /**
     * Constructor for client socket
     */
    UWUPSocket();
    /**
     * Constructor for server socket
     */
    UWUPSocket(int my_port);
    /**
     * A function that accepts a connection and returns a connected socket
     */
    UWUPSocket accept();
    /**
     * A function that starts a connection with a peer
     */
    UWUPSocket connect(std::string peer_address, int peer_port);
};
