#include "./protocol/PortHandler.hpp"
#include "./protocol/UWUP.hpp"

#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <sys/types.h>
#include <thread>

int main(int argc, char *argv[]) {
    // Init socket
    int sockfd;
    int my_port = 8080;
    UWUPSocket socket;
    socket.listen(my_port);
    auto sock = socket.accept();
    std::cout << "CONNECTION ESTABLISHED" << std::endl;
    FILE *fp = fopen("outfile.log", "rb");
    int len = 0;
    char data[1024];
    char msg[] = "Test Message";
    // sock->send(msg, sizeof(msg));
    memset(data, 0, sizeof(data));
    while ((len = fread(data, 1, 1024, fp)) > 0) {
        sock->send(data, len);
        memset(data, 0, sizeof(data));
    }
    std::this_thread::sleep_for(std::chrono::seconds(60));
    delete sock;
    fclose(fp);
    return 0;
}