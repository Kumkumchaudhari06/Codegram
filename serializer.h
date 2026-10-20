#pragma once
#include <string>
#include "storage/Objects.h"

namespace Serializer {
    std::string serializeTree(const Tree& tree);
    bool parseTree(const std::string& data, Tree& out);

    std::string serializeCommit(const Commit& c);
    bool parseCommit(const std::string& data, Commit& out);
}