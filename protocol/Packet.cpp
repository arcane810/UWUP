#include "Packet.hpp"
#include <cstring>
#include <stdlib.h>

Packet::Packet(char *buffer, int len) {
    packet_length = len;
    data_len = len - sizeof(PacketStruct);
    packet_struct = (PacketStruct *)malloc(len);
    memcpy(packet_struct, buffer, len);
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
    packet_length = sizeof(PacketStruct)+len;
    data_len = len;
    packet_struct = (PacketStruct *) malloc(13 + len);
    packet_struct->ack_number = ack_number;
    packet_struct->seq_number = seq_number;
    packet_struct->flags = flags;
    packet_struct->rwnd = rwnd;
    memcpy(packet_struct->data, buffer, len);
    // data = (char *)malloc(len * sizeof(char) - 13);
    // memcpy(data, packet_struct->data,len * sizeof(char) - 13);
}


// Packet::~Packet(){
//     delete packet_struct;
// }


std::ostream& operator<<(std::ostream& os, Packet const& p)
{
    os << "{\n" << p.ack_number << ' ' << p.seq_number <<  ' ' << p.rwnd << '\n' << p.packet_struct->data << "\n}" << std::endl;
    return os;
}
