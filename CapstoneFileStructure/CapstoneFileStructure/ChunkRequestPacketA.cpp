#include "ChunkRequestPacketA.h"

ChunkRequestPacketA::ChunkRequestPacketA(UdpPacketA & packet)
    :PacketWithChunk(packet)
{
}

ChunkRequestPacketA::ChunkRequestPacketA(dataDistId_t dataDistId, fileNum_t fileNum, chunkNum_t chunkNum)
    : PacketWithChunk(PacketType::ChunkRequest, dataDistId, fileNum, chunkNum, 0)
{
}

ChunkRequestPacketA::~ChunkRequestPacketA()
{
}