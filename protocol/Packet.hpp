#include <cstdint>

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
 * A class to handle packet structsa, etc.
 */
class Packet {
    /// packed struct data
    PacketStruct packet_struct;

  public:
};