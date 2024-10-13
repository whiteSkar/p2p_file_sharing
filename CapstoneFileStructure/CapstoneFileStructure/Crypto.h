#pragma once

#include <string>
#include <boost\shared_array.hpp>

class Crypto
{
public:
    static std::string Crypto::computeSHA1(const char* bytes, int size);
};

