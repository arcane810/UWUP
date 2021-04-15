#include "./protocol/UWUP.hpp"
#include <thread>

#include <cstring>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {

    UWUPSocket socket;
    socket.connect("127.0.0.1", 8080);
    int i = 50;
    char data[2048];
    memset(data, 0, sizeof(data));
    FILE *fp = fopen("out.txt", "wb+");
    int len;
    while ((len = socket.recv(data, 2048)) > 0) {
        std::cout << data << std::endl;
        fwrite(data, len, 1, fp);
        fflush(fp);
        memset(data, 0, sizeof(data));
    }
    fclose(fp);
    return 0;
}