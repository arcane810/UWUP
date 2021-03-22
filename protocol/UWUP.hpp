#pragma once
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

class UWUPSocket {
    int sockfd;
    bool is_connected;
    int my_port;
    std::string peer_address;
    int peer_port;
    bool is_listen;
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
    UWUPSocket accept();
    int connect(std::string peer_address, int peer_port);
};
