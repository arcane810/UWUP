#include "Packet.hpp"
#include <cstring>
#include <stdlib.h>

Packet::Packet(char *buffer, int len) {
    packet_struct = (PacketStruct *)malloc(len * sizeof(char));
    memcpy(packet_struct, buffer, len * sizeof(char));
    // packet_struct = (PacketStruct *)buffer;
    ack_number = packet_struct->ack_number;
    seq_number = packet_struct->seq_number;
    flags = packet_struct->flags;
    rwnd = packet_struct->rwnd;
    // should I malloc this?
    // data = (char *)malloc(len * sizeof(char) - 13);
    // memcpy(data, packet_struct->data,len * sizeof(char) - 13);
}

Packet::Packet(uint32_t ack_number, uint32_t seq_number, uint32_t flags, uint32_t rwnd, char* buffer, int len) : ack_number(ack_number), seq_number(seq_number), flags(flags), rwnd(rwnd) {
    packet_struct = (PacketStruct *) malloc(13 + sizeof(char) * len);
    packet_struct->ack_number = ack_number;
    packet_struct->seq_number = seq_number;
    packet_struct->flags = flags;
    packet_struct->rwnd = rwnd;
    memcpy(packet_struct->data, buffer, len * sizeof(char));
    // data = (char *)malloc(len * sizeof(char) - 13);
    // memcpy(data, packet_struct->data,len * sizeof(char) - 13);
}


std::ostream& operator<<(std::ostream& os, Packet const& p)
{
    os << "{\n" << p.ack_number << ' ' << p.seq_number <<  ' ' << p.rwnd << '\n' << p.packet_struct->data << "\n}" << std::endl;
    return os;
}
