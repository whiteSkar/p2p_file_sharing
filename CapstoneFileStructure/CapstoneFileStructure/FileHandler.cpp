#include <iostream>
#include <numeric>

#include "Crypto.h"
#include "FileHandler.h"

namespace fs = boost::filesystem;

// Pre: filePath needs to be a platform independant path
bool FileHandler::doesFileExist(std::string filePath)
{
    auto path = fs::path(filePath);
    if (fs::exists(path) && !fs::is_directory(path))
        return true;

    return false;
}

// For generic files
FileHandler::FileHandler(std::string absPath, bool isForReading)
{
    this->absPath = fs::path(absPath);
    hashes = nullptr;
    if (isForReading)
    {
        if (!doesFileExist(this->absPath.generic_string()))
            throw std::runtime_error(this->absPath.generic_string() + " does not exist.");

        stdInstream = std::unique_ptr<std::ifstream>(
            new std::ifstream(this->absPath.generic_string(), std::ios_base::in | std::ios_base::binary));

        _fileSize = fs::file_size(this->absPath);
        outstream = nullptr;
        instream = nullptr;
    }
    else
    {
        instream = nullptr;

        if (!doesFileExist(this->absPath.generic_string()) && !fs::is_directory(this->absPath.parent_path()))
        {
            if (!fs::create_directories(this->absPath.parent_path()))
                throw std::runtime_error("Failed to create directories in path: " + this->absPath.parent_path().generic_string());
        }

        outstream = std::unique_ptr<fs::fstream>(new fs::fstream(this->absPath, std::ios::binary | std::ios::app));
        _fileSize = fs::file_size(this->absPath);
    }
}

// For data dist files that needs to be chunked
// if fileSize is not set, it's a reader. Otherwise, it's a writer
FileHandler::FileHandler(std::string absPath, std::string relPath,
                         int chunkSize, uint64_t fileSize,
                         std::vector<std::string>* hashes)
{
    this->absPath = fs::path(absPath);
    this->relPath = fs::path(relPath);
    _chunkSize = chunkSize;
    _fileSize = fileSize;
    this->hashes = hashes;

    if (fileSize == 0)
    {
        if (!doesFileExist(this->absPath.generic_string()))
            throw std::runtime_error(this->absPath.generic_string() + " does not exist.");

        instream = std::unique_ptr<fs::fstream>(new fs::fstream(this->absPath, std::ios::binary | std::ios::in));
        _fileSize = fs::file_size(this->absPath);
        outstream = nullptr;
    }
    else
    {
        instream = nullptr;

        if (!fs::is_directory(this->absPath.parent_path())) {
            if (!fs::create_directories(this->absPath.parent_path())) {
                throw std::runtime_error("Failed to create directories in path: " + this->absPath.parent_path().generic_string());
            }
        }

        if (doesFileExist(this->absPath.generic_string()))
            outstream = std::unique_ptr<fs::fstream>(new fs::fstream(this->absPath, std::ios::binary | std::ios::in | std::ios::out));
        else
            outstream = std::unique_ptr<fs::fstream>(new fs::fstream(this->absPath, std::ios::binary | std::ios::out));

        if (fs::file_size(this->absPath) != fileSize)
            allocateFileOnDisk();

        _chunkStatuses = std::unique_ptr<boost::dynamic_bitset<>>(
            new boost::dynamic_bitset<>(getNumberOfChunks()));
        _chunkStatuses->set();
    }
}

FileHandler::~FileHandler(void)
{
    close();
}

void FileHandler::close()
{
    if (instream)
        instream->close();
    if (outstream)
        outstream->close();
    if (stdInstream)
        stdInstream->close();
}

// This method should be called right after constructing FileHandler object
//  if the FileHandler object is for receiving a data dist file
void FileHandler::setRequiredChunkIndicies(std::vector<uint32_t> &indicies)
{
    _chunkStatuses = std::unique_ptr<boost::dynamic_bitset<>>(new boost::dynamic_bitset<>(getNumberOfChunks()));
    _chunkStatuses->set();
    for (auto &i : indicies)
    {
        _chunkStatuses->operator[](i) = false; // 1 is complete 0 is required 
    }
}

uint32_t FileHandler::getRequiredChunkCount()
{
    return _chunkStatuses->size() - _chunkStatuses->count();
}

int FileHandler::getNumberOfActiveChunks()
{
    return _activeChunks.size();
}

std::shared_ptr<Chunk> FileHandler::getChunkForSending(uint32_t chunkNum)
{
    int activeChunkIndex = getActiveChunkIndex(chunkNum);
    if (activeChunkIndex != -1)
        return _activeChunks[activeChunkIndex];

    auto chunk = getChunk(chunkNum);
    _activeChunks.push_back(chunk);
    return chunk;
}

// Pre: chunkNum must be valid
std::shared_ptr<Chunk> FileHandler::getChunk(int chunkNum, bool shouldCheckHash)
{
    int chunkSize = _chunkSize;
    int chunkIndex = chunkNum * chunkSize;
    if (chunkSize >= _fileSize)
        chunkSize = (int)_fileSize;
    else if (chunkIndex + _chunkSize > _fileSize) // for last chunk
        chunkSize = (int)getFileSize() - chunkIndex;

    auto data = std::unique_ptr<unsigned char[]>(new unsigned char[chunkSize]);
    readInData(data.get(), chunkIndex, chunkSize);

    std::string dataString((char*)data.get(), chunkSize);

    std::string hash = shouldCheckHash ? hashes->at(chunkNum) : "";
    auto chunk = std::shared_ptr<Chunk>(new Chunk(dataString, chunkNum,
                                        relPath.generic_string(), hash));
    return chunk;
}

std::vector<uint32_t> FileHandler::getIncompleteChunkIndicies()
{
    std::vector<uint32_t> indicies;

    for (int i = 0; i < getNumberOfChunks(); i++) {
        if (!getChunk(i)->isComplete()) {
            indicies.push_back(i);
        }
    }

    return indicies;
}

// Pre: chunkNum must be valid
std::shared_ptr<Chunk> FileHandler::createChunkForReceiving(uint32_t chunkNum)
{
    int activeChunkIndex = getActiveChunkIndex(chunkNum);
    if (activeChunkIndex != -1)
        return _activeChunks[activeChunkIndex];

    int chunkSize = _chunkSize;
    int chunkIndex = chunkNum * chunkSize;
    if (chunkSize >= _fileSize)
        chunkSize = (int)_fileSize;
    else if (chunkIndex + _chunkSize > _fileSize) // for last chunk
        chunkSize = (int)_fileSize - chunkIndex;

    auto chunk = std::shared_ptr<Chunk>(new Chunk(chunkSize, chunkNum,
                                        this->relPath.generic_string(),
                                        hashes->at(chunkNum)));
    _activeChunks.push_back(chunk);

    return chunk;
}

void FileHandler::writeReceivedChunk(std::shared_ptr<Chunk> chunk)
{
    if (!outstream)
        throw std::runtime_error("Should not write data using a reading file.");

    outstream->seekp(chunk->getChunkNum() * _chunkSize, std::ios::beg);
    outstream->write(chunk->getData().data(), chunk->getDataSize());
    outstream->flush();

    removeFromRequiredChunks(chunk);
    removeFromActiveChunks(chunk);
}

void FileHandler::writeLine(std::string line)
{
    if (!outstream)
        throw std::runtime_error("Should not write data using a reading file.");

    outstream->write(line.data(), line.size());
    outstream->write("\n", 1);
    outstream->flush();
}

std::string FileHandler::readLine()
{
    std::string line;

    if (stdInstream)
        std::getline(*stdInstream.get(), line);

    return line;
}

bool FileHandler::isComplete()
{
    return _chunkStatuses->all();
}

std::string FileHandler::getProgressString()
{
    auto size = _chunkStatuses->size();
    std::string s;
    s.resize(size);
    for (size_t i = 0; i < size; i++)
    {
        if (_chunkStatuses->operator[](i))
            s[i] = '1';
        else if (getActiveChunkIndex(i) != -1)
            s[i] = 'A';
        else
            s[i] = '0';
    }
    return s;
}


void FileHandler::removeFromRequiredChunks(std::shared_ptr<Chunk> chunk)
{
    _chunkStatuses->set(chunk->getChunkNum());
}

int FileHandler::getNumberOfChunks()
{
    return (int)ceil((double)getFileSize() / (double)getChunkSize());
}

void FileHandler::setFileSize(uintmax_t fileSize)
{
    _fileSize = fileSize;
}


/* Getters */

std::string FileHandler::getAbsPath()
{
    return this->absPath.generic_string();
}

int FileHandler::getChunkSize()
{
    return _chunkSize;
}

uint64_t FileHandler::getFileSize()
{
    return _fileSize;
}


/* Private Functions */

void FileHandler::removeFromActiveChunks(std::shared_ptr<Chunk> chunk)
{
    for (unsigned int i = 0; i < _activeChunks.size(); i++)
    {
        if (chunk->getChunkNum() == _activeChunks[i]->getChunkNum())
        {
            _activeChunks.erase(_activeChunks.begin() + i);
            //std::cout << "Removing chunk " << chunk->getChunkNum() << " from active chunks" << std::endl;
            break;
        }
    }
}


int FileHandler::getActiveChunkIndex(uint32_t chunkNum)
{
    for (unsigned int i = 0; i < _activeChunks.size(); i++)
    {
        if (_activeChunks[i] && _activeChunks[i]->getChunkNum() == chunkNum)
            return i;
    }
    return -1;
}

void FileHandler::readInData(unsigned char* data, int index, uint64_t dataSize)
{
    if (!instream)
        throw std::runtime_error("Should not read in data using a writing file.");

    instream->seekg(index, std::ios::beg);
    instream->read((char*)data, dataSize);
}

bool FileHandler::isChunkRequired(uint32_t chunkNum)
{
    if (getActiveChunkIndex(chunkNum) != -1) //chunk is being downloaded (questionable definition of requied)
        return false;
    return !_chunkStatuses->test(chunkNum);
}

void FileHandler::allocateFileOnDisk() 
{
    outstream->seekp(getFileSize() - 1);
    outstream->write("E", 1);
    outstream->flush();
}