#include "./protocol/UWUP.hpp"
// #include <arpa/inet.h>
// #include <iostream>
// #include <netdb.h>
// #include <sys/types.h>
#include <thread>

#include <iostream>
#include <string> 

int main() {
    // // Init socket
    // int sockfd;
    // int my_port = 8080;
    // if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    //     throw("Error creating socket");
    // }

    // std::cout << "Socket Created\n";

    // int optval = 1;
    // setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
    //            sizeof(int));
    // struct sockaddr_in servaddr;

    // servaddr.sin_family = AF_INET;
    // servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    // servaddr.sin_port = htons(my_port);


    // // Packet backet(pak, packet.packet_length);
    // // std::cout << backet << std::endl;
    // int i = 1;
    // while(i--){
    // char s[] = "Initial SYN packet, client 1";
    // int size = sizeof(s);
    // char *msg1;
    // sprintf(msg1, "Initial SYN packet, client %d", i);
    // Packet syn(3+i,4+i,SYN,5+i,msg1, size);
    // char *pak = (char *)malloc(syn.packet_length);
    // memcpy(pak, syn.packet_struct, syn.packet_length);
    // // std::cout << packet << packet.packet_length << std::endl;
    // sendto(sockfd, pak, syn.packet_length, MSG_CONFIRM, (sockaddr *)&servaddr,
    //        (socklen_t)sizeof(servaddr));
    // }
    
    // // while(1){
    //     sockaddr_in src_addr;
    //     socklen_t src_len = sizeof(src_addr);
    //     src_addr.sin_family = AF_INET;
    //     char buff[1024];
    //     int len = recvfrom(sockfd, buff, 1024, 0, (sockaddr *)&src_addr,
    //                        &src_len);
    //     std::cout << len << std::endl;
    //     if(len == -1){
    //         perror("some error");
    //         // continue;
    //     }
    //     Packet response(buff, len);
    //     std::cout << response << std::endl;
    
    // std::this_thread::sleep_for(std::chrono::seconds(3));
    //     char msg2[] = "YEEEEEEEE";
    //     Packet ack(3+i,4+i,ACK,5+i,msg2, sizeof(msg2));
    //     char *pak = (char *)malloc(ack.packet_length);
    //     memcpy(pak, ack.packet_struct, ack.packet_length);
    //     sendto(sockfd, pak, ack.packet_length, MSG_CONFIRM, (sockaddr *)&servaddr,
    //         (socklen_t)sizeof(servaddr));


    
    // }

    UWUPSocket socket;
    socket.connect("127.0.0.1", 8080);
    int i = 50;
    while(i--){
        
        std::string s = std::to_string(i);
        char *msg = (char*)s.c_str();
        // sprintf(msg1, "Initial SYN packet, client %d", i);
        Packet packet(i,i,i,i,msg, s.length());
        std::cout << packet  << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));

    return 0;
}