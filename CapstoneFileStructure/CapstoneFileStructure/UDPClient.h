#ifndef __UDPClient_H
#define __UDPClient_H

#include <mutex>
#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "ChunkRequestPacket.h"
#include "PeerProgressPacket.h"
#include "UdpPacket.h"


using boost::asio::ip::udp;

class UDPClient
{
public:
    UDPClient(udp::socket * socket, std::mutex * socketMutex);
    UDPClient();
    ~UDPClient();

    void requestChunk(std::string metafileName, std::string filePath, 
                      int chunkNumber, const std::string & host, uint16_t port);

    void onDataSendComplete(boost::asio::const_buffers_1 buffer, std::string serializedPacket, const boost::system::error_code & error, size_t bytes_transferred);

    void notifyServer(const std::string & host, uint16_t port, PeerProgressPacket packet);

    std::string getLocalAddress();
    
    void sendProgress(std::string dataDistId, unsigned char progress, const std::string& trackerHost, uint16_t trackerPort, unsigned short peerPort);
    
    
private:
    udp::endpoint createEndPoint(const std::string & host, uint16_t port);

    std::string retrieveLocalAddress();


    std::string localAddress;
    boost::asio::io_service io_service_;
    udp::socket * socket_;
    std::mutex * socketMutex_;
};

#endif