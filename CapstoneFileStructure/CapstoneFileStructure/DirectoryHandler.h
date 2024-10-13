#include <string>
#include <boost/filesystem.hpp>

#include "Chunk.h"
#include "FileHandler.h"
#include "Metadata.h"

#pragma once

class DirectoryHandler
{
public:
    DirectoryHandler(std::string directoryPath, std::shared_ptr<Metadata> metadata);

    std::unique_ptr<FileHandler> getNextFileHandler();
    bool hasNextFile();

    std::string getDirectoryPath();
    std::shared_ptr<Metadata> getMetadata();

    bool doesFileExist(std::string filePath);
    std::shared_ptr<FileHandler> getFile(std::string filePath, bool forReading);
    void closeFile(std::string filePath);

private:
    std::shared_ptr<Metadata> metadata;
    std::string directoryPath;

    std::string getAbsPath(std::string relPath);

    std::unordered_map<std::string, std::shared_ptr<FileHandler>> openFileHandlers;

    boost::filesystem::directory_iterator dirIter;
    boost::filesystem::directory_iterator endIter;

    std::unique_ptr<DirectoryHandler> nestedDirHandler;
};

