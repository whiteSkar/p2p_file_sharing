#include <cstdint>
#include <vector>
#include <unordered_map>

#pragma once

class Metadata
{
public:
    Metadata();

    std::string getMetaFileName();
    std::string getRootDirectoryName();
    uint64_t getFileCount();
    uint32_t getChunkSize();

    std::string getFilePath(uint64_t fileIndex);
    uint64_t getFileSize(std::string filePath);
    int getChunkCountForFile(std::string filePath);
    std::string getHash(std::string filePath, uint32_t chunkIndex);
    std::vector<std::string>* getHashesForFile(std::string filePath);

    void setMetaFileName(std::string metaFileName);
    void setRootDirectoryName(std::string rootDirectoryName);
    void setChunkSize(uint32_t chunkSize);

    void addHashes(std::string filePath, std::vector<std::string> const &hashes);
    void addFileMetadatum(std::string filePath, uint64_t fileSize, std::vector<std::string> &hashes);

    bool isThisFilePartOfDataDist(std::string filePath);

private:
    typedef struct FileMetadatum
    {
        std::string filePath;
        uint64_t fileSize;
        std::vector<std::string> hashes;

        FileMetadatum(std::string filePath, uint64_t fileSize, std::vector<std::string> &hashes)
        {
            this->filePath = filePath;
            this->fileSize = fileSize;
            this->hashes = hashes;
        }
    } FileMetadatum;

    FileMetadatum& getFileMetadatum(uint64_t fileIndex);
    FileMetadatum& getFileMetadatum(std::string filePath);
    std::unordered_map<std::string, FileMetadatum>::iterator getFileMetadatumIt(std::string filePath);

    std::string metaFileName;
    std::string rootDirectoryName;

    std::vector<std::string> filePaths;  // needs to be a datastructure that keeps insertion order
    std::unordered_map<std::string, FileMetadatum> fileMetadataMap;

    uint32_t chunkSize;
};
