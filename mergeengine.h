#pragma once
#include <string>
#include <vector>
#include "core/commitmanager.h"
#include "core/filetracker.h"
#include "core/snapshot.h"
#include "merge/mergestate.h"
#include "storage/storage.h"

struct MergeOutcome {
    enum Kind { UpToDate, FastForward, Merged, Conflicts, Failed };
    Kind kind = Failed;
    std::string message;
    std::string newCommit;                  // FastForward / Merged
    std::vector<std::string> autoMerged;    // files both sides changed that merged cleanly
    std::vector<std::string> conflicted;    // files that need a decision
    bool ok() const { return kind != Failed; }
};

class MergeEngine {
public:
    MergeEngine(Storage& storage, FileTracker& tracker, CommitManager& commits);

    bool inProgress() const;
    bool state(MergeState& out) const;
    bool clearState();

    // Merges a branch/commit into the current branch. dryRun only reports what would happen.
    MergeOutcome merge(const std::string& target, const std::string& message = "", bool dryRun = false);
    Result abort();

private:
    struct Plan {
        Snapshot::Entries result;                 // files that merged without a conflict
        std::vector<ConflictEntry> conflicts;
        std::vector<std::string> autoMerged;
    };
    bool buildPlan(const Snapshot::Entries& base, const Snapshot::Entries& ours,
                   const Snapshot::Entries& theirs, Plan& plan, std::string& error);
    bool setIndex(const Snapshot::Entries& entries);

    Storage& storage_;
    FileTracker& tracker_;
    CommitManager& commits_;
    MergeStateStore states_;
};