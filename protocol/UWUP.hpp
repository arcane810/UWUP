#pragma once
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
    bool is_connected;
    /// port on which the socket listens
    int my_port;
    /// address of the peer to which the connection had been established to
    std::string peer_address;
    /// port of the peer to which the connection had been established to
    int peer_port;
    /// flag to check if the socket is waiting to accept connections
    bool is_listen;
    /// the sequence number of the next socket
    int seq_number;

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
    int connect(std::string peer_address, int peer_port);
};
