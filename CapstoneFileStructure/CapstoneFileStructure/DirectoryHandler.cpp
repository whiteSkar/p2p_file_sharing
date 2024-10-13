#include "DirectoryHandler.h"

namespace fs = boost::filesystem;

// Pre: metadata's chunkSize must have been set
DirectoryHandler::DirectoryHandler(std::string directoryPath, std::shared_ptr<Metadata> metadata)
{
    this->metadata = metadata;
    this->directoryPath = directoryPath;
    nestedDirHandler = nullptr;

    fs::path dirPath = directoryPath;

    if (!fs::exists(dirPath))
    {
        if (!fs::create_directories(dirPath))
            throw std::runtime_error(dirPath.generic_string() + " couldn't be created.");
    }
    else if (!fs::is_directory(dirPath))
    {
        throw std::runtime_error(dirPath.generic_string() + " is not a directory.");
    }

    dirIter = fs::directory_iterator(dirPath);
}

bool DirectoryHandler::doesFileExist(std::string filePath)
{
    auto absPath = getAbsPath(filePath);
    return FileHandler::doesFileExist(absPath);
}

std::shared_ptr<FileHandler> DirectoryHandler::getFile(std::string filePath, bool forReading)
{
    auto absPath = getAbsPath(filePath);
    if (openFileHandlers.find(absPath) == openFileHandlers.end())
    {
        std::shared_ptr<FileHandler> file;
        if (forReading)
            file = std::shared_ptr<FileHandler>(
                new FileHandler(absPath, filePath, 
                                metadata->getChunkSize(), 0,
                                metadata->getHashesForFile(filePath)));
        else
            file = std::shared_ptr<FileHandler>(
                new FileHandler(absPath, filePath, metadata->getChunkSize(), 
                                metadata->getFileSize(absPath),
                                metadata->getHashesForFile(filePath)));

        openFileHandlers.insert(std::make_pair(absPath, file));
    }

    return openFileHandlers.at(absPath);
}

void DirectoryHandler::closeFile(std::string filePath)
{
    auto absPath = getAbsPath(filePath);
    auto it = openFileHandlers.find(absPath);
    if (it != openFileHandlers.end())
    {
        // If there are no more references to it, it should close by itself.
        openFileHandlers.erase(it);
    }
}

std::string DirectoryHandler::getAbsPath(std::string relPath)
{
    return directoryPath + "/" + relPath;
}

bool DirectoryHandler::hasNextFile()
{
    return dirIter != endIter;
}

std::string DirectoryHandler::getDirectoryPath()
{
    return directoryPath;
}

std::shared_ptr<Metadata> DirectoryHandler::getMetadata()
{
    return metadata;
}

// Pre: must be called only if hasNextFile() returned true
// Post: *partial documentation* returns nullptr if the next file points to empty directory
std::unique_ptr<FileHandler> DirectoryHandler::getNextFileHandler()
{
    std::unique_ptr<FileHandler> fh = nullptr;
    if (dirIter != endIter)
    {
        if (fs::is_directory(dirIter->status()))
        {
            if (!nestedDirHandler ||
                (nestedDirHandler->getDirectoryPath() != dirIter->path().generic_string() &&
                    nestedDirHandler->getDirectoryPath() != dirIter->path().string()))
                nestedDirHandler = std::unique_ptr<DirectoryHandler>(
                    new DirectoryHandler(dirIter->path().generic_string(), this->metadata));

            if (nestedDirHandler->hasNextFile())
                fh = nestedDirHandler->getNextFileHandler();

            if (!nestedDirHandler->hasNextFile())
                dirIter++;
        }
        else
        {
            auto filePath = dirIter->path().generic_string();

            auto hashes = metadata->isThisFilePartOfDataDist(filePath) 
                ? metadata->getHashesForFile(filePath)
                : nullptr;

            fh = std::unique_ptr<FileHandler>(
                new FileHandler(filePath, filePath, 
                                this->metadata->getChunkSize(), 0,
                                hashes));
            dirIter++;
        }
    }

    return move(fh);
}



