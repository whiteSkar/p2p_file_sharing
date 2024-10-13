#ifndef __ChunkRequestPacket_H
#define __ChunkRequestPacket_H

#include "UdpPacket.h"

class ChunkRequestPacket : public UdpPacket
{
public:
    static const uint16_t MAX_METADATA_NAME_SIZE;
    static const uint16_t MAX_FILE_PATH_SIZE;

    ChunkRequestPacket(UdpPacket &udpPacket);
    ChunkRequestPacket(uint16_t packetNum, std::string metadataName, std::string filePath, uint32_t chunkNum);

    std::string getMetadataName();
    std::string getFilePath();
    uint32_t getChunkNumber();

private:
    static const std::string DELIM;

    void parsePacketData();

    std::string metadataName;
    std::string filePath;
    uint32_t chunkNumber;
};

#endif
