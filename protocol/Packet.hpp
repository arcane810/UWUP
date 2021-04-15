#include <cstdint>
#include <iostream>

const int MAX_PACKET_SIZE = 1024;

/**
 * A structure to store the packed structure
 */
struct PacketStruct {
    /// ACK Number ( 4 Bytes )
    uint32_t ack_number;
    /// Seq Number ( 4 Bytes )
    uint32_t seq_number;
    /// Flags ( 1 Byte )
    uint8_t flags;
    /// Advertised Receive Window ( 4 Bytes )
    uint32_t rwnd;
    /// Variable Size data array
    char data[];
} __attribute__((packed));

/**
 * Flag mask values
 */

const uint32_t SYN = 1;
const uint32_t ACK = 2;
const uint32_t FIN = 4;

enum status { ACKED, NOT_ACKED, NOT_SENT, INACTIVE, RECIEVED };

/**
 * A class to handle packet structsa, etc.
 */
class Packet {
  public:
    /// packed struct data
    PacketStruct *packet_struct;
    /// Total length of packet (length of data + length of header)
    int packet_length;
    /// ACK Number ( 4 Bytes )
    uint32_t ack_number;
    /// Seq Number ( 4 Bytes )
    uint32_t seq_number;
    /// Flags ( 1 Byte )
    uint8_t flags;
    /// Advertised Receive Window ( 4 Bytes )
    uint32_t rwnd;
    /// Data recvd
    // char *data;
    /// Length of data
    uint32_t data_len;
    /// Status of Packed
    uint32_t status = INACTIVE;

    Packet(const char *buffer, int len);
    Packet(uint32_t ack_number, uint32_t seq_number, uint32_t flags,
           uint32_t rwnd, char *buffer, int len);
    // ~Packet();
    std::string getFlagStr(uint32_t flag);
    friend std::ostream &operator<<(std::ostream &os, Packet const &p);
};