#ifndef __DataDistPeer_H
#define __DataDistPeer_H

#include <cstdint>
#include <iostream>
#include <chrono>

class DataDistPeer
{
public:
    DataDistPeer(std::string host, uint16_t port, std::string localHost, uint16_t localPort, unsigned char progress = 0);
    DataDistPeer();
    ~DataDistPeer();

    std::string getHost();
    std::uint16_t getPort();
    std::uint16_t getLocalPort();
    std::string getEndpoint();
    std::string getId();
    std::string  getLocalHost();
    unsigned char getProgress();

    std::chrono::time_point<std::chrono::system_clock> getLastTimeAlive();
      

private:
    std::string host;
    std::string endpoint;
    std::uint16_t port, localPort;
    std::string localHost;
    unsigned char progress = 0;

    std::chrono::time_point<std::chrono::system_clock> lastTimeAlive;
};

#endif

