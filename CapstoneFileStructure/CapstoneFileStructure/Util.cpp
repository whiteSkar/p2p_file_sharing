#include "Util.h"

std::string Util::getCompletedFilesFilePath(std::string outDir, std::shared_ptr<Metadata> metadata)
{
    std::string metaFileName = metadata->getMetaFileName();
    int indexOfDotExt = metaFileName.find_first_of('.');
    if (indexOfDotExt == std::string::npos) {
        throw std::invalid_argument("metadata file name is invalid");
    }

    metaFileName = metaFileName.substr(0, indexOfDotExt);
    return outDir + "/" + metaFileName + ".completed";
}

void Util::showException(std::exception e) {
    std::string str = "EXCEPTION: " + std::string(e.what()) + "\n";
    std::cout << str;
}