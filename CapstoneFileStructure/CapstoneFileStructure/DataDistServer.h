#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <chrono>

#include "PacketHeader.h"
#include "PeerProgressPacket.h"
#include "DataDistTracker.h"
#include "PeerListPacket.h"

#pragma once

using boost::asio::ip::udp;

class DataDistServer
{
public:
    // TODO: Constant organization task 
    static const std::string META_FILTER;

    DataDistServer(short port, std::string metadataDirectory);
    DataDistServer();
    DataDistServer(const DataDistServer & dataDistServer);
    ~DataDistServer();

    void run();
    void destruct();

    void addAllDataDists();

    void receive();
    void onDataReceive(std::shared_ptr<char> data, 
                       std::shared_ptr<udp::endpoint> sender_endpoint,
                       const boost::system::error_code & error,
                       size_t bytes_recvd);
    void processPacket(std::shared_ptr<UdpPacket> udpPacket,
                       std::shared_ptr<udp::endpoint> sender_endpoint);

    /**
    * Adds a DataDistTracker instance to the DataDistServer
    * @param  dataDistId  String indicating the ID of the dataDist that you wish to insert.
    * @return  boolean indicating whether you successfully added a new DataDist
    */
    bool addDataDist(std::string dataDistId);
    void updateSeeder(std::string dataDistId, DataDistPeer dataDistPeer);
    void updateLeecher(std::string dataDistId, DataDistPeer dataDistPeer);
    void removeSeeder(std::string dataDistId, std::string dataDistPeerId);
    void removeLeecher(std::string dataDistId, std::string dataDistPeerId);
    std::vector<DataDistPeer> fetchSeederList(std::string dataDistId);
    std::vector<DataDistPeer> fetchLeecherList(std::string dataDistId);

private:
    void sendPeerList(DataDistPeer peer, PeerListPacket peerListPacket);
    void onDataSendComplete(boost::asio::const_buffers_1 buffer, const boost::system::error_code & error, size_t bytes_transferred);
    void trackAndUpdatePeers();
    void trackPeers();
    bool isAlive(DataDistPeer peer);
    void sendPeerlistsToPeers();

    // TODO: Constant organization task 
    static const int TRACK_PEERS_INTERVAL = 4;  // Use even number
    // TODO: After Constant organization task is done, use the constant originally defined in Peer plus some buffer
    //       The buffer is so that the peer has some time to stay alive even when packets fail
    static const int PEER_TIMEOUT_SECONDS = 5 * 2 + 1;
    static const int DEFAULT_SERVER_PORT = 30005;

    boost::asio::io_service io_service;

    std::unordered_map<std::string, std::pair<DataDistTracker, std::shared_ptr<std::mutex>>> dataDists;

    udp::socket socket;
    boost::asio::io_service threadpool_ioservice;
    boost::thread_group threadpool;
    std::thread processingThread;

    bool isDestructing;
    std::string metadataDirectory;
};


