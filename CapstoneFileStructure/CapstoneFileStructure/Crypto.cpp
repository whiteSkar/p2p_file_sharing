#include "Crypto.h"
#include <boost/uuid/sha1.hpp>
#include <sstream>
#include <iomanip>


std::string Crypto::computeSHA1(const char* bytes, int size)
{
    boost::uuids::detail::sha1 sha1;
    unsigned int hash[5];
    sha1.process_bytes(bytes, size);
    sha1.get_digest(hash);

    std::stringstream ss;
    for (std::size_t i = 0; i < sizeof(hash) / sizeof(hash[0]); i++)
        ss << std::hex << std::setfill('0') << std::setw(sizeof(int) * 2) << hash[i];

    return ss.str();
}