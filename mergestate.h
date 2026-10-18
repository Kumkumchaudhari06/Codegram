#pragma once
#include <string>
#include <vector>

// Saved in .codegram/MERGE_STATE while a merge has unresolved conflicts
struct ConflictEntry {
    std::string path;
    std::string baseBlob, oursBlob, theirsBlob;   // "" = the file does not exist on that side
};

struct MergeState {
    std::string oursCommit, theirsCommit, baseCommit;
    std::string oursLabel, theirsLabel;
    std::vector<ConflictEntry> conflicts;
};

class MergeStateStore {
public:
    explicit MergeStateStore(const std::string& repoDir);
    bool exists() const;
    bool load(MergeState& out) const;
    bool save(const MergeState& state) const;
    bool clear() const;
private:
    std::string file_;
};