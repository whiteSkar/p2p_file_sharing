#include "Crypto.h"
#include "UDPListener.h"


const std::chrono::microseconds UDPListener::RESPONSE_RATE_INTERVAL = std::chrono::microseconds(500);

UDPListener::UDPListener(boost::asio::ip::udp::socket *socket, 
                         std::mutex *socketMutex, 
                         std::string outputDirectory,
                         std::unordered_map<std::string, std::shared_ptr<Metadata>> *activeMetadata,
                         std::mutex *activeMetadataMutex,
                         std::unordered_map<std::string, DataDistPeer> *seeders,
                         std::unordered_map<std::string, DataDistPeer> *leechers,
                         std::mutex *peerListMutex)
{
    if (!socket || !socketMutex) {
        throw std::invalid_argument("socket or socketMutex is null");
    }

    this->socket = socket;
    this->socketMutex = socketMutex;
    this->outputDirectory = outputDirectory;
    this->activeMetadata = activeMetadata;
    this->activeMetadataMutex = activeMetadataMutex;
    this->seeders = seeders;
    this->leechers = leechers;
    this->peerListMutex = peerListMutex;

    metadata = nullptr;
    directoryHandlerForReceiving = nullptr;
    chunksToReceive = nullptr;

    numPacketsToSend = 0;
    receive();
}

UDPListener::UDPListener()
{
}

void UDPListener::run()
{
    boost::asio::io_service::work work(threadpool_ioservice);

    threadpool_ioservice.run();
}

void UDPListener::stop()
{
    threadpool_ioservice.stop();
}

void UDPListener::receive()
{
    auto data = std::shared_ptr<char>(new char[UdpPacket::MAX_SIZE]);
    auto buffer = boost::asio::mutable_buffers_1(data.get(), UdpPacket::MAX_SIZE);

    auto sender_endpoint = std::shared_ptr<udp::endpoint>(new udp::endpoint());
    socketMutex->lock();
    socket->async_receive_from(buffer,
        *sender_endpoint,
        boost::bind(&UDPListener::onDataReceive,
                    this, data, sender_endpoint,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
    socketMutex->unlock();
}

void UDPListener::setChunksToReceive(std::unordered_map<std::string, std::unordered_map<uint32_t, bool>> *chunksToReceive,
    std::mutex *chunksToReceiveMutex)
{
    if (!chunksToReceive || !chunksToReceiveMutex)
        throw std::invalid_argument("chunksToReceive or chunksToReceiveMutex is null.");

    this->chunksToReceiveMutex = chunksToReceiveMutex;
    this->chunksToReceiveMutex->lock();
    this->chunksToReceive = chunksToReceive;
    this->chunksToReceiveMutex->unlock();
}

void UDPListener::onDataReceive(std::shared_ptr<char> data, 
                                std::shared_ptr<udp::endpoint> sender_endpoint, 
                                const boost::system::error_code &error, 
                                size_t bytes_recvd)
{
    if (error) {
        // Linux socekt error codes ECONNRESET and ECONNREFUSED aren't tested 
        if (error.value() == WSAECONNRESET || error.value() == WSAECONNREFUSED ||
            error.value() == ECONNRESET || error.value() == ECONNREFUSED) { 

            std::string senderEndpointStr = sender_endpoint->address().to_string() +
                                            ":" +
                                            std::to_string(sender_endpoint->port());

            std::lock_guard<std::mutex> lockGuard(*peerListMutex);
            auto it = seeders->find(senderEndpointStr);
            if (it != seeders->end()) {
                seeders->erase(it);
            }

            it = leechers->find(senderEndpointStr);
            if (it != leechers->end()) {
                leechers->erase(it);
            }
        }

        receive();
        return;
    } else if (bytes_recvd == 0) {
        std::cout << "onDataReceive received 0 bytes." << std::endl;
        receive();
        return;
    }

    try {
        auto receivedBytes = (unsigned char *)data.get();
        auto udpPacket = std::shared_ptr<UdpPacket>(new UdpPacket(receivedBytes, (uint32_t)bytes_recvd));
        threadpool_ioservice.post(std::bind(&UDPListener::processPacket, this, udpPacket, sender_endpoint));
        receive();
    } catch (std::exception e) {
        std::cout << "EXCEPTION: " << e.what() << std::endl;
        receive();
    }
}

void UDPListener::handleChunkRequest(std::shared_ptr<UdpPacket> udpPacket, 
                                     std::shared_ptr<udp::endpoint> sender_endpoint) {
    auto chunkRequestPacket = ChunkRequestPacket(*udpPacket);
    auto chunkNumber = chunkRequestPacket.getChunkNumber();
    auto metadataName = chunkRequestPacket.getMetadataName();
    std::string filePath = chunkRequestPacket.getFilePath();
    std::string str = "Received message requesting chunk: " + std::to_string(chunkNumber)
                    + " for file " + filePath + " of metadata " + metadataName;
    showMsg(str);
    
    std::shared_ptr<Metadata> requestedMetadata;
    {
        std::lock_guard<std::mutex> activeMetadataMutexLock(*activeMetadataMutex);
        auto it = activeMetadata->find(metadataName);
        if (it == activeMetadata->end()) {
            showMsg("Requested datadist " + metadataName + " is not available.");
            return;
        }
        requestedMetadata = it->second;
    }

    auto it = directoryHandlersForSending.find(requestedMetadata->getMetaFileName());
    if (it == directoryHandlersForSending.end()) {
        showMsg("directoryHandlerForSending_ is not yet initialized.");

        directoryHandlersForSending[requestedMetadata->getMetaFileName()] = 
            std::shared_ptr<DirectoryHandler>(
                new DirectoryHandler(outputDirectory, requestedMetadata));
        str = "Requested dataDist " + requestedMetadata->getRootDirectoryName() + " is loaded.";
        showMsg(str);
    }
    
    auto directoryHandlerForSending = 
        directoryHandlersForSending[requestedMetadata->getMetaFileName()];

    if (!directoryHandlerForSending->getMetadata()->isThisFilePartOfDataDist(filePath))
    {
        showMsg("Requested file is not part of the seeding data dist.");
        return;
    }

    if (!directoryHandlerForSending->doesFileExist(filePath)) {
        str = "Server doesn't have " + filePath + " yet.";
        showMsg(str);;
        return;
    }

    auto file = directoryHandlerForSending->getFile(filePath, true);
    auto chunk = file->getChunkForSending(chunkNumber);
    if (!chunk->isComplete()) {
        str = "Requested chunk " + std::to_string(chunkNumber)
            + " of file " + filePath + " is incomplete.";
        showMsg(str);;
        return;
    }

    numPacketsToSend = chunk->getNumberOfPackets();

    try {
        for (int i = 0; i < numPacketsToSend; i++)
        {
            auto packetToSend = chunk->getPacket(i);
            if (!packetToSend) {
                continue;
            }
            auto dataToSend = packetToSend->serializePacket();
            socketMutex->lock();
            socket->send_to(boost::asio::const_buffers_1(
                                dataToSend.data(), dataToSend.length()), *sender_endpoint);
            socketMutex->unlock();
        }
        file->removeFromActiveChunks(chunk);
    }
    catch (std::runtime_error e) {
        return;
    }
}

void UDPListener::handlePeerListPacket(std::shared_ptr<UdpPacket> udpPacket) {
    PeerListPacket peerListPacket;
    try {
        peerListPacket = PeerListPacket(*udpPacket);
        // Change to catch parsing errors only.
    }
    catch (std::exception e) {
        return;
    }

    std::lock_guard<std::mutex> lockGuard(*peerListMutex);
    if (peerListPacket.getAreSeeders()) {

        for (auto& seeder : peerListPacket.getPeers()) {

            if (peerListPacket.getPeerEndpoint() == seeder.getEndpoint()) {
                continue;
            }
            else if (seeder.getHost() == peerListPacket.getPeerInfo().getHost() && seeder.getLocalHost() != "") {
                auto localSeeder = DataDistPeer(seeder.getLocalHost(), seeder.getLocalPort(), seeder.getLocalHost(), seeder.getLocalPort());
                (*seeders)[localSeeder.getEndpoint()] = localSeeder;
            }
            else {
                (*seeders)[seeder.getEndpoint()] = seeder;
            }
        }
    }
    else {
        for (auto& leecher : peerListPacket.getPeers()) {
            if (peerListPacket.getPeerEndpoint() == leecher.getEndpoint()) {
                continue;
            }
            else if (leecher.getHost() == peerListPacket.getPeerInfo().getHost() && leecher.getLocalHost() != "") {
                auto localLeecher = DataDistPeer(leecher.getLocalHost(), leecher.getLocalPort(), leecher.getLocalHost(), leecher.getLocalPort());
                (*leechers)[localLeecher.getEndpoint()] = localLeecher;
            }
            else {
                (*leechers)[leecher.getEndpoint()] = leecher;
            }
        }
    }
}

void UDPListener::setMetadata(std::shared_ptr<Metadata> metadata)
{
    this->metadata = metadata;
}

void UDPListener::handleFilePacket(std::shared_ptr<UdpPacket> udpPacket) {
    auto filePacket = FilePacket(*udpPacket);
    auto chunkNumber = filePacket.getChunkNumber();

    std::string filePath = filePacket.getFilePath();
    if (!metadata || !metadata->isThisFilePartOfDataDist(filePath))
    {
        return;
    }

    if (!directoryHandlerForReceiving || directoryHandlerForReceiving->getMetadata() != metadata) {
        directoryHandlerForReceiving = std::shared_ptr<DirectoryHandler>(
            new DirectoryHandler(outputDirectory, metadata));

        completedFileHandler = std::unique_ptr<FileHandler>(
            new FileHandler(Util::getCompletedFilesFilePath(outputDirectory, metadata), false));
    }

    auto file = directoryHandlerForReceiving->getFile(filePath, false);

    chunksToReceiveMutex->lock();
    if (!chunksToReceive) {
        return;
    }
    auto chunksToReceiveIt = chunksToReceive->find(filePath);
    if (chunksToReceiveIt == chunksToReceive->end())
    {
        std::string str = "Received a file packet for a file that is not requested or is already downloaded.";
        showMsg(str);
        chunksToReceiveMutex->unlock();
        return;
    }

    if (file->getRequiredChunkCount() == 0)
    {
        // Required chunk indicies is not set yet
        std::vector<uint32_t> indicies;
        for (auto &indexBoolPair : chunksToReceiveIt->second)
            indicies.push_back(indexBoolPair.first);
        file->setRequiredChunkIndicies(std::move(indicies));
    }

    auto chunkIndexBoolPairIt = chunksToReceiveIt->second.find(chunkNumber);
    if (chunkIndexBoolPairIt == chunksToReceiveIt->second.end() || chunkIndexBoolPairIt->second)
    {
        std::string str = "Received a file packet for a chunk that is not requested or is already received.";
        showMsg(str);
        chunksToReceiveMutex->unlock();
        return;
    }
    chunksToReceiveMutex->unlock();

    auto writeChunk = file->createChunkForReceiving(chunkNumber);
    writeChunk->putPacket(filePacket);

    if (writeChunk->isComplete())
    {
        std::string fileFullPath = directoryHandlerForReceiving->getDirectoryPath() + "/" + filePath;
        std::string str = "Chunk " + std::to_string(chunkNumber)
                        + " of " + fileFullPath + " is complete.";
        showMsg(str);

        file->writeReceivedChunk(writeChunk);
            
        chunksToReceiveMutex->lock();
        auto chunksToReceiveIt = chunksToReceive->find(filePath);
        if (chunksToReceiveIt != chunksToReceive->end())
            chunksToReceiveIt->second.find(chunkNumber)->second = true;
        chunksToReceiveMutex->unlock();
    }
    if (file->isComplete())
    {
        std::string str = "*** All chunks are received. File " + file->getAbsPath()
                        + " is complete and output at: " + directoryHandlerForReceiving->getDirectoryPath()
                        + " ***";
        showMsg(str);
        
        directoryHandlerForReceiving->closeFile(filePath);

        completedFileHandler->writeLine(filePath);

        chunksToReceiveMutex->lock();
        chunksToReceive->erase(filePath);
        if (chunksToReceive->empty())
        {
            directoryHandlerForReceiving = nullptr;

            str = "\n*************************************************************** \
                   \nDataDist download complete. \
                   \n***************************************************************\n";
            showMsg(str);;
        }
        chunksToReceiveMutex->unlock();
    }
}


void UDPListener::processPacket(std::shared_ptr<UdpPacket> udpPacket,
                                std::shared_ptr<udp::endpoint> sender_endpoint) {
    try {
        auto packetType = udpPacket->getPacketType();

        if (packetType == PacketHeader::PacketType::ChunkRequest) {
            handleChunkRequest(udpPacket, sender_endpoint);
        }
        else if (packetType == PacketHeader::PacketType::FilePacket) {
            handleFilePacket(udpPacket);
        }
        else if (packetType == PacketHeader::PacketType::PeerList) {
            handlePeerListPacket(udpPacket);
        }
    } catch (std::exception e) {
        std::cout << "EXCEPTION: " << e.what() << std::endl;
        receive();
    }
}

void UDPListener::showMsg(std::string msg) {
    msg = std::to_string(socket->local_endpoint().port()) + ": " + msg + "\n";
    std::cout << msg;
}