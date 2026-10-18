#pragma once
#include <set>
#include <string>
#include <vector>
#include "core/types.h"
#include "storage/storage.h"

class CommitManager {
public:
    explicit CommitManager(Storage& storage);

    // Commit from the current staging area. extraParents makes it a merge commit.
    Result commit(const std::string& message, const std::vector<std::string>& extraParents,
                  std::string* newHash, bool allowEmpty = false);

    bool read(const std::string& hash, Commit& out) const;
    std::vector<Commit> log(const std::string& startHash, size_t limit = 0) const;   // newest first

    std::set<std::string> ancestors(const std::string& hash, bool includeSelf) const;
    bool isAncestor(const std::string& maybeAncestor, const std::string& descendant) const;   // true if equal
    std::string mergeBase(const std::string& a, const std::string& b) const;                  // "" if none

    // "HEAD", a branch name, a full hash or a short hash (4+ characters), plus "~N" for parents
    std::string resolve(const std::string& spec) const;

    static std::string firstLine(const std::string& message);
    static std::string shortHash(const std::string& hash);
    static std::string formatTime(long long timestamp);

private:
    std::string resolveBase(const std::string& name) const;
    Storage& storage_;
};