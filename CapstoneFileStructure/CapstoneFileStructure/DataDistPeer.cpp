#include "DataDistPeer.h"
#include <string>

DataDistPeer::DataDistPeer(std::string host, uint16_t port, std::string localHost, uint16_t localPort, unsigned char progress)
    : host(host), port(port), localHost(localHost), localPort(localPort), progress(progress)
{
    using namespace std::chrono;
    endpoint = host + ":" + std::to_string(port);
    lastTimeAlive = std::chrono::system_clock::now();
}

DataDistPeer::DataDistPeer() : DataDistPeer("", 0, "", 0, 0)
{
}

DataDistPeer::~DataDistPeer()
{
}

std::string DataDistPeer::getHost() {
    return host;
}

std::uint16_t DataDistPeer::getPort() {
    return port;
}

std::uint16_t DataDistPeer::getLocalPort() {
    return localPort;
}

std::string DataDistPeer::getEndpoint() {
    return endpoint;
}

std::string DataDistPeer::getId() {
    return getEndpoint();
}

std::chrono::time_point<std::chrono::system_clock> DataDistPeer::getLastTimeAlive() {
    return lastTimeAlive;
}

std::string DataDistPeer::getLocalHost() {
    return localHost;
}

unsigned char DataDistPeer::getProgress() {
    return progress;
}