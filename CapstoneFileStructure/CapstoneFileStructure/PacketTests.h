#pragma once
#include "FilePacketA.h"
#include "ChunkRequestPacketA.h"
#include <cassert>
#include <string.h>
#include <iostream>

class PacketTests
{
public:
    PacketTests();
    ~PacketTests();
    void runAll();

    void testCopyToFilePacket();
    void testBaseHeader();
    void testChunkRequestPacket();
    void testFilePacketAccessors();
    void testFilePacketWriteDataToChunk();
    void testToBuffer();

};