#pragma once
#include <string>
#include <vector>
#include "core/snapshot.h"
#include "core/types.h"
#include "storage/storage.h"

// Works with the project folder: scanning, ignore rules, status, writing files
class FileTracker {
public:
    FileTracker(const std::string& root, Storage& storage);

    // The blob hash a file with this content would get (nothing is stored)
    static std::string hashContent(const std::string& content);

    Snapshot::Entries scan() const;                       // every non-ignored file: path -> blob hash
    bool isIgnored(const std::string& path) const;        // rules come from .codegramignore
    bool exists(const std::string& path) const;
    bool readFile(const std::string& path, std::string& out) const;
    bool writeFile(const std::string& path, const std::string& content) const;
    bool removeFile(const std::string& path) const;       // also removes folders left empty

    Status computeStatus() const;

    // Changes the working folder from snapshot `from` to snapshot `to`.
    // Without force it refuses to overwrite or delete files with local changes.
    Result syncWorkingDir(const Snapshot::Entries& from, const Snapshot::Entries& to, bool force) const;

private:
    std::string fullPath(const std::string& path) const;
    std::vector<std::string> ignorePatterns() const;
    static bool matches(const std::string& path, const std::string& pattern);

    std::string root_;
    Storage& storage_;
};