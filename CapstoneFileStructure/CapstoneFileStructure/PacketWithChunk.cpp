#include "PacketWithChunk.h"

const int PacketWithChunk::BASE_HEADER_SIZE = UdpPacketA::BASE_HEADER_SIZE + ADDITIONAL_HEADER_SIZE;

PacketWithChunk::~PacketWithChunk()
{
}

PacketWithChunk::fileNum_t PacketWithChunk::getFileNum()
{
    return getFromData<fileNum_t>(UdpPacketA::BASE_HEADER_SIZE);
}

PacketWithChunk::chunkNum_t PacketWithChunk::getChunkNum()
{
    return getFromData<chunkNum_t>(UdpPacketA::BASE_HEADER_SIZE + FILE_NUM_SIZE);
}

PacketWithChunk::PacketWithChunk(UdpPacketA & packet)
    :UdpPacketA(packet)
{
}

PacketWithChunk::PacketWithChunk(PacketType packetType, dataDistId_t dataDistId, fileNum_t fileNum, chunkNum_t chunkNum, uint32_t dataSize)
    : UdpPacketA(packetType, dataDistId, dataSize + ADDITIONAL_HEADER_SIZE)
{
    this->setFileNum(fileNum);
    this->setChunkNum(chunkNum);
}

void PacketWithChunk::setFileNum(fileNum_t fileNum)
{
    setInData(fileNum, UdpPacketA::BASE_HEADER_SIZE);
}

void PacketWithChunk::setChunkNum(chunkNum_t chunkNum)
{
    setInData(chunkNum, UdpPacketA::BASE_HEADER_SIZE + FILE_NUM_SIZE);
}
