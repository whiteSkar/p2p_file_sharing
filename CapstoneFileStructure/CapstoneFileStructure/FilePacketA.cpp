#include "FilePacketA.h"

const int FilePacketA::MAX_DATA_SIZE = UdpPacketA::MAX_DATA_SIZE - FilePacketA::ADDITIONAL_HEADER_SIZE;

FilePacketA::FilePacketA(UdpPacketA & packet)
    :PacketWithChunk(packet)
{
}

FilePacketA::FilePacketA(dataDistId_t dataDistId, fileNum_t fileNum, chunkNum_t chunkNum, packetNum_t packetNum, unsigned char * data, uint32_t dataSize)
    : PacketWithChunk(UdpPacketA::PacketType::FilePacket, dataDistId, fileNum, chunkNum, dataSize + ADDITIONAL_HEADER_SIZE)
{
    this->setPacketNum(packetNum);
    this->copyData(data, dataSize);
}

FilePacketA::~FilePacketA()
{
}

FilePacketA::packetNum_t FilePacketA::getPacketNum()
{
    return getFromData<packetNum_t>(BASE_HEADER_SIZE);
}

uint32_t FilePacketA::getFileDataSize()
{
    return getSize() - BASE_HEADER_SIZE - ADDITIONAL_HEADER_SIZE;
}

//pre: Chunk must have enough memory allocated
void FilePacketA::writeToChunk(unsigned char * locationInChunk)
{
    unsigned char* pointer = _data.get() + BASE_HEADER_SIZE + ADDITIONAL_HEADER_SIZE;
    uint32_t actualDataSize = getFileDataSize();
    for (int i = 0; i < actualDataSize; i++)
    {
        locationInChunk[i] = pointer[i];
    }
}


//Private:
void FilePacketA::setPacketNum(packetNum_t packetNum)
{
    setInData(packetNum, BASE_HEADER_SIZE);
}

// copies from data to _data;
void FilePacketA::copyData(unsigned char* data, uint32_t dataSize)
{
    unsigned char* pointer = _data.get() + BASE_HEADER_SIZE + ADDITIONAL_HEADER_SIZE;
    for (int i = 0; i < dataSize; i++)
    {
        pointer[i] = data[i];
    }
}
