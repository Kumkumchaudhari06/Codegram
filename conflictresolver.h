#pragma once
#include <string>
#include <vector>
#include "core/filetracker.h"
#include "core/types.h"
#include "merge/mergestate.h"
#include "merge/textmerge.h"
#include "storage/storage.h"

// Everything a GUI needs to show one conflicted file
struct ConflictView {
    std::string path;
    bool binary = false;
    bool oursDeleted = false, theirsDeleted = false;
    std::string baseText, oursText, theirsText;   // the three versions (empty for binary files)
    std::string markerText;                        // the file with conflict markers (text conflicts only)
    std::vector<TextMerge::Hunk> hunks;            // every conflict block with its line numbers
};

class ConflictResolver {
public:
    ConflictResolver(Storage& storage, FileTracker& tracker);

    bool active() const;
    std::vector<std::string> conflictedPaths() const;
    size_t remaining() const;
    bool view(const std::string& path, ConflictView& out) const;

    // Keep our side / their side. In a text file this decides every conflicting block and keeps all
    // non-conflicting changes from both sides. Both = our lines followed by their lines.
    Result resolveWholeFile(const std::string& path, TextMerge::Choice choice);
    // One decision per conflict block, in the order shown by view()
    Result resolveHunks(const std::string& path, const std::vector<TextMerge::Resolution>& decisions);
    // The user typed the final content (GUI editor)
    Result resolveWithContent(const std::string& path, const std::string& content);
    // The user edited the file in the working folder: accept it as it is now
    Result markResolved(const std::string& path);

private:
    Result finish(const std::string& path, bool deleteFile, const std::string& content);
    Storage& storage_;
    FileTracker& tracker_;
    MergeStateStore states_;
};