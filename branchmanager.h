#pragma once
#include <string>
#include <vector>
#include "core/commitmanager.h"
#include "core/filetracker.h"
#include "core/types.h"
#include "storage/storage.h"

struct BranchInfo {
    std::string name, commit, subject;
    bool current = false;
};

class BranchManager {
public:
    BranchManager(Storage& storage, FileTracker& tracker, CommitManager& commits);

    Result create(const std::string& name, const std::string& startPoint = "");   // "" = current commit
    Result remove(const std::string& name, bool force = false);
    std::vector<BranchInfo> list() const;
    // Switch to a branch (or a commit, which detaches HEAD). Updates the files and the staging area.
    Result switchTo(const std::string& target, bool force = false);

private:
    Storage& storage_;
    FileTracker& tracker_;
    CommitManager& commits_;
};