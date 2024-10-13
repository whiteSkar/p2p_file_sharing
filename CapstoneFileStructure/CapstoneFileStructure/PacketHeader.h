#include <boost/asio.hpp>
#pragma once

class PacketHeader
{
public:
    typedef unsigned char Type;

    enum PacketType: Type 
    {
        ChunkRequest,
        FilePacket,
        PeerProgress,
        PeerList,
        Unknown
    };

    static const int HEADER_SIZE = sizeof(Type) + sizeof(uint16_t);

    PacketHeader();
    PacketHeader(PacketType packetType);
    PacketHeader(uint16_t packetNum, PacketType packetType);
    PacketHeader(unsigned char* data, uint32_t dataSize);

    std::string serializeHeader();

    PacketType getType();
    uint16_t getPacketNum();

private:
    // in order
    PacketType _type;
    uint16_t _packetNum;    // Used for identifying where this packet belongs in a chunk

    static PacketType getType(Type c);
};

