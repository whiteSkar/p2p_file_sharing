#include "FilePacket.h"

const std::string FilePacket::DELIM = ",";

const uint16_t FilePacket::MAX_FILE_PATH_SIZE = 150;    // This is just much easier to implement without violating OOP principles
const uint16_t FilePacket::MAX_FILE_DATA_SIZE = FilePacket::MAX_DATA_SIZE - 
                                                FilePacket::MAX_FILE_PATH_SIZE - 
                                                std::to_string(std::numeric_limits<uint32_t>::max()).size();

FilePacket::FilePacket(UdpPacket &udpPacket) : FilePacket(udpPacket.getPacketNumber())
{
    packetData = udpPacket.getPacketData();
    parsePacketData();
}

FilePacket::FilePacket(uint16_t packetNum)
{
    packetHeader = PacketHeader(packetNum, PacketHeader::PacketType::FilePacket);
    fileDataIndex = 0;
}

FilePacket::FilePacket(uint16_t packetNum, std::string filePath, uint32_t chunkNumber, std::string fileData)
{
    if (filePath.size() > MAX_FILE_PATH_SIZE)
        throw std::invalid_argument("filePath is too long. It must be <= " + MAX_FILE_PATH_SIZE);
    if (fileData.size() > MAX_FILE_DATA_SIZE)
        throw std::invalid_argument("fileData is too long. It must be <= " + MAX_FILE_DATA_SIZE);

    packetHeader = PacketHeader(packetNum, PacketHeader::PacketType::FilePacket);
    this->filePath = filePath;
    this->chunkNumber = chunkNumber;

    std::string fileDataMetadata = filePath + DELIM + std::to_string(chunkNumber) + DELIM;
    packetData = fileDataMetadata + fileData;
    fileDataIndex = fileDataMetadata.size();
}

bool FilePacket::hasFileData()
{
    return packetData.size() > (unsigned int)fileDataIndex;
}

std::string FilePacket::getFileData()
{
    return packetData.substr(fileDataIndex);
}

void FilePacket::setFileData(std::string fileData)
{
    packetData = packetData.substr(0, fileDataIndex) + fileData;
}

std::string FilePacket::getFilePath()
{
    return filePath;
}

uint32_t FilePacket::getChunkNumber()
{
    return chunkNumber;
}


/* Private Methods */

void FilePacket::parsePacketData()
{
    auto indexOfDelimiter1 = packetData.find_first_of(DELIM);
    if (indexOfDelimiter1 == std::string::npos)
        throw std::runtime_error("File path is not set");

    filePath = packetData.substr(0, indexOfDelimiter1);

    auto indexOfDelimiter2 = packetData.find_first_of(DELIM, indexOfDelimiter1 + 1);
    if (indexOfDelimiter2 == std::string::npos)
        throw std::runtime_error("Chunk number is not set");

    chunkNumber = std::stoul(packetData.substr(indexOfDelimiter1 + 1, indexOfDelimiter2 - (indexOfDelimiter1 + 1)));

    fileDataIndex = indexOfDelimiter2 + 1;
}