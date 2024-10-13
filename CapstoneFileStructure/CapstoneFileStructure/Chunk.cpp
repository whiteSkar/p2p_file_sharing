#include <iostream>
#include "Crypto.h"

#include "Chunk.h"


// Used for creating empty chunk for receiving
// Pre: filePath must be the same path when receiving and sending.
Chunk::Chunk(int chunkSize, uint32_t chunkNum, std::string filePath, std::string hash)
{
    _chunkNum = chunkNum;
    _filePath = filePath;
    _isComplete = false;
    this->hash = hash;

    _chunkData.resize(chunkSize);

    initializePackets(chunkSize, false);
}

// Used for creating chunks from disk data for sending
// Pre: filePath must be the same path when receiving and sending.
Chunk::Chunk(std::string data, uint32_t chunkNum, std::string filePath, std::string hash)
{
    _chunkData = data;
    _chunkNum = chunkNum;
    _filePath = filePath;
    _isComplete = Crypto::computeSHA1(data.data(), data.size()) == hash;
    this->hash = hash;

    initializePackets(_chunkData.size(), true);
}

std::shared_ptr<UdpPacket> Chunk::getPacket(uint32_t packetNum)
{
    if (packetNum < 0 || packetNum >= filePackets.size())
        return nullptr;

    return filePackets[packetNum];
}

void Chunk::putPacket(FilePacket &packet)
{
    if (!_isComplete)
    {
        int packetNum = packet.getPacketNumber();
        if (packetNum < 0 || (unsigned int)packetNum >= filePackets.size())
            throw std::runtime_error("packetNum is out of range.");

        auto chunkPacket = filePackets[packetNum];
        if (chunkPacket->hasFileData())
        {
            return;
        }

        chunkPacket->setFileData(packet.getFileData());

        if (packetNum == filePackets.size() - 1)
        {
            _chunkData.clear();
            for (unsigned int i = 0; i < filePackets.size(); i++)
                _chunkData += filePackets[i]->getFileData();

            if (Crypto::computeSHA1(_chunkData.data(), _chunkData.size()) != hash) {
                resetChunkData();
            } else {
                _isComplete = true;
            }
        }
    }
}

void Chunk::resetChunkData()
{
    for (auto filePacket : filePackets)
        filePacket->setFileData("");
    _chunkData.clear();
    _isComplete = false;
}

uint32_t Chunk::getChunkNum()
{
    return _chunkNum;
}

int Chunk::getDataSize()
{
    return _chunkData.size();
}

bool Chunk::isComplete()
{
    return _isComplete;
}

std::string Chunk::getData()
{
    return _chunkData;
}

int Chunk::getNumberOfPackets()
{
    return filePackets.size();
}

void Chunk::initializePackets(uint32_t chunkSize, bool isForSending)
{
    uint16_t numPackets = (int)ceil((float)chunkSize / (float)FilePacket::MAX_FILE_DATA_SIZE);

    if (isForSending)
    {
        for (int i = 0; i < numPackets; i++)
        {
            std::string fileData = _chunkData.substr(i * FilePacket::MAX_FILE_DATA_SIZE, FilePacket::MAX_FILE_DATA_SIZE);
            auto packet = std::shared_ptr<FilePacket>(new FilePacket(i, _filePath, _chunkNum, fileData));
            filePackets.push_back(packet);
        }
    }
    else
    {
        for (int i = 0; i < numPackets; i++)
        {
            auto packet = std::shared_ptr<FilePacket>(new FilePacket(i));
            filePackets.push_back(packet);
        }
    }
}