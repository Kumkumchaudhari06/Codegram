#pragma once
#include <string>

namespace Archive {
    // Packs every file under srcDir (no trailing slash) into one archive file.
    // The top-level "backup" folder is skipped.
    bool exportDir(const std::string& srcDir, const std::string& archiveFile, bool compress = true);

    // Unpacks an archive into destDir. Fails without writing anything if the archive is invalid.
    bool importArchive(const std::string& archiveFile, const std::string& destDir);
}