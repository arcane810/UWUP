#include "Packet.hpp"
#include <cstring>
#include <stdlib.h>

Packet::Packet(char *buffer, int len) {
    packet_struct = (PacketStruct *)malloc(len * sizeof(char));
    memcpy(packet_struct, buffer, len * sizeof(char));
    // TODO unpack header
}