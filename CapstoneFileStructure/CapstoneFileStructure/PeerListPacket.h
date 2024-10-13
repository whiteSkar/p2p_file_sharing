#ifndef __PeerListPacket_H
#define __PeerListPacket_H
#include "UdpPacket.h"
#include "DataDistPeer.h"
#include <boost/algorithm/string.hpp>

class PeerListPacket : public UdpPacket
{
public:
    PeerListPacket();
    PeerListPacket(UdpPacket & udpPacket);
    PeerListPacket(std::string peerEndpoint, std::vector<DataDistPeer> peers, bool peersAreSeeders);
    bool getAreSeeders();
    std::string getPeerEndpoint();
    DataDistPeer getPeerInfo();
    std::vector<DataDistPeer> getPeers();
    
private:
    void parsePacketData();
    static const char DELIMITER = ';';
    
    std::string peerEndpoint;
    DataDistPeer peerInfo;
    bool areSeeders;
    std::vector<DataDistPeer> peers;
    
};

#endif