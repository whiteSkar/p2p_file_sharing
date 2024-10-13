#pragma once
#include "UdpPacketA.h"

//this class should never be created only extended appon
class PacketWithChunk :
    public UdpPacketA
{
public:
    typedef uint16_t fileNum_t;
    typedef uint32_t chunkNum_t;

    ~PacketWithChunk();
    fileNum_t getFileNum();
    chunkNum_t getChunkNum();
protected:
    static const int BASE_HEADER_SIZE;
    PacketWithChunk(UdpPacketA &packet);
    PacketWithChunk(PacketType packetType, dataDistId_t dataDistId, fileNum_t fileNum, chunkNum_t chunkNum, uint32_t dataSize);
private:
    static const int FILE_NUM_SIZE = sizeof(fileNum_t);
    static const int CHUNK_NUM_SIZE = sizeof(chunkNum_t);
    static const int ADDITIONAL_HEADER_SIZE = FILE_NUM_SIZE + CHUNK_NUM_SIZE;
    void setFileNum(fileNum_t fileNum);
    void setChunkNum(chunkNum_t chunkNum);
};

