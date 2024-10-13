#pragma once

#include <fstream>
#include <string>
#include "Crypto.h"
#include "DirectoryHandler.h"
#include "MetadataHandler.h"
#include "FileHandler.h"

void throwWrongFormatFileException(std::string fileName, std::string message);

const std::string MetadataHandler::META_FILE_NAME_KEY = "meta_file_name";
const std::string MetadataHandler::ROOT_DIR_KEY = "name";
const std::string MetadataHandler::CHUNK_SIZE_KEY = "chunk_size";
const std::string MetadataHandler::FILE_PATH_KEY = "file_path";
const std::string MetadataHandler::FILE_SIZE_KEY = "file_size";
const std::string MetadataHandler::HASHES_KEY = "hashes";

std::shared_ptr<Metadata> MetadataHandler::getMetadata(std::string metaFilePath)
{
    std::ifstream file(metaFilePath);
    std::string line;

    if (file.is_open())
    {
        std::shared_ptr<Metadata> metadata(new Metadata());

        while (getline(file, line))
        {
            auto indexOfSeparator = line.find_first_of(":");
            if (indexOfSeparator == std::string::npos)
                throwWrongFormatFileException(metaFilePath, ": is not found.");

            std::string key = line.substr(0, indexOfSeparator);
            std::string value = line.substr(indexOfSeparator + 1);

            if (key.empty() || value.empty())
                throwWrongFormatFileException(metaFilePath, "key or value cannot be found.");

            if (key == META_FILE_NAME_KEY)
            {
                metadata->setMetaFileName(value);
            }
            else if (key == ROOT_DIR_KEY)
            {
                metadata->setRootDirectoryName(value);
            }
            else if (key == CHUNK_SIZE_KEY)
            {
                metadata->setChunkSize(stoi(value));
            }
            else if (key == FILE_PATH_KEY)
            {
                indexOfSeparator = value.find_first_of(",");
                if (indexOfSeparator == std::string::npos)
                    throwWrongFormatFileException(metaFilePath, "',' is not found for file path");

                std::string pathValue = value.substr(0, indexOfSeparator);
                std::string fileSizePair = value.substr(indexOfSeparator + 1);

                indexOfSeparator = fileSizePair.find_first_of(":");
                if (indexOfSeparator == std::string::npos)
                    throwWrongFormatFileException(metaFilePath, "':' is not found for file size pair");

                std::string sizeKey = fileSizePair.substr(0, indexOfSeparator);
                std::string sizeValue = fileSizePair.substr(indexOfSeparator + 1);

                if (sizeKey.empty() || sizeValue.empty())
                    throwWrongFormatFileException(metaFilePath, "file size key or file size value cannot be found.");

                metadata->addFileMetadatum(pathValue, stoull(sizeValue), std::vector<std::string>());
            }
            else if (key == HASHES_KEY)
            {
                const int LENGTH_OF_SHA1 = 40;

                if (metadata->getFileCount() == 0 || metadata->getChunkSize() == 0)
                    throwWrongFormatFileException(metaFilePath, "File count or Chunk count is 0.");

                int expectedHashCount = 0;
                for (unsigned int i = 0; i < metadata->getFileCount(); i++)
                {
                    auto filePath = metadata->getFilePath(i);
                    expectedHashCount += (int)ceil((double)metadata->getFileSize(filePath) / metadata->getChunkSize());
                }

                int actualHashCount = value.size() / LENGTH_OF_SHA1;

                if (actualHashCount != expectedHashCount || value.size() % LENGTH_OF_SHA1 != 0)
                    throwWrongFormatFileException(metaFilePath, "number of length of actual hashes is wrong.");

                int numChunks = 0;
                for (unsigned int i = 0; i < metadata->getFileCount(); i++)
                {
                    auto filePath = metadata->getFilePath(i);
                    int chunkCount = metadata->getChunkCountForFile(filePath);

                    std::vector<std::string> hashes;
                    for (int j = 0; j < chunkCount; j++)
                    {
                        int startIndex = numChunks * LENGTH_OF_SHA1;
                        hashes.push_back(value.substr(startIndex, LENGTH_OF_SHA1));
                        numChunks++;
                    }

                    metadata->addHashes(filePath, hashes);
                }
            }
        }

        file.close();
        return metadata;
    }

    throw std::runtime_error(metaFilePath + " couldn't be opened.");
}

// Pre: metaFilePath is a platform independant path
void MetadataHandler::createMetadataFile(std::string dataDistPath, int chunkSize)
{
    boost::filesystem::path dirPath = dataDistPath;

    if (!boost::filesystem::exists(dirPath) || !boost::filesystem::is_directory(dirPath))
    {
        throw std::runtime_error(dirPath.generic_string() + " doesn't exist.");
    }

    auto metadata = std::shared_ptr<Metadata>(new Metadata());
    // TODO: When Constant organization task is done, use that constant for ".meta"
    std::string metaFilePath = dataDistPath + ".meta";
    metadata->setMetaFileName(boost::filesystem::path(metaFilePath).filename().generic_string());
    metadata->setRootDirectoryName(dirPath.filename().generic_string());
    metadata->setChunkSize(chunkSize);

    DirectoryHandler directoryHandler(dataDistPath, metadata);
    while (directoryHandler.hasNextFile())
    {
        std::unique_ptr<FileHandler> fh = directoryHandler.getNextFileHandler();
        if (fh)
        {
            std::vector<std::string> hashes;
            for (int i = 0; i < fh->getNumberOfChunks(); i++)
            {
                std::shared_ptr<Chunk> chunk = fh->getChunk(i, false);
                std::string sha1Val = Crypto::computeSHA1(chunk->getData().data(), chunk->getDataSize());
                hashes.push_back(sha1Val);

            }

            // Metadata should specify relative file path from the data dist path
            std::string dataDistParentpath = dirPath.parent_path().generic_string();
            std::string fileAbsPath = fh->getAbsPath();
            std::string fileRelPath = fileAbsPath;
            if (dataDistParentpath != "" && fileAbsPath.find(dataDistParentpath) == 0) {
                fileRelPath = fileAbsPath.substr(dataDistParentpath.size() + 1);
            }
            metadata->addFileMetadatum(fileRelPath, fh->getFileSize(), hashes);
        }
    }

    createMetadataFileHelper(metadata, metaFilePath);
}

void MetadataHandler::createMetadataFileHelper(std::shared_ptr<Metadata> metadata, std::string metaFilePath)
{
    if (metadata->getFileCount() == 0 || metadata->getChunkSize() == 0)
        throw std::runtime_error("metadata object isn't fully populated.");

    std::ofstream metaFile(metaFilePath);
    if (metaFile.is_open())
    {
        metaFile << META_FILE_NAME_KEY << ":" << metadata->getMetaFileName() << std::endl;
        metaFile << ROOT_DIR_KEY << ":" << metadata->getRootDirectoryName() << std::endl;
        metaFile << CHUNK_SIZE_KEY << ":" << metadata->getChunkSize() << std::endl;

        for (unsigned int i = 0; i < metadata->getFileCount(); i++)
        {
            auto filePath = metadata->getFilePath(i);
            metaFile << FILE_PATH_KEY << ":" << filePath << ","; // file path can't containt a comma right?
            metaFile << FILE_SIZE_KEY << ":" << metadata->getFileSize(filePath) << std::endl;
        }

        metaFile << HASHES_KEY << ":";
        for (unsigned int i = 0; i < metadata->getFileCount(); i++)
        {
            auto filePath = metadata->getFilePath(i);
            for (int i = 0; i < metadata->getChunkCountForFile(filePath); i++)
                metaFile << metadata->getHash(filePath, i);
        }

        metaFile.close();
    }
    else
    {
        throw std::runtime_error(metaFilePath + " couldn't be opened.");
    }
}


void throwWrongFormatFileException(std::string fileName, std::string message)
{
    throw std::runtime_error(fileName + " is in wrong format.\n" + message + "\n");
}