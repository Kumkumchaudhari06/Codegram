#pragma once
#include <functional>
#include <string>
#include "core/snapshot.h"

// Builds the text of a diff between two snapshots (all files, or one file/folder)
namespace DiffReport {
    // Gives the content of a file: (path, blob hash) -> text
    typedef std::function<bool(const std::string& path, const std::string& hash, std::string& out)> Loader;

    struct Report {
        std::string text;
        int files = 0;
        int additions = 0;
        int deletions = 0;
    };

    Report build(const Snapshot::Entries& before, const Snapshot::Entries& after,
                 const Loader& loadBefore, const Loader& loadAfter, const std::string& pathFilter = "");
}