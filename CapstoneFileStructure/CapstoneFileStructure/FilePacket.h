#ifndef __FilePacket_H
#define __FilePacket_H

#include "UdpPacket.h"

class FilePacket : public UdpPacket
{
public:
    static const uint16_t MAX_FILE_PATH_SIZE;
    static const uint16_t MAX_FILE_DATA_SIZE;

    FilePacket(UdpPacket &udpPacket);
    FilePacket(uint16_t packetNum);
    FilePacket(uint16_t packetNum, std::string filePath, uint32_t chunkNumber, std::string fileData);

    bool hasFileData();
    std::string getFileData();
    void setFileData(std::string fileData);

    std::string getFilePath();
    uint32_t getChunkNumber();

private:
    static const std::string DELIM;

    std::string filePath;
    uint32_t chunkNumber;

    int fileDataIndex;

    void parsePacketData();
};

#endif