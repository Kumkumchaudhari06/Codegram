#pragma once
#include <string>
#include <vector>

class RefStore {
public:
    // repoDir is the .codegram folder
    explicit RefStore(const std::string& repoDir);

    // Letters, digits, . _ - and / only; no "..", no leading "-" or "/", no ".lock" ending
    static bool isValidBranchName(const std::string& name);
        // Aliases used by BranchManager / RepoManager / ConflictResolver
    static bool validBranchName(const std::string& name) { return isValidBranchName(name); }
    bool init(const std::string& branch = "main") { return initHead(branch); }
    bool getBranch(const std::string& name, std::string& hashOut) const {
        hashOut = branchCommit(name);
        return !hashOut.empty();
    }
    // HEAD
    bool initHead(const std::string& branch = "main");   // HEAD -> branch (branch need not exist yet)
    bool setHeadToBranch(const std::string& branch);
    bool setHeadDetached(const std::string& commitHash);
    bool isDetached() const;
    std::string currentBranch() const;   // "" if detached or HEAD is missing
    std::string headCommit() const;      // "" if the branch has no commits yet
    bool updateHead(const std::string& commitHash);   // moves the current branch (or detached HEAD)

    // Branches
    bool branchExists(const std::string& name) const;
    std::string branchCommit(const std::string& name) const;   // "" if missing
    bool setBranch(const std::string& name, const std::string& commitHash);      // create or move
    bool createBranch(const std::string& name, const std::string& commitHash);   // fails if it exists
    bool deleteBranch(const std::string& name);   // fails if missing or currently checked out
    std::vector<std::string> listBranches() const;   // sorted

private:
    std::string headFile_;
    std::string headsDir_;
    std::string branchPath(const std::string& name) const;
};