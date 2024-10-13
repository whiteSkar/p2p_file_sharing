#include "PeerProgressPacket.h"

PeerProgressPacket::PeerProgressPacket(UdpPacket &udpPacket)
{
    packetHeader = PacketHeader(udpPacket.getPacketNumber(), PacketHeader::PacketType::PeerProgress);
    packetData = udpPacket.getPacketData();
    parsePacketData();
}


PeerProgressPacket::PeerProgressPacket(std::string dataDistId, unsigned char progress, std::string localHost, unsigned short localPort) :
    dataDistId(dataDistId), progress(progress), localHost(localHost), localPort(localPort)
{
    packetHeader = PacketHeader(PacketHeader::PacketType::PeerProgress);
    unsigned char length = dataDistId.length();
    unsigned char addressLength = localHost.length();
    packetData = "";
    packetData.push_back(length);
    packetData += dataDistId;
    packetData.push_back(progress);
    packetData.push_back(addressLength);
    packetData += localHost;
    std::string port = std::to_string(localPort);
    packetData.push_back(port.length());
    packetData += port;
}

void PeerProgressPacket::parsePacketData() {
    int dataDistIdLength = packetData[0];
    int progressOffset = 1;
    int localHostLengthOffset = 2;
    dataDistId = packetData.substr(LENGTH_OFFSET, dataDistIdLength);
    progress = packetData[progressOffset + dataDistIdLength];
    int localAddressLength = packetData[localHostLengthOffset + dataDistIdLength];
    localHost = packetData.substr(3 + dataDistIdLength, localAddressLength);
    int localPortOffset = localHostLengthOffset + dataDistIdLength + localAddressLength + 1;
    int localPortLength = packetData[localPortOffset];
    localPort = std::stoi(packetData.substr(localPortOffset + 1, localPortLength));
}

unsigned char PeerProgressPacket::getProgress() {
    return progress;
}

std::string  PeerProgressPacket::getDataDistId() {
    return dataDistId;
}

std::string  PeerProgressPacket::getLocalAddress() {
    return localHost;
}

unsigned short PeerProgressPacket::getLocalPort() {
    return localPort;
}
