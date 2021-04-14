#include "./protocol/UWUP.hpp"
#include <thread>

#include <iostream>
#include <string>

int main() {

    UWUPSocket socket;
    socket.connect("127.0.0.1", 8080);
    int i = 50;
    // while (i--) {

    //     // std::string s = std::to_string(i);
    //     // char *msg = (char *)s.c_str();
    //     // // sprintf(msg1, "Initial SYN packet, client %d", i);
    //     // Packet packet(i, i, i, i, msg, s.length());
    //     // std::cout << packet << std::endl;

    //     std::this_thread::sleep_for(std::chrono::seconds(5));
    //     char msg[] = "Test Message";
    //     socket.send(msg, sizeof(msg));
    //     std::cout << "Send one packet" << std::endl;
    // }
    std::this_thread::sleep_for(std::chrono::seconds(600));

    return 0;
}