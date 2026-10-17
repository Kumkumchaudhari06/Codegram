#pragma once
#include <string>
#include <vector>

namespace Backup {
    // Copies sourceDir into backupRoot/snapshot_<time>. Anything inside backupRoot is skipped,
    // so backupRoot may safely live inside sourceDir. Returns the snapshot path, or "" on failure.
    std::string create(const std::string& sourceDir, const std::string& backupRoot);

    // Snapshot folders found in backupRoot, oldest first
    std::vector<std::string> list(const std::string& backupRoot);

    // Replaces the contents of targetDir (except its "backup" folder) with the snapshot
    bool restore(const std::string& snapshotDir, const std::string& targetDir);
}