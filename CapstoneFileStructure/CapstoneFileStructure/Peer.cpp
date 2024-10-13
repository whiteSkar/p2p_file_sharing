#include "Peer.h"

const std::chrono::seconds Peer::CHUNK_REQUEST_INTERVAL = std::chrono::seconds(3);
const std::chrono::seconds Peer::KEEPALIVE_INTERVAL = std::chrono::seconds(5);
const std::chrono::seconds Peer::UPDATE_PEER_LIST_INTERVAL = std::chrono::seconds(3);
const std::chrono::seconds Peer::CHECK_DL_STATUS_INTERVAL = std::chrono::seconds(5);
const std::chrono::milliseconds Peer::REQUEST_RATE_INTERVAL = std::chrono::milliseconds(5);

// TODO: Constant organization task 
const std::string Peer::META_FILTER = ".meta";
const std::string Peer::TRACKER_HOST = "40.118.135.71"; // "127.0.0.1"; // for local testing
const int Peer::TRACKER_PORT = 20020;

Peer::Peer(uint16_t port, std::string outputDirectory, 
           std::string trackerHost, int trackerPort)
    : socket(ioService, udp::endpoint(udp::v4(), port)), 
      client(&socket, &socketMutex), 
      server(&socket, &socketMutex, 
             outputDirectory, 
             &activeMetadata, &activeMetadataMutex, 
             &seeders, &leechers, &peerListMutex)
{
    std::srand((unsigned int)time(NULL));
    this->port = port;
    this->outputDirectory = outputDirectory;
    this->trackerHost = trackerHost;
    this->trackerPort = trackerPort;

    curMetadata = nullptr;
    isDestructing = false;

    // btw, does just initializaing socketwork do something or did whoever implement this
    //   forgot to use it?
    boost::asio::io_service::work socketWork(ioService);

    serverThread = std::thread(&UDPListener::run, &server);
    std::string str = "Server on port " + std::to_string(port) + " is running.\n";
    std::cout << str;

    requestingChunkThread = std::thread(std::bind(&Peer::periodicallyRequestChunks, this));
    startNextDatadistThread = std::thread(&Peer::periodicallyStartNextDatadist, this);

    loadAllMetadata();
    registerAndQueueDataDists();
}

// Pre: findChunksToReceive() should be called before
//      curMetadata should be valid
int Peer::getDatadistProgress() {
    std::uint64_t totalChunkCount = 0;
    for (int i = 0; i < curMetadata->getFileCount(); i++) {
        auto filePath = curMetadata->getFilePath(i);
        totalChunkCount += curMetadata->getChunkCountForFile(filePath);
    }

    std::uint64_t incompleteChunkCount = 0;
    for (auto& fileChunks : chunksToReceive) {
        incompleteChunkCount += fileChunks.second.size();
    }

    auto completeChunkCount = totalChunkCount - incompleteChunkCount;

    int progress;
    if (totalChunkCount == 0) {
        progress = 0;
    } else {
        progress = (int)((float)completeChunkCount / totalChunkCount * 100);
    }

    return progress;
}

std::shared_ptr<std::unordered_set<std::string>> Peer::getCompleteFiles(
        std::shared_ptr<Metadata> metadata) {
    std::string completedFilesFilePath = Util::getCompletedFilesFilePath(outputDirectory, metadata);
    auto completedFiles = std::shared_ptr<std::unordered_set<std::string>>(
        new std::unordered_set<std::string>());
    if (FileHandler::doesFileExist(completedFilesFilePath))
    {
        auto completedFilesHandler = std::unique_ptr<FileHandler>(
            new FileHandler(Util::getCompletedFilesFilePath(outputDirectory, metadata), true));

        std::string filePath;
        filePath = completedFilesHandler->readLine();
        while (!filePath.empty())
        {
            completedFiles->insert(filePath);
            filePath = completedFilesHandler->readLine();
        }
        completedFilesHandler->close();
    }

    return completedFiles;
}

void Peer::run() {
    ioService.run();
}

void Peer::addDatadistToDownload(std::shared_ptr<Metadata> metadata)
{
    metadataQueue.push(metadata);
}

std::unordered_map<std::string, std::shared_ptr<Metadata>> Peer::getActiveMetadata()
{
    std::lock_guard<std::mutex> activeMetadataMutexLock(activeMetadataMutex);
    return activeMetadata;
}

std::unique_ptr<Peer> Peer::spawnTestPeer(uint16_t port, 
        std::string outputDirectory, std::shared_ptr<Metadata> metadata,
        std::string trackerHost, int trackerPort) {
    auto peer = std::unique_ptr<Peer>(new Peer(port, outputDirectory, trackerHost, trackerPort));
    if (metadata) {
        // Explicitly calling this function is to simulate adding a new datadist to download
        //  while the program is already running.
        peer->addDatadistToDownload(metadata);
    }
    peer->run();
    return peer;
}

Peer::~Peer() {
    if (!isDestructing) {
        destruct();
    }
}

void Peer::destruct() {
    isDestructing = true;

    server.stop();
    ioService.stop();

    if (startNextDatadistThread.joinable()) {
        startNextDatadistThread.join();
    }

    if (requestingChunkThread.joinable()) {
        requestingChunkThread.join();
    }

    for (auto& thread : periodicallyKeepAliveThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    if (serverThread.joinable()) {
        serverThread.join();
    }
}

void Peer::loadAllMetadata() {

    if (!boost::filesystem::exists(outputDirectory) || 
        !boost::filesystem::is_directory(outputDirectory)) {

        std::string str = "Loading all metadata failed. " + 
                          outputDirectory + 
                          " is not a directory.\n";
        std::cout << str;
        return;
    }

    boost::filesystem::directory_iterator end_itr;
    std::lock_guard<std::mutex> activeMetadataMutexLock(activeMetadataMutex);
    for (boost::filesystem::directory_iterator it(outputDirectory); it != end_itr; it++) {
        if (!boost::filesystem::is_regular_file(it->status())) {
            continue;
        } else if (it->path().extension() == META_FILTER) {
            std::shared_ptr<Metadata> metadata;
            try {
                metadata = MetadataHandler::getMetadata(it->path().generic_string());
            } catch (std::exception e) {
                std::cout << e.what() << std::endl;
                continue;
            }
            std::pair<std::string, std::shared_ptr<Metadata>> metaPair(metadata->getMetaFileName(), metadata);
            activeMetadata.insert(metaPair);

            //For testing
            std::string str = "Loaded metadata " + it->path().generic_string() + ".\n";
            std::cout << str;
        }
    }
}


bool Peer::isDownloadingDone()
{
    std::lock_guard<std::mutex> lock(chunksToReceiveMutex);
    return chunksToReceive.empty();
}

void Peer::periodicallyStartNextDatadist()
{
    while (!isDestructing) {
        try {
            if (isDownloadingDone()) {
                if (requestingChunkThread.joinable()) {
                    requestingChunkThread.join();
                }

                // To indicate that this peer has become a seeder now
                //  and get rid of himself from leecher list.
                // Otherwise, this peer gets peer list for more than one data dist trackers
                //  and only the last data dist tracker is in this peer's peer list.
                if (curMetadata) {
                    sendProgress(curMetadata->getMetaFileName(), 100);
                }

                if (!metadataQueue.empty()) {
                    auto nextMetadata = metadataQueue.front();
                    metadataQueue.pop();

                    if (!nextMetadata) {
                        continue;
                    }

                    server.setMetadata(nextMetadata);
                    curMetadata = nextMetadata;

                    {
                        std::lock_guard<std::mutex> activeMetadataMutexLock(activeMetadataMutex);
                        activeMetadata.insert(std::pair<std::string, std::shared_ptr<Metadata>>(
                            curMetadata->getMetaFileName(), curMetadata));
                    }

                    findChunksToReceive();

                    if (chunksToReceive.empty()) {
                        continue;
                    }

                    {
                        std::lock_guard<std::mutex> peerListMutexLock(peerListMutex);
                        seeders.clear();
                        leechers.clear();
                        std::string str = "Peer: cleared peer list.\n";
                        std::cout << str;
                    }

                    auto dataDistId = nextMetadata->getMetaFileName();
                    int progress = getDatadistProgress();
                    sendProgress(dataDistId, progress);

                    server.setChunksToReceive(&chunksToReceive, &chunksToReceiveMutex);
                    requestingChunkThread = std::thread(std::bind(&Peer::periodicallyRequestChunks, this));
                }
            }
        } catch (std::exception e) {
            Util::showException(e);
        }
        
        std::this_thread::sleep_for(CHECK_DL_STATUS_INTERVAL);
    }
}

void Peer::sendProgress(std::string datadistId, int progress)
{
    auto it = dataDistProgress.find(datadistId);
    dataDistProgress[datadistId] = progress;

    client.sendProgress(datadistId, progress, trackerHost, trackerPort, port);

    if (it == dataDistProgress.end()) {
        periodicallyKeepAliveThreads.push_back(std::thread(
            std::bind(&Peer::periodicallyKeepAlive, this,
                datadistId, trackerHost, trackerPort)));
    }
}

// Pre: LoadAllMetadata() should be called before.
// Post: Remove queued data dists' metadata from activeMetadata.
void Peer::registerAndQueueDataDists() {
    std::unordered_set<std::string> inactiveMetaFiles;

    std::lock_guard<std::mutex> activeMetadataMutexLock(activeMetadataMutex);
    for (auto& metaPair : activeMetadata) {
        auto metadatum = metaPair.second;
        auto actualFileCount = metadatum->getFileCount();
        auto completeFileCount = getCompleteFiles(metadatum)->size();

        if (completeFileCount >= actualFileCount) {
            sendProgress(metadatum->getMetaFileName(), 100);
        } else {
            inactiveMetaFiles.insert(metaPair.first);

            // Automatically pick up any incomplete datadist to continue downloading
            addDatadistToDownload(metadatum);
        }
    }

    for (auto& metaFileName : inactiveMetaFiles) {
        // inactive metadata will be added back in when its downloading starts
        activeMetadata.erase(activeMetadata.find(metaFileName));
    }
}

void Peer::requestChunks(std::string metafileName)
{
    std::lock_guard<std::mutex> chunksToReceiveMutexLock(chunksToReceiveMutex);

    std::vector<DataDistPeer> seedersVec(seeders.size());
    std::vector<DataDistPeer> leechersVec(leechers.size());

    {
        std::lock_guard<std::mutex> peerListMutexLock(peerListMutex);

        std::transform(seeders.begin(), seeders.end(), seedersVec.begin(),
            [](const std::pair<std::string, DataDistPeer>& peerPair) {
                return peerPair.second;
            }
        );

        std::transform(leechers.begin(), leechers.end(), leechersVec.begin(),
            [](const std::pair<std::string, DataDistPeer>& peerPair) {
                return peerPair.second;
            }
        );
    }

    for (auto it = chunksToReceive.begin(); it != chunksToReceive.end(); it++)
    {
        auto &chunkReceivedMap = it->second;
        for (auto mapIt = chunkReceivedMap.begin(); mapIt != chunkReceivedMap.end(); mapIt++)
        {
            if (mapIt->second) {
                continue;
            }

            std::uint64_t numPeers = seedersVec.size() + leechersVec.size();
            if (numPeers == 0) {
                continue;
            }

            unsigned int randIndex = std::rand() % numPeers;
            std::vector<DataDistPeer>& peers =
                randIndex < seedersVec.size() ? seedersVec : leechersVec;
            randIndex = randIndex < seedersVec.size() ? randIndex : randIndex - seedersVec.size();

            ioService.post(std::bind(&UDPClient::requestChunk, &client,
                           metafileName, it->first, mapIt->first,
                           peers[randIndex].getHost(), peers[randIndex].getPort()));

            std::this_thread::sleep_for(REQUEST_RATE_INTERVAL);
        }
    }
}

void Peer::findChunksToReceive()
{    
    if (!curMetadata) {
        std::string str = "curMetadata is not set.\n";
        std::cout << str;
        return;
    }

    uint64_t dataDistFileCount = curMetadata->getFileCount();
    auto completedFiles = getCompleteFiles(curMetadata);

    std::string str = "Number of files in data dist is: " + 
                      std::to_string(dataDistFileCount) + 
                      "\n" +
                      "Number of files already completed is: " +
                      std::to_string(completedFiles->size()) +
                      "\n";
    std::cout << str;

    for (int i = 0; i < dataDistFileCount; i++)
    {
        std::string path = curMetadata->getFilePath(i);
        if (completedFiles->find(path) != completedFiles->end())
            continue;

        std::vector<uint32_t> incompleteChunkIndicies;

        std::string customPath = outputDirectory + "/" + path;
        if (FileHandler::doesFileExist(customPath))
        {
            int fileSize = 0;   // File exists, FileHandler will get fileSize itself
            FileHandler fileHandler(customPath, path, curMetadata->getChunkSize(),
                                    fileSize, curMetadata->getHashesForFile(path));
            incompleteChunkIndicies = fileHandler.getIncompleteChunkIndicies();
        }
        else
        {
            incompleteChunkIndicies.resize(curMetadata->getChunkCountForFile(path));
            std::iota(incompleteChunkIndicies.begin(), incompleteChunkIndicies.end(), 0);
        }

        if (!incompleteChunkIndicies.empty())
        {
            std::unordered_map<uint32_t, bool> indicies;
            for (uint32_t i : incompleteChunkIndicies)
                indicies.insert(std::pair<uint32_t, bool>(i, false));

            std::lock_guard<std::mutex> chunksToReceiveMutexLock(chunksToReceiveMutex);
            chunksToReceive.insert(std::pair<std::string, std::unordered_map<uint32_t, bool>>(path, indicies));
        }
        else
        {
            // A rare case where the completed files file is modified manually.
            // Just regenerate the missing file paths
            auto completedFilesHandler = std::unique_ptr<FileHandler>(
                new FileHandler(Util::getCompletedFilesFilePath(outputDirectory, curMetadata), false));
            completedFilesHandler->writeLine(path);
            completedFilesHandler->close();
        }
    }
}

// Pre: dataDistId must be valid id
void Peer::periodicallyKeepAlive(std::string dataDistId, const std::string & host, uint16_t port) {
    while (!isDestructing) {
        std::this_thread::sleep_for(KEEPALIVE_INTERVAL);

        try {
            client.sendProgress(dataDistId, dataDistProgress[dataDistId], host, port, this->port);
        }
        catch (std::exception e) {
            Util::showException(e);
        }
    }
}

//We'll have to make this smart in order to do the rate limiting properly.
void Peer::periodicallyRequestChunks()
{
    while (!isDestructing && !chunksToReceive.empty())
    {
        try {
            requestChunks(curMetadata->getMetaFileName());
        } catch (std::exception e) {
            Util::showException(e);
        }

        std::this_thread::sleep_for(CHUNK_REQUEST_INTERVAL);
    }
}