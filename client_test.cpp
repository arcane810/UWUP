#include "./protocol/UWUP.hpp"
#include <thread>

#include <cstring>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    std::string save_location = "downloaded_file";
    if (argc > 1) {
        save_location = std::string(argv[1]);
    }
    try {
        UWUPSocket socket;
        socket.set_options(UWUPSocket::SET_KEEP_ALIVE, true);
        socket.connect("127.0.0.1", 8080);
        int i = 50;
        char data[20480];
        memset(data, 0, sizeof(data));
        FILE *fp = fopen(save_location.c_str(), "wb+");
        int len;
        while ((len = socket.recv(data, 20480)) > 0) {
            // std::cout << data << std::endl;
            fwrite(data, len, 1, fp);
            fflush(fp);
            memset(data, 0, sizeof(data));
        }
        std::cout << "END REACHED" << std::endl;
        fclose(fp);
    } catch (std::exception e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}