#include "DataDistTracker.h"

DataDistTracker::DataDistTracker()
{
}

bool DataDistTracker::isSeederRegistered(DataDistPeer dataDistPeer) {
    return seeders.find(dataDistPeer.getId()) != seeders.end();
}

bool DataDistTracker::isLeecherRegistered(DataDistPeer dataDistPeer) {
    return leechers.find(dataDistPeer.getId()) != leechers.end();
}

void DataDistTracker::updateSeeder(DataDistPeer dataDistPeer) {
    seeders[dataDistPeer.getId()] = dataDistPeer;
}

void DataDistTracker::updateLeecher(DataDistPeer dataDistPeer) {
    leechers[dataDistPeer.getId()] = dataDistPeer;
}

void DataDistTracker::deleteSeeder(std::string dataDistPeerId) {
    seeders.erase(dataDistPeerId);
}

void DataDistTracker::deleteLeecher(std::string dataDistPeerId) {
    leechers.erase(dataDistPeerId);
}

std::vector<DataDistPeer> DataDistTracker::getSeeders()
{
    std::vector<DataDistPeer> seedersList;
    seedersList.reserve(seeders.size());
    for (auto kv : seeders) {
        seedersList.push_back(kv.second);
    }
    return seedersList;
}

std::vector<DataDistPeer> DataDistTracker::getLeechers()
{
    std::vector<DataDistPeer> leechersList;
    leechersList.reserve(seeders.size());
    for (auto kv : leechers) {
        leechersList.push_back(kv.second);
    }
    return leechersList;
}

