#ifndef __PeerRegistrationPacket_H
#define __PeerRegistrationPacket_H

#include "UdpPacket.h"

class PeerProgressPacket: public UdpPacket
{
public:
    PeerProgressPacket(UdpPacket & udpPacket);
    PeerProgressPacket(std::string dataDistId, unsigned char progress, std::string localHost, unsigned short localPort);
    unsigned char getProgress();
    std::string getDataDistId();

    std::string getLocalAddress();

    unsigned short getLocalPort();

protected:
    
    void parsePacketData();
    std::string localHost;
    std::string dataDistId;
    unsigned char progress;
    unsigned short localPort;

    // TODO: Constant organization task 
    const static int LENGTH_OFFSET = 1;
};

#endif