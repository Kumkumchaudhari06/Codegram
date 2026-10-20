#pragma once
#include <string>

namespace Sha256 {
    // Returns the 64-character lowercase hex SHA-256 of the data
    std::string hash(const std::string& data);
}