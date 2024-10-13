#ifndef __UdpPacket_H
#define __UdpPacket_H

#include <boost/asio.hpp>
#include "PacketHeader.h"

#pragma once

class UdpPacket
{
public:
    static const int MAX_SIZE = 1024; // bytes
    static const int MAX_DATA_SIZE = MAX_SIZE - PacketHeader::HEADER_SIZE;

    UdpPacket();
    UdpPacket(unsigned char* data, uint32_t dataSize);

    std::string serializePacket();

    void setPacketData(std::string data);

    uint16_t getPacketNumber();
    PacketHeader::PacketType getPacketType();
    std::string getPacketData();

protected:
    PacketHeader packetHeader;
    std::string packetData;
};

#endif
