CC = g++
OUT_DIR = build
CFLAGS = -c -std=c++17 -O2 -g -Wall
LIBFLAGS = -pthread
INC = -I src/includes
.PHONY: test clean
all : dir build/client.o build/server.o


build/server.o : build/server_test.o build/UWUP.o build/PortHandler.o build/Packet.o
		$(CC) $(LIBFLAGS) build/server_test.o build/UWUP.o build/PortHandler.o build/Packet.o -o build/server_test

build/client.o : build/client_test.o build/UWUP.o build/PortHandler.o build/Packet.o
		$(CC) $(LIBFLAGS) build/client_test.o build/UWUP.o build/PortHandler.o build/Packet.o -o build/client_test

build/server_test.o : server_test.cpp
			$(CC) $(CFLAGS) server_test.cpp -o build/server_test.o

build/client_test.o : client_test.cpp
			$(CC) $(CFLAGS) client_test.cpp -o build/client_test.o

build/UWUP.o : protocol/UWUP.cpp
			$(CC) $(CFLAGS) protocol/UWUP.cpp -o $@

build/PortHandler.o : protocol/PortHandler.cpp
			$(CC) $(CFLAGS) protocol/PortHandler.cpp -o $@

build/Packet.o : protocol/Packet.cpp
			$(CC) $(CFLAGS) protocol/Packet.cpp -o $@

dir:
			@mkdir -p $(OUT_DIR)

test:
			./scripts/test

clean : 
			rm -rf $(OUT_DIR)
			@mkdir -p $(OUT_DIR)
			
