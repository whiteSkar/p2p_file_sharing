#pragma once

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <cstdlib>
#include <iostream>
#include <mutex>

#include "ChunkRequestPacket.h"
#include "DirectoryHandler.h"
#include "DataDistPeer.h"
#include "FileHandler.h"
#include "MetadataHandler.h"
#include "PeerListPacket.h"
#include "UdpPacket.h"
#include "Util.h"

using boost::asio::ip::udp;

class UDPListener
{
public:
    static const std::chrono::microseconds RESPONSE_RATE_INTERVAL;
    UDPListener::UDPListener();
    UDPListener::UDPListener(boost::asio::ip::udp::socket *socket, 
                             std::mutex *socketMutex, 
                             std::string outputDirectory,
                             std::unordered_map<std::string, std::shared_ptr<Metadata>> *activeMetadata,
                             std::mutex *activeMetadataMutex,
                             std::unordered_map<std::string, DataDistPeer> *seeders,
                             std::unordered_map<std::string, DataDistPeer> *leechers,
                             std::mutex *peerListMutex);

    void run();
    void stop();
    
    void setChunksToReceive(std::unordered_map<std::string, std::unordered_map<uint32_t, bool>> *chunksToReceive,
                            std::mutex *chunksToReceiveMutex);

    void setMetadata(std::shared_ptr<Metadata> metadata);


private:
    void receive();

    void onDataReceive(std::shared_ptr<char> data, 
                       std::shared_ptr<udp::endpoint> sender_endpoint,
                       const boost::system::error_code &error, 
                       size_t bytes_recvd);

    void handleChunkRequest(std::shared_ptr<UdpPacket> udpPacket, 
                            std::shared_ptr<udp::endpoint> sender_endpoint);

    void handleFilePacket(std::shared_ptr<UdpPacket> udpPacket);

    void handlePeerListPacket(std::shared_ptr<UdpPacket> udpPacket);

    void processPacket(std::shared_ptr<UdpPacket> udpPacket,
                       std::shared_ptr<udp::endpoint> sender_endpoint);

    void showMsg(std::string msg);

    udp::socket *socket;
    std::mutex *socketMutex;

    std::string outputDirectory;

    int numPacketsToSend;
    boost::asio::io_service threadpool_ioservice;

    std::unordered_map<std::string, DataDistPeer> *seeders;
    std::unordered_map<std::string, DataDistPeer> *leechers;
    std::mutex *peerListMutex;

    std::unordered_map<std::string, std::unordered_map<uint32_t, bool>> *chunksToReceive;
    std::mutex *chunksToReceiveMutex;

    std::unique_ptr<FileHandler> completedFileHandler;

    std::shared_ptr<Metadata> metadata;

    std::unordered_map<std::string, std::shared_ptr<Metadata>> *activeMetadata;
    std::mutex *activeMetadataMutex;

    std::shared_ptr<DirectoryHandler> directoryHandlerForReceiving;
    std::unordered_map<std::string, std::shared_ptr<DirectoryHandler>> directoryHandlersForSending;
};