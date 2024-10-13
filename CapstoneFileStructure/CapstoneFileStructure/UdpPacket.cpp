#include "UdpPacket.h"

UdpPacket::UdpPacket()
{
}

UdpPacket::UdpPacket(unsigned char* data, uint32_t dataSize)
    : packetHeader(PacketHeader(data, dataSize))
{
    packetData = std::string((char *)data, dataSize);
    packetData = packetData.substr(PacketHeader::HEADER_SIZE);
}

std::string UdpPacket::serializePacket() {
    return packetHeader.serializeHeader() + packetData;
}

void UdpPacket::setPacketData(std::string data)
{
    this->packetData = data;
}


uint16_t UdpPacket::getPacketNumber()
{
    return packetHeader.getPacketNum();
}

PacketHeader::PacketType UdpPacket::getPacketType()
{
    return packetHeader.getType();
}

std::string UdpPacket::getPacketData()
{
    return packetData;
}
