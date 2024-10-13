#include "PeerListPacket.h"

PeerListPacket::PeerListPacket()
{
}

PeerListPacket::PeerListPacket(UdpPacket &udpPacket) {
    packetHeader = PacketHeader(udpPacket.getPacketNumber(), PacketHeader::PacketType::PeerList);
    packetData = udpPacket.getPacketData();
    parsePacketData();
}


PeerListPacket::PeerListPacket(std::string peerEndpoint, std::vector<DataDistPeer> peers, bool peersAreSeeders) : peers(peers), areSeeders(peersAreSeeders)
{
    packetHeader = PacketHeader(PacketHeader::PacketType::PeerList);
    std::random_shuffle(peers.begin(), peers.end());
    packetData = "";
    (peersAreSeeders) ? packetData += 's' : packetData += 'l';
    packetData += peerEndpoint + DELIMITER;
    for (auto peer : peers) {
        auto packetDataRatioToMaxSize = 0.9f;
        if (packetData.length() <= UdpPacket::MAX_SIZE * packetDataRatioToMaxSize){
            //Adding local ip first and then public ip
            packetData += peer.getLocalHost() + ":" + std::to_string(peer.getLocalPort()) + DELIMITER;
            packetData += peer.getEndpoint() + DELIMITER;
        }
    }
}

void PeerListPacket::parsePacketData() {
    areSeeders = (packetData[0] == 's');
    std::vector<std::string> hosts;
    std::vector<DataDistPeer> peerList;

    try {
        boost::split(hosts, packetData.substr(1, packetData.find_last_of(';') - 1), boost::is_any_of(";"));

        // Peer's own ip
        auto index = hosts[0].find_first_of(':');
        peerList.emplace_back(hosts[0].substr(0, index), std::stoi(hosts[0].substr(index + 1)), "", 0);

        // Incrementing by 2 due to each peer having a local and public IP.
        for (int i = 1; i < hosts.size(); i += 2) {
            auto localhostIndex = hosts[i].find_first_of(':');
            auto index = hosts[i + 1].find_first_of(':');
            auto hostAddress = hosts[i + 1].substr(0, index);
            auto hostPort = std::stoi(hosts[i + 1].substr(index + 1));
            auto localHostAddress = hosts[i].substr(0, localhostIndex);
            auto localHostPort = std::stoi(hosts[i].substr(localhostIndex + 1));
            peerList.emplace_back(hostAddress, hostPort, localHostAddress, localHostPort);
        }
        peers = peerList;
        peerInfo = peerList.front();
        peerEndpoint = peerList.front().getEndpoint();
    } catch (std::runtime_error& e) {
        peers = std::vector<DataDistPeer>();
        peerEndpoint = "";
    }
}

bool PeerListPacket::getAreSeeders()
{
    return areSeeders;
}

DataDistPeer PeerListPacket::getPeerInfo()
{
    return peerInfo;
}

std::string PeerListPacket::getPeerEndpoint()
{
    return peerEndpoint;
}

std::vector<DataDistPeer> PeerListPacket::getPeers()
{
    return peers;
}
