#pragma once
#include "PacketWithChunk.h"
class ChunkRequestPacketA :
    public PacketWithChunk
{
public:
    ChunkRequestPacketA(UdpPacketA &packet);
    ChunkRequestPacketA(dataDistId_t dataDistId, fileNum_t fileNum, chunkNum_t chunkNum);
    ~ChunkRequestPacketA();
};

