#include <vector>
#include <boost\shared_array.hpp>

#include "FilePacket.h"
#include "UdpPacket.h"

#pragma once

class Chunk
{
public:
    Chunk(int dataSize, uint32_t chunkNum, std::string filePath, std::string hash);
    Chunk(std::string data, uint32_t chunkNum, std::string filePath, std::string hash);

    std::shared_ptr<UdpPacket> getPacket(uint32_t packetNum);
    void putPacket(FilePacket &packet);

    int getNumberOfPackets();

    uint32_t getChunkNum();
    int getDataSize();
    bool isComplete();
    std::string getData();

private:
    std::vector<std::shared_ptr<FilePacket>> filePackets;

    std::string _filePath;
    std::string hash;

    // The position of the chunk in a file
    uint32_t _chunkNum;

    // The number of bytes in data
    uint32_t _chunkSize;

    bool _isComplete;

    std::string _chunkData;

    void resetChunkData();

    void initializePackets(uint32_t chunkSize, bool isForSending);
};
