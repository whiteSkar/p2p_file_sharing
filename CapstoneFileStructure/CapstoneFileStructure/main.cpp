#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cassert>

#include "DirectoryHandler.h"
#include "FileHandler.h"
#include "FilePacket.h"
#include "Peer.h"
#include "DataDistServer.h"
#include "PeerListPacket.h"

#define DEFAULT_CHUNK_SIZE 51200

using namespace std;

void testCore();
void testChunks();
void testMetadata();
void testDataDistServer();
void testPeerRegistrationPacket();
void testPacketSerialization();
void testAddSeederLeecherToDataDistServer();
void testPeerListPacket();
void testLoadActiveMeta();

void testMultiDataSeed();
void testIncompleteDataDist();
void testDownloadQueue();
void testNAT(); // different NAT. need two peers on different NAT running this function

void createDataDistServer();


int main()
{
    //testChunks();
    //testPeerRegistrationPacket();
    //testAddSeederLeecherToDataDistServer();
    //testDataDistServer();
    //testPeerListPacket();
    //testMetadata();
    //testPacketSerialization();

    //testMultiDataSeed();
    //testIncompleteDataDist();
    //testDownloadQueue();
    //testNAT();
    testCore();

    //testLoadActiveMeta();
    //createDataDistServer();
    std::cout << "TEST COMPLETE. PRESS ANYTHING TO EXIT" << std::endl;
    char c;
    cin >> c;
}

void createDataDistServer()
{
    DataDistServer server(20020, "dataDistDirectory");
    server.run();
}

void testPeerListPacket() {
    std::vector<DataDistPeer> peers;
    peers.emplace_back("123.123.123", 1231, "192.168.0.1", 0);
    peers.emplace_back("222.333.444", 1111, "192.168.0.2", 0);
    peers.emplace_back("333.333.333", 3333, "192.168.0.3", 0);
    PeerListPacket peerListPacket("127.0.0.1:3030", peers, true);

    auto serializedPacket = peerListPacket.serializePacket();
    UdpPacket udpPacket((unsigned char *) serializedPacket.data(), serializedPacket.size());
    PeerListPacket parsedPacket(udpPacket);
    assert(parsedPacket.getPeers().size() == peerListPacket.getPeers().size() + 1);
}

void testAddSeederLeecherToDataDistServer() {
    DataDistServer server(30003, "dataDistDirectory");
    std::thread t2(&DataDistServer::run, &server);
    //std::thread t1(&DataDistServer::run, server);
    server.addDataDist("test");

    boost::asio::io_service ioService;
    boost::asio::ip::udp::socket socket(ioService, udp::endpoint(udp::v4(), 40000));
    std::mutex socketMutex;
    UDPClient client(&socket, &socketMutex);

    client.sendProgress("test", 50, "127.0.0.1", 30003, 3000);
    client.sendProgress("test", 100, "127.0.0.1", 30003, 3000);

    server.destruct();
    t2.join();
    /*std:: cout << server. << std::endl;*/
}

// Mostly to test how memory works in C++, not doing much right now
void testDataDistServer() {
    DataDistServer server(30003, "dataDistDirectory");
    server.addDataDist("hello");
    for (int i = 0; i < 30; i++) {
        server.updateSeeder("hello", DataDistPeer("sdfsdfsdf", i, "", 0));
    }

    for (int i = 0; i < 30; i++) {
        server.fetchLeecherList("hello");
        server.fetchSeederList("hello");
    }
}

void testPeerRegistrationPacket() {
    PeerProgressPacket packet("this is a test", 98, "192.168.0.1", 12312);
    assert(packet.getDataDistId() == "this is a test");
    assert(packet.getProgress() == 98);
    assert(packet.getLocalAddress() == "192.168.0.1");
    assert(packet.getLocalPort() == 12312);
    UdpPacket udpPacket(packet);
    PeerProgressPacket newPacket(udpPacket);
    assert(newPacket.getDataDistId() == packet.getDataDistId());
    assert(newPacket.getProgress() == packet.getProgress());
    assert(newPacket.getLocalAddress() == packet.getLocalAddress());
    assert(newPacket.getLocalPort() == packet.getLocalPort());
}

void testMetadata()
{
    MetadataHandler::createMetadataFile("TestingResources/MetadataTest/testGame",
                                        DEFAULT_CHUNK_SIZE);
    try
    {
        auto metadata = MetadataHandler::getMetadata("TestingResources/MetadataTest/testGame.meta");
        cout << metadata->getChunkSize() << endl;

        for (unsigned int i = 0; i < metadata->getFileCount(); i++)
        {
            auto filePath = metadata->getFilePath(i);
            cout << filePath << " " << metadata->getFileSize(filePath) << endl;

            for (int j = 0; j < metadata->getChunkCountForFile(filePath); j++)
                cout << metadata->getHash(filePath, j) << endl;
        }
    }
    catch (exception e)
    {
        cout << "FAIL: " << e.what() << std::endl;
    }
}

void testChunks()
{/*
    string testFilename = "TestChunks/test.jpg";
    string testChunkOutputFilename = "TestChunks/testChunks.jpg";

    if (boost::filesystem::exists(testChunkOutputFilename))
        boost::filesystem::remove(testChunkOutputFilename);

    FileHandler fh1(testFilename, testFilename, 4096, 0);
    FileHandler fh2(testChunkOutputFilename, testChunkOutputFilename, fh1.getChunkSize(), fh1.getFileSize());
    assert(fh1.getFileSize() == fh2.getFileSize());

    cout << fh2.getNumberOfChunks() << endl;
    for (int i = 0; i < fh2.getNumberOfChunks(); i++)
    {
        auto c1 = fh1.getChunk(i);
        fh2.writeReceivedChunk(c1);
    }
    */
}

void testPacketSerialization()
{
    std::string c = "hello";
    int packetNum = 4;
    FilePacket packet(packetNum);
    auto data = packet.serializePacket();

    UdpPacket packet2((unsigned char *)data.data(), data.size());
    assert(packet.getPacketType() == packet2.getPacketType());
    assert(packet.getPacketNumber() == packet2.getPacketNumber());

    assert(packet.getPacketNumber() == packetNum);
    assert(packet.getPacketType() == PacketHeader::FilePacket);
}

void testLoadActiveMeta()
{
    std::cout << "Testing LoadActiveMetadata..." << std::endl;

    Peer peer(40011, "TestingResources");
    auto activeMetadata = peer.getActiveMetadata();

    assert(activeMetadata.size() == 1);

    std::cout << "Testing LoadActiveMetadata SUCCEEDED." << std::endl << std::endl;
    peer.destruct();
}

void testMultiDataSeed()
{
    std::string trackerHost = "127.0.0.1";
    int trackerPort = 20020;
    DataDistServer dataDistServer(trackerPort, "dataDistDirectory");
    std::thread dataDistServerThread(&DataDistServer::run, &dataDistServer);

    std::string peer1Dir = "TestingResources/MultiDataSeedTest/ExpectedOutputDirectory1";
    if (boost::filesystem::exists(peer1Dir + "/testGame"))
        boost::filesystem::remove_all(peer1Dir + "/testGame");
    if (boost::filesystem::exists(peer1Dir + "/testGame.completed"))
        boost::filesystem::remove_all(peer1Dir + "/testGame.completed");

    std::string peer2Dir = "TestingResources/MultiDataSeedTest/ExpectedOutputDirectory2";
    if (boost::filesystem::exists(peer2Dir + "/testGame2"))
        boost::filesystem::remove_all(peer2Dir + "/testGame2");
    if (boost::filesystem::exists(peer2Dir + "/testGame2.completed"))
        boost::filesystem::remove_all(peer2Dir + "/testGame2.completed");

    uint16_t port1 = 30001;
    std::thread t1(&Peer::spawnTestPeer, port1, peer1Dir, nullptr, trackerHost, trackerPort);
    std::string str = "Initialized testing peer on port " + std::to_string(port1) + "\n";
    std::cout << str;

    uint16_t port2 = 30002;
    std::thread t2(&Peer::spawnTestPeer, port2, peer2Dir, nullptr, trackerHost, trackerPort);
    str = "Initialized testing peer on port " + std::to_string(port2) + "\n";
    std::cout << str;

    try
    {
        std::string basePeerDir = "TestingResources/MultiDataSeedTest/BasePeer";
        Peer peer(30000, basePeerDir, trackerHost, trackerPort);

        std::thread basePeerThread(&Peer::run, &peer);

        // type anything to stop base peer
        char c;
        cin >> c;

        peer.destruct();
        if (basePeerThread.joinable()) {
            basePeerThread.join();
        }
    }
    catch (exception e)
    {
        std::cout << "EXCEPTION: " << e.what() << std::endl;
    }

    // Not sure if detaching and exiting main is okay
    // However, these are just for test peers.
    // Production code will not need this. so it should be okay.
    t1.detach();
    t2.detach();

    dataDistServer.destruct();
    if (dataDistServerThread.joinable()) {
        dataDistServerThread.join();
    }
}

void testIncompleteDataDist()
{
    std::string trackerHost = "127.0.0.1";
    int trackerPort = 20020;
    DataDistServer dataDistServer(trackerPort, "dataDistDirectory");
    std::thread dataDistServerThread(&DataDistServer::run, &dataDistServer);

    std::string peer1Dir = "TestingResources/IncompleteDataDistTest/Peer1";
    if (boost::filesystem::exists(peer1Dir + "/testGame/Counter Strike_screenshot.png"))
        boost::filesystem::remove_all(peer1Dir + "/testGame/Counter Strike_screenshot.png");
    if (boost::filesystem::exists(peer1Dir + "/testGame/dota2_fanart01_by_crow_god-d84ek06.jpg"))
        boost::filesystem::remove_all(peer1Dir + "/testGame/dota2_fanart01_by_crow_god-d84ek06.jpg");
    if (boost::filesystem::exists(peer1Dir + "/testGame.completed"))
        boost::filesystem::remove_all(peer1Dir + "/testGame.completed");

    std::string peer2Dir = "TestingResources/IncompleteDataDistTest/Peer2";
    if (boost::filesystem::exists(peer2Dir + "/testGame/z directory"))
        boost::filesystem::remove_all(peer2Dir + "/testGame/z directory");
    if (boost::filesystem::exists(peer2Dir + "/testGame.completed"))
        boost::filesystem::remove_all(peer2Dir + "/testGame.completed");

    uint16_t port1 = 30001;
    std::thread t1(&Peer::spawnTestPeer, port1, peer1Dir, nullptr, trackerHost, trackerPort);
    std::string str = "Initialized testing peer on port " + std::to_string(port1) + "\n";
    std::cout << str;

    uint16_t port2 = 30002;
    std::thread t2(&Peer::spawnTestPeer, port2, peer2Dir, nullptr, trackerHost, trackerPort);
    str = "Initialized testing peer on port " + std::to_string(port2) + "\n";
    std::cout << str;

    // type anything to stop base peer
    char c;
    cin >> c;

    // Not sure if detaching and exiting main is okay
    // However, these are just for test peers.
    // Production code will not need this. so it should be okay.
    t1.detach();
    t2.detach();

    dataDistServer.destruct();
    if (dataDistServerThread.joinable()) {
        dataDistServerThread.join();
    }
}

void testDownloadQueue()
{
    std::string trackerHost = "127.0.0.1";
    int trackerPort = 20020;
    DataDistServer dataDistServer(trackerPort, "dataDistDirectory");
    std::thread dataDistServerThread(&DataDistServer::run, &dataDistServer);

    std::string peer0Dir = "TestingResources/DownloadQueueTest/Peer 0";
    if (boost::filesystem::exists(peer0Dir + "/testGame"))
        boost::filesystem::remove_all(peer0Dir + "/testGame");
    if (boost::filesystem::exists(peer0Dir + "/testGame.completed"))
        boost::filesystem::remove_all(peer0Dir + "/testGame.completed");
    if (boost::filesystem::exists(peer0Dir + "/testGame2"))
        boost::filesystem::remove_all(peer0Dir + "/testGame2");
    if (boost::filesystem::exists(peer0Dir + "/testGame2.completed"))
        boost::filesystem::remove_all(peer0Dir + "/testGame2.completed");

    std::string peer1Dir = "TestingResources/DownloadQueueTest/Peer 1";
    std::string peer2Dir = "TestingResources/DownloadQueueTest/Peer 2";

    uint16_t port1 = 30001;
    std::thread t1(&Peer::spawnTestPeer, port1, peer1Dir, nullptr, trackerHost, trackerPort);
    std::string str = "Initialized testing peer on port " + std::to_string(port1) + "\n";
    std::cout << str;

    uint16_t port2 = 30002;
    std::thread t2(&Peer::spawnTestPeer, port2, peer2Dir, nullptr, trackerHost, trackerPort);
    str = "Initialized testing peer on port " + std::to_string(port2) + "\n";
    std::cout << str;

    try
    {
        Peer peer0(30000, peer0Dir, trackerHost, trackerPort);

        std::thread basePeerThread(&Peer::run, &peer0);

        // type anything to stop base peer
        char c;
        cin >> c;

        peer0.destruct();
        if (basePeerThread.joinable()) {
            basePeerThread.join();
        }
    }
    catch (exception e)
    {
        std::cout << "EXCEPTION: " << e.what() << std::endl;
    }

    // Not sure if detaching and exiting main is okay
    // However, these are just for test peers.
    // Production code will not need this. so it should be okay.
    t1.detach();
    t2.detach();

    dataDistServer.destruct();
    if (dataDistServerThread.joinable()) {
        dataDistServerThread.join();
    }
}

void testNAT()
{
    try
    {
        // Manually delete/add necessary data dists for testing
        // Basic setup should be:
        // Peer 1 has data dist 1 and metadata 2
        // Peer 2 has data dist 2 and metadata 1
        // They should be able to seed/download from each other

        Peer peer(40011, "TestingResources/NATTest/Peer");

        std::thread basePeerThread(&Peer::run, &peer);

        // debugging purpose
        // type anything to stop base peer
        char c;
        cin >> c;

        peer.destruct();
        if (basePeerThread.joinable()) {
            basePeerThread.join();
        }
    }
    catch (exception e)
    {
        std::cout << "EXCEPTION: " << e.what() << std::endl;
    }

    // debugging purpose
    // type anything to exit the program
    char c;
    cin >> c;
}

void testCore()
{
    std::cout << "initialization commencing" << std::endl;

    string metadataFileName = "testGame.meta";
    std::shared_ptr<Metadata> metadataForPeer3 = 
        MetadataHandler::getMetadata("ExpectedOutputDirectory3/" + metadataFileName);

    // For testing with local dataDistServer
    //DataDistServer dataDistServer(20020, "dataDistDirectory");
    //std::thread dataDistServerThread(&DataDistServer::run, &dataDistServer);

    if (boost::filesystem::exists("BasePeer/testGame"))
        boost::filesystem::remove_all("BasePeer/testGame");
    if (boost::filesystem::exists("BasePeer/testGame.completed"))
        boost::filesystem::remove_all("BasePeer/testGame.completed");

    if (boost::filesystem::exists("BasePeer/testGame2"))
        boost::filesystem::remove_all("BasePeer/testGame2");
    if (boost::filesystem::exists("BasePeer/testGame2.completed"))
        boost::filesystem::remove_all("BasePeer/testGame2.completed");

    if (boost::filesystem::exists("ExpectedOutputDirectory3/testGame"))
        boost::filesystem::remove_all("ExpectedOutputDirectory3/testGame");
    if (boost::filesystem::exists("ExpectedOutputDirectory3/testGame.completed"))
        boost::filesystem::remove_all("ExpectedOutputDirectory3/testGame.completed");
    
    int port1 = 20011;
    std::thread t1([&] { Peer::spawnTestPeer(port1, "ExpectedOutputDirectory1", nullptr); });
    std::string str = "Initialized testing peer on port " + std::to_string(port1) + "\n";
    std::cout << str;
    
    int port2 = 20012;
    std::thread t2([&] { Peer::spawnTestPeer(port2, "ExpectedOutputDirectory2", nullptr); });
    str = "Initialized testing peer on port " + std::to_string(port2) + "\n";
    std::cout << str;

    int port3 = 20013;
    std::thread t3([&] { Peer::spawnTestPeer(port3, "ExpectedOutputDirectory3", metadataForPeer3); });
    str = "Initialized testing peer on port " + std::to_string(port3) + "\n";
    std::cout << str;
    
    try
    {
        Peer peer(40011, "BasePeer");

        std::thread basePeerThread(&Peer::run, &peer);

        // debugging purpose
        // type anything to stop base peer
        char c;
        cin >> c;

        peer.destruct();
        if (basePeerThread.joinable()) {
            basePeerThread.join();
        }
    }
    catch (exception e)
    {
        std::cout << "EXCEPTION: " << e.what() << std::endl;
    }

    // Not sure if detaching and exiting main is okay
    // However, these are just for test peers.
    // Production code will not need this. so it should be okay.
    t1.detach();
    t2.detach();
    t3.detach();

    // For testing with local dataDistServer
    //dataDistServer.destruct();
    //if (dataDistServerThread.joinable()) {
    //    dataDistServerThread.join();
    //}

    // debugging purpose
    // type anything to exit the program
    char c;
    cin >> c;
}