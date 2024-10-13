#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp> 
#include <boost/dynamic_bitset.hpp>

#include "Chunk.h"

#pragma once

class FileHandler
{
public:

    static bool doesFileExist(std::string filePath);

    FileHandler(std::string path, bool isForReading);
    FileHandler(std::string absPath, std::string relPath, 
                int chunkSize, uint64_t fileSize,
                std::vector<std::string>* hashes);

    ~FileHandler(void);

    void close();

    void setRequiredChunkIndicies(std::vector<uint32_t> &indicies);
    uint32_t getRequiredChunkCount();

    std::shared_ptr<Chunk> getChunkForSending(uint32_t chunkNum);
    std::shared_ptr<Chunk> createChunkForReceiving(uint32_t chunkNum);

    //should only be called for hashing 
    std::shared_ptr<Chunk> getChunk(int number, bool shouldCheckHash = true);

    std::vector<uint32_t> getIncompleteChunkIndicies();

    int getNumberOfActiveChunks();

    void writeReceivedChunk(std::shared_ptr<Chunk> chunk);
    void removeFromActiveChunks(std::shared_ptr<Chunk> chunk);

    void writeLine(std::string);
    std::string readLine();

    int getChunkSize();
    uint64_t getFileSize();  // size_t would be better
    std::string getAbsPath();
    bool isComplete();

    int getNumberOfChunks();

    void setFileSize(uintmax_t fileSize);

    //Mainly for demoing. However could be used to update a graphic.
    std::string getProgressString();

private:
    boost::filesystem::path absPath;
    boost::filesystem::path relPath;
    int _chunkSize;
    uint64_t _fileSize;
    std::unique_ptr<boost::dynamic_bitset<>> _chunkStatuses;

    std::vector<std::shared_ptr<Chunk>> _activeChunks;

    std::unique_ptr<boost::filesystem::fstream> outstream;
    std::unique_ptr<boost::filesystem::fstream> instream;
    std::unique_ptr<std::ifstream> stdInstream;

    std::vector<std::string>* hashes;

    void removeFromRequiredChunks(std::shared_ptr<Chunk> chunk);

    bool isChunkRequired(uint32_t chunkNum);

    void readInData(unsigned char* data, int index, uint64_t dataSize);
    int getActiveChunkIndex(uint32_t chunkNum);

    void allocateFileOnDisk();
};
