#include "UDPClient.h"

UDPClient::UDPClient(udp::socket * socket, std::mutex * socketMutex)
{
    if (!socket || !socketMutex) {
        throw new std::exception("socekt or socketMutex is null");
    }

    socket_ = socket;
    socketMutex_ = socketMutex;
    localAddress = retrieveLocalAddress();
}

UDPClient::UDPClient()
{
}

UDPClient::~UDPClient()
{
}

udp::endpoint UDPClient::createEndPoint(const std::string& host, uint16_t port)
{
    return udp::endpoint(boost::asio::ip::address::from_string(host), port);
}


/* Side effecting call, only call once.
 * Creates socket that queries and resolves the endpoints of that address, then connects to that endpoint
 * this ensures that local_endpoint returns the local ip of the network adapter used.
*/
std::string UDPClient::retrieveLocalAddress()
{
    
    boost::asio::io_service netService;
    udp::resolver resolver(netService);
    udp::resolver::query query(udp::v4(), "google.com", "");
    udp::resolver::iterator endpoints = resolver.resolve(query);
    udp::endpoint ep = *endpoints;
    udp::socket socket(netService);
    socket.connect(ep);
    boost::asio::ip::address addr = socket.local_endpoint().address();
    return addr.to_string();
}

std::string UDPClient::getLocalAddress()
{
    return localAddress;
}

void UDPClient::requestChunk(std::string metafileName, std::string filePath, 
                             int chunkNumber, const std::string& host, uint16_t port)
{
    auto endpoint = createEndPoint(host, port);

    auto packet = ChunkRequestPacket::ChunkRequestPacket(0, metafileName, filePath, chunkNumber);
    auto serializedPacket = packet.serializePacket();
    auto buffer = boost::asio::const_buffers_1(serializedPacket.data(), serializedPacket.length());

    std::lock_guard<std::mutex> lock(*socketMutex_);
    socket_->send_to(buffer, endpoint);
}

void UDPClient::onDataSendComplete(boost::asio::const_buffers_1 buffer, std::string serializedPacket,
    const boost::system::error_code &error, size_t bytes_transferred)
{
    if (!error && bytes_transferred > 0)
    {
        return;
    }

    std::string str = "UDPClient::onDataSendComplete ";
    if (error) {
        str += "got error : " + error.message() + "\n";
    } else {
        str += "transferred 0 bytes.\n";
    }

    std::cout << str;
}

void UDPClient::notifyServer(const std::string& host, uint16_t port, PeerProgressPacket packet) {
    auto dataDistServerEndpoint = createEndPoint(host, port);
    auto serializedPacket = packet.serializePacket();
    auto buffer = boost::asio::const_buffers_1(serializedPacket.data(), serializedPacket.length());
    
    std::lock_guard<std::mutex> lock(*socketMutex_);    // BTW whoever created socketMutex, is there a reason why this is a pointer?
    auto localPort = socket_->local_endpoint().port();
    std::string str = std::to_string(localPort) + " is notifying the server with datadist id "
                      + packet.getDataDistId() + " with progress "
                      + std::to_string(packet.getProgress()) + "\n";
    std::cout << str;

    socket_->async_send_to(buffer, dataDistServerEndpoint,
        boost::bind(&UDPClient::onDataSendComplete, this, buffer, serializedPacket,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void UDPClient::sendProgress(std::string dataDistId, unsigned char progress, const std::string& trackerHost, uint16_t trackerPort, unsigned short peerPort) {
    auto packet = PeerProgressPacket::PeerProgressPacket(dataDistId, progress, localAddress, peerPort);
    notifyServer(trackerHost, trackerPort, std::move(packet));
}
