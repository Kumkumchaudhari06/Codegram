#pragma once
#include <string>

namespace Compression {
    std::string compress(const std::string& data);
    // Returns false if the data is not valid compressed data
    bool decompress(const std::string& packed, std::string& out);
}