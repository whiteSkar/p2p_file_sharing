#include "ChunkRequestPacket.h"

const std::string ChunkRequestPacket::DELIM = ",";

// TODO: Constant organization task 
const uint16_t ChunkRequestPacket::MAX_METADATA_NAME_SIZE = 50;
const uint16_t ChunkRequestPacket::MAX_FILE_PATH_SIZE = 150;    // This is just much easier to implement without violating OOP principles

ChunkRequestPacket::ChunkRequestPacket(UdpPacket &udpPacket)
{
    packetHeader = PacketHeader(udpPacket.getPacketNumber(), PacketHeader::PacketType::ChunkRequest);
    packetData = udpPacket.getPacketData();
    parsePacketData();
}

ChunkRequestPacket::ChunkRequestPacket(uint16_t packetNum, std::string metadataName, std::string filePath, uint32_t chunkNum)
{
    if (metadataName.size() > MAX_METADATA_NAME_SIZE)
        throw std::invalid_argument("metadataName is too long. It must be <= " + MAX_METADATA_NAME_SIZE);
    if (filePath.size() > MAX_FILE_PATH_SIZE)
        throw std::invalid_argument("filePath is too long. It must be <= " + MAX_FILE_PATH_SIZE);
    packetHeader = PacketHeader(packetNum, PacketHeader::PacketType::ChunkRequest);
    this->metadataName = metadataName;
    this->filePath = filePath;
    this->chunkNumber = chunkNum;

    packetData = metadataName + DELIM + filePath + DELIM + std::to_string(chunkNumber);
}

std::string ChunkRequestPacket::getMetadataName()
{
    return metadataName;
}

std::string ChunkRequestPacket::getFilePath()
{
    return filePath;
}

uint32_t ChunkRequestPacket::getChunkNumber()
{
    return chunkNumber;
}

void ChunkRequestPacket::parsePacketData()
{
    auto indexOfDelimiter1 = packetData.find_first_of(DELIM);
    if (indexOfDelimiter1 == std::string::npos)
        throw std::runtime_error("Metadata name is not set");

    metadataName = packetData.substr(0, indexOfDelimiter1);

    auto indexOfDelimiter2 = packetData.find_first_of(DELIM, indexOfDelimiter1 + 1);
    if (indexOfDelimiter2 == std::string::npos)
        throw std::runtime_error("File Path is not set");

    filePath = packetData.substr(indexOfDelimiter1 + 1, indexOfDelimiter2 - (indexOfDelimiter1 + 1));

    chunkNumber = std::stoul(packetData.substr(indexOfDelimiter2 + 1));
}