#include "PacketHeader.h"
#include <iostream>

PacketHeader::PacketHeader()
{

}

PacketHeader::PacketHeader(PacketType packetType)
{
    _type = packetType;
    _packetNum = 0;
}

PacketHeader::PacketHeader(uint16_t packetNum, PacketType packetType)
{
    _type = packetType;
    _packetNum = packetNum;
}

PacketHeader::PacketHeader(unsigned char* data, uint32_t dataSize)
{
    if (dataSize < sizeof(Type) + sizeof(_packetNum))
        throw std::invalid_argument("data size is too small.");

    _type = getType(data[0]);
    data += sizeof(Type);

    _packetNum = ((uint16_t*)data)[0];
}


std::string PacketHeader::serializeHeader() {
    auto packetNumPtr = getPacketNum();
    auto packetNumber = static_cast<char *>(static_cast<void *>(&packetNumPtr));    
    std::string packetType = "";
    packetType += static_cast<Type>(_type);
    std::string packetNum(packetNumber, sizeof(uint16_t));
    return packetType + packetNum;
}

PacketHeader::PacketType PacketHeader::getType()
{
    return _type;
}

uint16_t PacketHeader::getPacketNum()
{
    return _packetNum;
}

//private
PacketHeader::PacketType PacketHeader::getType(Type c)
{
    return static_cast<PacketType>(c);
}