#pragma once
#include <string>
#include <vector>
#include "core/commitmanager.h"
#include "merge/conflictresolver.h"
#include "merge/mergestate.h"
#include "rootcause/rootcauseanalyzer.h"
#include "storage/storage.h"

// The "Root cause finder" button: which lines conflict, why, and which commits caused it
class RootCauseFinder {
public:
    RootCauseFinder(const std::string& root, Storage& storage, CommitManager& commits, ConflictResolver& resolver);

    // Causes for one conflicted file of the merge in progress, most likely first.
    // Line numbers refer to the file with conflict markers in the working folder.
    Result explain(const std::string& path, std::vector<RootCause>& out);

private:
    SideChange lastChange(const std::string& tip, const std::string& base, const std::string& path) const;

    std::string root_;
    Storage& storage_;
    CommitManager& commits_;
    ConflictResolver& resolver_;
    MergeStateStore states_;
};