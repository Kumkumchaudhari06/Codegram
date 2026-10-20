#pragma once
#include <string>
#include <vector>
#include "core/filetracker.h"
#include "core/types.h"
#include "storage/storage.h"

class Staging {
public:
    Staging(Storage& storage, FileTracker& tracker);

    static std::string normalizeSpec(const std::string& raw);                  // "." means everything
    static bool matches(const std::string& path, const std::string& spec);     // spec is normalized

    // Stages new, changed and deleted files under a file, a folder, or "."
    Result add(const std::string& spec, std::vector<std::string>* touched = nullptr);
    // Puts the staged version back to what the last commit has
    Result unstage(const std::string& spec);

private:
    Storage& storage_;
    FileTracker& tracker_;
};