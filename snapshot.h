#pragma once
#include <map>
#include <string>
#include <vector>
#include "core/types.h"
#include "storage/objectstore.h"

// A snapshot is the flat list of files in a commit: path -> blob hash
namespace Snapshot {
    typedef std::map<std::string, std::string> Entries;

    bool ofCommit(const ObjectStore& store, const std::string& commitHash, Entries& out);   // "" = empty
    std::string blobAt(const ObjectStore& store, const std::string& commitHash, const std::string& path);
    // Added / Modified / Deleted / Renamed (a deleted and an added file with the same content)
    std::vector<FileChange> compare(const Entries& before, const Entries& after, bool detectRenames = true);
}