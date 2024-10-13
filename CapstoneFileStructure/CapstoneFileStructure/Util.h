#pragma once

#include <string>
#include <memory>
#include <iostream>

#include "Metadata.h"

class Util
{
public:
    static std::string getCompletedFilesFilePath(std::string outDir, std::shared_ptr<Metadata> metadata);

    static void showException(std::exception e);
};

