#include <thread>
#include <numeric>
#include <functional>
#include <typeinfo>
#include <queue>
#include <unordered_set>
#include <iostream>

#include "DataDistPeer.h"
#include "UDPClient.h"
#include "UDPListener.h"
#include "Util.h"
#include "MetadataHandler.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>

#pragma once

class Peer
{
public:
    static const std::chrono::seconds CHUNK_REQUEST_INTERVAL, 
                                      KEEPALIVE_INTERVAL, 
                                      UPDATE_PEER_LIST_INTERVAL,
                                      CHECK_DL_STATUS_INTERVAL;
    static const std::chrono::milliseconds REQUEST_RATE_INTERVAL;
    static const std::string META_FILTER;
    static const std::string TRACKER_HOST;
    static const int TRACKER_PORT;

    static std::unique_ptr<Peer> spawnTestPeer(uint16_t port, 
        std::string outputDirectory, std::shared_ptr<Metadata> metadata, 
        std::string trackerHost = TRACKER_HOST, int trackerPort = TRACKER_PORT);

    Peer(uint16_t port, std::string outputDirectory, 
         std::string trackerHost = TRACKER_HOST, int trackerPort = TRACKER_PORT);

    ~Peer();
    void destruct();

    void run();

    void addDatadistToDownload(std::shared_ptr<Metadata> metadata);

    // ONLY FOR TESTING
    std::unordered_map<std::string, std::shared_ptr<Metadata>> getActiveMetadata();

private:
    boost::asio::io_service ioService;
    boost::asio::ip::udp::socket socket;
    std::mutex socketMutex;
    UDPClient client;
    UDPListener server;
    std::string outputDirectory;

    uint16_t port;
    bool isDestructing;

    std::string trackerHost;
    int trackerPort;

    std::unordered_map<std::string, std::unordered_map<uint32_t, bool>> chunksToReceive;
    std::mutex chunksToReceiveMutex;

    std::unordered_map<std::string, int> dataDistProgress;

    std::thread serverThread;
    std::thread requestingChunkThread;
    std::vector<std::thread> periodicallyKeepAliveThreads;
    std::thread startNextDatadistThread;

    std::unordered_map<std::string, DataDistPeer> seeders;
    std::unordered_map<std::string, DataDistPeer> leechers;
    std::mutex peerListMutex;

    std::shared_ptr<Metadata> curMetadata;
    std::queue<std::shared_ptr<Metadata>> metadataQueue;

    // active are the ones that are either complete or is in downloading.
    std::unordered_map<std::string, std::shared_ptr<Metadata>> activeMetadata;
    std::mutex activeMetadataMutex;

    void loadAllMetadata();
    int getDatadistProgress();

    std::shared_ptr<std::unordered_set<std::string>> getCompleteFiles(
        std::shared_ptr<Metadata> metadata);

    void periodicallyKeepAlive(std::string dataDistId, const std::string & host, uint16_t port);

    bool isDownloadingDone();
    void periodicallyStartNextDatadist();

    void sendProgress(std::string datadistId, int progress);
    void registerAndQueueDataDists();

    void findChunksToReceive();
    void periodicallyRequestChunks();
    void requestChunks(std::string metafileName);
};

