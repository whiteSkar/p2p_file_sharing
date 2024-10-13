#include "DataDistServer.h"

// TODO: Constant organization task 
const std::string DataDistServer::META_FILTER = ".meta";

DataDistServer::DataDistServer(short port, std::string metadataDirectory)
    :socket(io_service, udp::endpoint(udp::v4(), port))
{
    std::cout << "DataDistServer is running on port: " << port << std::endl;

    this->metadataDirectory = metadataDirectory;
    isDestructing = false;

    addAllDataDists();
    receive();
}

DataDistServer::DataDistServer()
    : socket(io_service, udp::endpoint(udp::v4(), DEFAULT_SERVER_PORT))
{

}

DataDistServer::DataDistServer(const DataDistServer& dataDistServer)
    : socket(io_service, udp::endpoint(udp::v4(), DEFAULT_SERVER_PORT))
{
}

DataDistServer::~DataDistServer() {
    if (!isDestructing) {
        destruct();
    }
}

void DataDistServer::run() {
    boost::asio::io_service::work work(threadpool_ioservice);
    threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &threadpool_ioservice));
    processingThread = std::thread(&DataDistServer::trackAndUpdatePeers, this);
    io_service.run();
}

void DataDistServer::destruct() {
    isDestructing = true;
    io_service.stop();
    if (processingThread.joinable()) {
        processingThread.join();
    }
}

void DataDistServer::addAllDataDists() {

    if (!boost::filesystem::exists(metadataDirectory) ||
        !boost::filesystem::is_directory(metadataDirectory)) {

        std::string str = "Adding all metadata FAILED. " +
            metadataDirectory +
            " is not a directory.\n";
        std::cout << str;
        return;
    }

    boost::filesystem::directory_iterator end_itr;
    for (boost::filesystem::directory_iterator it(metadataDirectory); it != end_itr; it++) {
        if (!boost::filesystem::is_regular_file(it->status())) {
            continue;
        } else if (it->path().extension() == META_FILTER) {
            addDataDist(it->path().filename().generic_string());
        }
    }
}

void DataDistServer::trackAndUpdatePeers() {
    while (!isDestructing) {
        std::this_thread::sleep_for(std::chrono::seconds(TRACK_PEERS_INTERVAL / 2));
        threadpool_ioservice.post(std::bind(&DataDistServer::trackPeers, this));

        std::this_thread::sleep_for(std::chrono::seconds(TRACK_PEERS_INTERVAL / 2));
        threadpool_ioservice.post(std::bind(&DataDistServer::sendPeerlistsToPeers, this));
    }
}

void DataDistServer::trackPeers() {
    for (auto& distAndTracker : dataDists) {
        std::vector<DataDistPeer> leechersToRemove;
        std::vector<DataDistPeer> seedersToRemove;

        auto& distTracker = distAndTracker.second.first;
        auto distTrackerPeerListMutex = distAndTracker.second.second;
        std::lock_guard<std::mutex> distTrackerPeerListMutexLock(*distTrackerPeerListMutex.get());

        for (auto seeder : distTracker.getSeeders()) {
            if (!isAlive(seeder)) {
                seedersToRemove.push_back(seeder);
            }
        }

        for (auto leecher : distTracker.getLeechers()) {
            if (!isAlive(leecher)) {
                leechersToRemove.push_back(leecher);
            }
        }

        for (auto leecher : leechersToRemove) {
            std::string str = "Leecher " + leecher.getId() + " timed out. Removing from peerlist.\n";
            std::cout << str;
            distTracker.deleteLeecher(leecher.getId());
        }

        for (auto seeder : seedersToRemove) {
            std::string str = "Seeder " + seeder.getId() + " timed out. Removing from peerlist.\n";
            std::cout << str;
            distTracker.deleteSeeder(seeder.getId());
        }
    }
}

bool DataDistServer::isAlive(DataDistPeer peer) {
    auto curTime = std::chrono::system_clock::now();
    auto peerLastTimeAlive = peer.getLastTimeAlive();
    std::chrono::duration<double> inactiveDuration = curTime - peerLastTimeAlive;

    return inactiveDuration.count() < PEER_TIMEOUT_SECONDS;
}

void DataDistServer::sendPeerlistsToPeers() {
    for (auto& nameAndDist : dataDists) {
        auto& distTracker = nameAndDist.second.first;
        auto distTrackerPeerListMutex = nameAndDist.second.second;
        std::lock_guard<std::mutex> distTrackerPeerListMutexLock(*distTrackerPeerListMutex.get());

        auto seeders = distTracker.getSeeders();
        auto leechers = distTracker.getLeechers();

        for (auto leecher : leechers) {
            //Sends subset of entire list, fits within one packet.
            sendPeerList(leecher, std::move(PeerListPacket(leecher.getId(), leechers, false)));
            sendPeerList(leecher, std::move(PeerListPacket(leecher.getId(), seeders, true)));
        }
    }
}

void DataDistServer::sendPeerList(DataDistPeer peer, PeerListPacket peerListPacket) {
    auto endpoint = udp::endpoint(boost::asio::ip::address::from_string(peer.getHost()), peer.getPort());
    auto serializedPacket = peerListPacket.serializePacket();
    auto buffer = boost::asio::const_buffers_1(serializedPacket.data(), serializedPacket.length());
    socket.async_send_to(buffer, endpoint,
        boost::bind(&DataDistServer::onDataSendComplete, this, buffer,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void DataDistServer::onDataSendComplete(boost::asio::const_buffers_1 buffer,
    const boost::system::error_code &error, size_t bytes_transferred) {
        if (!error && bytes_transferred > 0) {
            return;
        }
        throw std::exception("*DEPRECATED*: Received no bytes or error");
}

/**
* Listens on the socket for packets, if one is received, calls the handler.
*/
void DataDistServer::receive()
{
    auto data = std::shared_ptr<char>(new char[UdpPacket::MAX_SIZE]);
    auto buffer = boost::asio::mutable_buffers_1(data.get(), UdpPacket::MAX_SIZE);

    auto sender_endpoint = std::shared_ptr<udp::endpoint>(new udp::endpoint());
    socket.async_receive_from(buffer,
        *sender_endpoint,
        boost::bind(&DataDistServer::onDataReceive,
                    this, data, sender_endpoint,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
}

/**
 * Async handler to call upon receiving a packet.
 * @param   data    Shared pointer to the data in the packet
 * @param   error   The boost error code that in the event that an error occurs
 * @param   bytes_recvd    The number of bytes received.
*/
void DataDistServer::onDataReceive(std::shared_ptr<char> data,
                                   std::shared_ptr<udp::endpoint> sender_endpoint,
                                   const boost::system::error_code & error,
                                   size_t bytes_recvd) {
    if (error) {
        std::cout << error.message() << std::endl;
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
        threadpool_ioservice.post(std::bind(&DataDistServer::processPacket, this, udpPacket, sender_endpoint));
        receive();
    } catch (std::exception e) {
        std::cout << "Error constructing packet. Exception MSG: " << e.what() << std::endl;
        receive();
    }
}

void DataDistServer::processPacket(std::shared_ptr<UdpPacket> udpPacket,
                                   std::shared_ptr<udp::endpoint> sender_endpoint) {
    auto packetType = udpPacket->getPacketType();
    if (packetType == PacketHeader::PacketType::PeerProgress) {
        PeerProgressPacket peerPacket(*udpPacket);
        auto progress = peerPacket.getProgress();

        if (progress == 100) {
            updateSeeder(peerPacket.getDataDistId(), 
                         DataDistPeer(sender_endpoint->address().to_string(), 
                                      sender_endpoint->port(),
                                      peerPacket.getLocalAddress(),
                                      peerPacket.getLocalPort(),
                                      progress));
        } else {
            updateLeecher(peerPacket.getDataDistId(), 
                          DataDistPeer(sender_endpoint->address().to_string(),
                                       sender_endpoint->port(),
                                       peerPacket.getLocalAddress(),
                                       peerPacket.getLocalPort(),
                                       progress));
        }
    }
}


// For production, might want to modify this function so that only valid users can call any of the below functions.
bool DataDistServer::addDataDist(std::string dataDistId) {
    std::cout << "DataDistServer: Added data dist: " << dataDistId << std::endl;

    auto mutex = std::shared_ptr<std::mutex>(new std::mutex());
    return dataDists.insert(std::make_pair(dataDistId, 
                            std::make_pair(std::move(DataDistTracker()), 
                                           mutex))).second;
}

void DataDistServer::updateSeeder(std::string dataDistId, DataDistPeer peer) {
    auto it = dataDists.find(dataDistId);
    if (it != dataDists.end()) {
        auto& distTracker = it->second.first;
        auto distTrackerPeerListMutex = it->second.second;
        std::lock_guard<std::mutex> distTrackerPeerListMutexLock(*distTrackerPeerListMutex.get());

        if (!distTracker.isSeederRegistered(peer)) {
            std::string str = "A seeder at endpoint: " + peer.getEndpoint()
                + " has been registered with DataDist: " + dataDistId + "\n";
            std::cout << str;
        }

        distTracker.updateSeeder(peer);

        if (distTracker.isLeecherRegistered(peer)) {
            removeLeecher(dataDistId, peer.getId());
        }
    }
}

void DataDistServer::updateLeecher(std::string dataDistId, DataDistPeer peer)
{
    auto it = dataDists.find(dataDistId);
    if (it != dataDists.end()) {
        auto& distTracker = it->second.first;
        auto distTrackerPeerListMutex = it->second.second;
        std::lock_guard<std::mutex> distTrackerPeerListMutexLock(*distTrackerPeerListMutex.get());

        if (!distTracker.isLeecherRegistered(peer)) {
            std::string str = "A leecher at endpoint: " + peer.getEndpoint()
                + " has been registered with DataDist: " + dataDistId + "\n";
            std::cout << str;
        }

        distTracker.updateLeecher(peer);
    }
}

void DataDistServer::removeSeeder(std::string dataDistId, std::string dataDistPeerId)
{
    auto it = dataDists.find(dataDistId);
    if (it != dataDists.end()) {
        auto& distTracker = it->second.first;
        // no lock because it's already locked in updateSeeder()

        distTracker.deleteSeeder(dataDistPeerId);

        std::string str = "A seeder " + dataDistPeerId
                        + " has been removed from DataDist: " + dataDistId + "\n";
        std::cout << str;
    }
}

void DataDistServer::removeLeecher(std::string dataDistId, std::string dataDistPeerId)
{
    auto it = dataDists.find(dataDistId);
    if (it != dataDists.end()) {
        auto& distTracker = it->second.first;
        // no lock because it's already locked in updateLeecher()

        distTracker.deleteLeecher(dataDistPeerId);

        std::string str = "A leecher " + dataDistPeerId
                        + " has been removed from DataDist: " + dataDistId + "\n";
        std::cout << str;
    }
}

std::vector<DataDistPeer> DataDistServer::fetchSeederList(std::string dataDistId)
{
    auto it = dataDists.find(dataDistId);
    if (it != dataDists.end()) {
        auto& distTracker = it->second.first;
        auto distTrackerPeerListMutex = it->second.second;
        std::lock_guard<std::mutex> distTrackerPeerListMutexLock(*distTrackerPeerListMutex.get());

        return distTracker.getSeeders();
    }

    return std::vector<DataDistPeer>();
}

std::vector<DataDistPeer> DataDistServer::fetchLeecherList(std::string dataDistId)
{
    auto it = dataDists.find(dataDistId);
    if (it != dataDists.end()) {
        auto& distTracker = it->second.first;
        auto distTrackerPeerListMutex = it->second.second;
        std::lock_guard<std::mutex> distTrackerPeerListMutexLock(*distTrackerPeerListMutex.get());

        return distTracker.getLeechers();
    }

    return std::vector<DataDistPeer>();
}