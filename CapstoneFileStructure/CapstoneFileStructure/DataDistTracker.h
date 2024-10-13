#include "Peer.h"

#pragma once

// An instance of the DataDist information on the server, keeps track of peers, etc.
class DataDistTracker
{
public:
    DataDistTracker();

    bool isSeederRegistered(DataDistPeer dataDistPeer);
    bool isLeecherRegistered(DataDistPeer dataDistPeer);

    void updateSeeder(DataDistPeer dataDistPeer);
    void updateLeecher(DataDistPeer dataDistPeer);

    void deleteSeeder(std::string dataDistPeerId);
    void deleteLeecher(std::string dataDistPeerId);

    std::vector<DataDistPeer> getSeeders();
    std::vector<DataDistPeer> getLeechers();

private:
    std::unordered_map<std::string, DataDistPeer> seeders;
    std::unordered_map<std::string, DataDistPeer> leechers;
};