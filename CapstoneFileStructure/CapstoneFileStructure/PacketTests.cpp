#include "PacketTests.h"



PacketTests::PacketTests()
{
}


PacketTests::~PacketTests()
{
}

void PacketTests::runAll()
{
    testCopyToFilePacket();
    testBaseHeader();
    testFilePacketAccessors();
    testFilePacketWriteDataToChunk();
    testChunkRequestPacket();
    testToBuffer();

    std::cout << "All Packet Tests Passed\n";
}

void PacketTests::testCopyToFilePacket()
{
    UdpPacketA base(UdpPacketA::PacketType::FilePacket, 1000, 4);
    FilePacketA packet(base);
    assert(packet.getPacketType() == UdpPacketA::PacketType::FilePacket);
    assert(packet.getDataDistID() == 1000);
    assert(packet.getSize() == UdpPacketA::BASE_HEADER_SIZE + 4);
}

void PacketTests::testBaseHeader()
{
    UdpPacketA packet(UdpPacketA::PacketType::FilePacket, 89, 0);
    assert(packet.getPacketType() == UdpPacketA::PacketType::FilePacket);
    assert(packet.getDataDistID() == 89);
    assert(packet.getSize() == UdpPacketA::BASE_HEADER_SIZE);
}

void PacketTests::testChunkRequestPacket()
{
    ChunkRequestPacketA packet(123456789, 34345, 984390);
    assert(packet.getDataDistID() == 123456789);
    assert(packet.getFileNum() == 34345);
    assert(packet.getPacketType() == UdpPacketA::PacketType::ChunkRequest);
    assert(packet.getChunkNum() == 984390);
}

void PacketTests::testFilePacketAccessors()
{
    FilePacketA filePacket(1001010, 15032, 1004000, 1000, (unsigned char*)"hello", 5);
    assert(filePacket.getPacketType() == UdpPacketA::PacketType::FilePacket);
    assert(filePacket.getDataDistID() == 1001010);
    assert(filePacket.getFileNum() == 15032);
    assert(filePacket.getChunkNum() == 1004000);
    assert(filePacket.getPacketNum() == 1000);
    assert(filePacket.getFileDataSize() == 5);
    assert(filePacket.getSize() == boost::asio::buffer_size(filePacket.getBuffer()));
}

void PacketTests::testFilePacketWriteDataToChunk()
{
    FilePacketA filePacket(1001010, 15032, 1004000, 10000, (unsigned char*)"hello", 5);
    char s[6];
    char i = '0';
    for (; i <= '4'; i++)
    {
        s[i - '0'] = i;
    }
    s[i - '0'] = '\0';
    filePacket.writeToChunk((unsigned char*)s);
    assert(strcmp(s, "hello") == 0);
}

void PacketTests::testToBuffer()
{
    FilePacketA fp(1, 2, 3, 4, (unsigned char*)"1234567890", 10);
    auto buffer = fp.getBuffer();

    UdpPacketA packet(buffer, boost::asio::buffer_size(buffer));
    assert(packet.getPacketType() == UdpPacketA::PacketType::FilePacket);
    FilePacketA fp2 = packet;

    assert(fp.getDataDistID() == fp2.getDataDistID());
    assert(fp.getFileNum() == fp2.getFileNum());
    assert(fp.getChunkNum() == fp2.getChunkNum());
    assert(fp.getPacketType() == fp2.getPacketType());
    assert(fp.getFileDataSize() == fp2.getFileDataSize());
}
