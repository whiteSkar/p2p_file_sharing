#pragma once
#include "PacketWithChunk.h"
class FilePacketA :
    public PacketWithChunk
{
public:
    typedef uint16_t packetNum_t;
    static const int MAX_DATA_SIZE;

    FilePacketA(UdpPacketA &packet);
    FilePacketA(dataDistId_t dataDistId, fileNum_t fileNum, chunkNum_t chunkNum, packetNum_t packetNum, unsigned char* data, uint32_t dataSize);
    ~FilePacketA();
    packetNum_t getPacketNum();
    uint32_t getFileDataSize();
    void writeToChunk(unsigned char* locationInChunk); //does no checking for chunk (do checking of chunk before calling)

private:
    static const int ADDITIONAL_HEADER_SIZE = sizeof(packetNum_t);
    void setPacketNum(packetNum_t packetNum);
    void copyData(unsigned char* data, uint32_t dataSize);
};

