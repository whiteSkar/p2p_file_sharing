#include <string>
#include "Metadata.h"

Metadata::Metadata()
{
    chunkSize = 0;
}

std::string Metadata::getRootDirectoryName()
{
    return rootDirectoryName;
}

uint64_t Metadata::getFileCount()
{
    return filePaths.size();
}

uint32_t Metadata::getChunkSize()
{
    return chunkSize;
}

std::string Metadata::getFilePath(uint64_t fileIndex)
{
    return getFileMetadatum(fileIndex).filePath;
}

uint64_t Metadata::getFileSize(std::string filePath)
{
    return getFileMetadatum(filePath).fileSize;
}

int Metadata::getChunkCountForFile(std::string filePath)
{
    return (int)ceil((float)getFileMetadatum(filePath).fileSize / chunkSize);
}

std::string Metadata::getHash(std::string filePath, uint32_t chunkIndex)
{
    FileMetadatum &fileMetadatum = getFileMetadatum(filePath);

    if (chunkIndex < 0 || chunkIndex >= fileMetadatum.hashes.size())
        throw std::invalid_argument("Argument out of range: " + std::to_string(chunkIndex));

    return fileMetadatum.hashes[chunkIndex];
}

std::vector<std::string>* Metadata::getHashesForFile(std::string filePath)
{
    return &getFileMetadatum(filePath).hashes;
}

void Metadata::setMetaFileName(std::string metaFileName)
{
    this->metaFileName = metaFileName;
}

std::string Metadata::getMetaFileName()
{
    return this->metaFileName;
}

void Metadata::setRootDirectoryName(std::string rootDirectoryName)
{
    this->rootDirectoryName = rootDirectoryName;
}

void Metadata::setChunkSize(uint32_t chunkSize)
{
    this->chunkSize = chunkSize;
}

void Metadata::addHashes(std::string filePath, std::vector<std::string> const &hashes)
{
    FileMetadatum &fileMetadatum = getFileMetadatum(filePath);
    fileMetadatum.hashes.insert(fileMetadatum.hashes.end(), hashes.begin(), hashes.end());
}

void Metadata::addFileMetadatum(std::string filePath, uint64_t fileSize, std::vector<std::string> &hashes)
{
    this->filePaths.push_back(filePath);
    this->fileMetadataMap.insert(std::pair<std::string, FileMetadatum>(filePath, FileMetadatum(filePath, fileSize, hashes)));
}

bool Metadata::isThisFilePartOfDataDist(std::string filePath)
{
    auto it = getFileMetadatumIt(filePath);
    if (it == fileMetadataMap.end())
        return false;
    return true;
}


/* Private Methods */

Metadata::FileMetadatum& Metadata::getFileMetadatum(uint64_t fileIndex)
{
    if (fileIndex >= filePaths.size())
        throw std::invalid_argument("fileIndex is bigger than fileMetadata size.");

    return getFileMetadatum(filePaths[(unsigned int)fileIndex]);
}

Metadata::FileMetadatum& Metadata::getFileMetadatum(std::string filePath)
{
    auto it = getFileMetadatumIt(filePath);
    if (it == fileMetadataMap.end())
        throw std::runtime_error(filePath + " is not part of this data dist");

    return it->second;
}

std::unordered_map<std::string, Metadata::FileMetadatum>::iterator Metadata::getFileMetadatumIt(std::string filePath)
{
    auto it = fileMetadataMap.end();

    int rootDirNameIndex = filePath.find(rootDirectoryName);
    if (rootDirNameIndex == std::string::npos)
        return it;

    // This is to remove the custom directory path infront of the root directory path of the torrent
    filePath = filePath.substr(rootDirNameIndex);

    it = fileMetadataMap.find(filePath);
    return it;
}