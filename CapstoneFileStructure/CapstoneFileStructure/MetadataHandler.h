#pragma once

#include <memory>
#include "Metadata.h"

class MetadataHandler
{
public:
    static const std::string ROOT_DIR_KEY;
    static const std::string CHUNK_SIZE_KEY;
    static const std::string FILE_PATH_KEY;
    static const std::string FILE_SIZE_KEY;
    static const std::string META_FILE_NAME_KEY;
    static const std::string HASHES_KEY;

    static std::shared_ptr<Metadata> getMetadata(std::string metaFilePath);
    static void createMetadataFile(std::string dataDistPath, int chunkSize);

private:
    static void createMetadataFileHelper(std::shared_ptr<Metadata> metadata, std::string metaFilePath);
};
