#pragma once
#include <string>
#include <vector>

struct Result {
    bool ok = false;
    std::string message;
    static Result success(const std::string& m = "") { Result r; r.ok = true; r.message = m; return r; }
    static Result fail(const std::string& m) { Result r; r.ok = false; r.message = m; return r; }
};

enum class ChangeType { Added, Modified, Deleted, Renamed };

inline const char* changeLabel(ChangeType t) {
    switch (t) {
        case ChangeType::Added:    return "added";
        case ChangeType::Modified: return "modified";
        case ChangeType::Deleted:  return "deleted";
        case ChangeType::Renamed:  return "renamed";
    }
    return "";
}

struct FileChange {
    ChangeType type = ChangeType::Modified;
    std::string path;      // current path
    std::string oldPath;   // only for Renamed
};

struct Status {
    std::string branch;          // "" when detached
    bool detached = false;
    std::string headCommit;      // "" before the first commit
    bool merging = false;
    std::vector<FileChange> staged;
    std::vector<FileChange> unstaged;
    std::vector<std::string> untracked;
    std::vector<std::string> conflicted;
    bool clean() const { return staged.empty() && unstaged.empty() && conflicted.empty(); }
};