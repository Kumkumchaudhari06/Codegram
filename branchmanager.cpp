#include "core/branchmanager.h"
#include "utils/logger.h"

BranchManager::BranchManager(Storage& s, FileTracker& t, CommitManager& c) : storage_(s), tracker_(t), commits_(c) {}

Result BranchManager::create(const std::string& name, const std::string& startPoint) {
    if (!RefStore::isValidBranchName(name)) return Result::fail("invalid branch name: " + name);
    if (storage_.refs.branchExists(name)) return Result::fail("branch '" + name + "' already exists");
    std::string commit = startPoint.empty() ? storage_.refs.headCommit() : commits_.resolve(startPoint);
    if (commit.empty())
        return Result::fail(startPoint.empty() ? "there are no commits yet: make a first commit before creating branches"
                                               : "unknown start point: " + startPoint);
    if (!storage_.refs.createBranch(name, commit)) return Result::fail("cannot create branch '" + name + "'");
    Logger::info("created branch " + name);
    return Result::success("Created branch '" + name + "' at " + CommitManager::shortHash(commit));
}

Result BranchManager::remove(const std::string& name, bool force) {
    if (!storage_.refs.branchExists(name)) return Result::fail("no such branch: " + name);
    if (name == storage_.refs.currentBranch()) return Result::fail("cannot delete the branch you are on");
    std::string tip = storage_.refs.branchCommit(name);
    if (!force && !commits_.isAncestor(tip, storage_.refs.headCommit()))
        return Result::fail("branch '" + name + "' has commits that are not merged into the current branch (use -D to force)");
    if (!storage_.refs.deleteBranch(name)) return Result::fail("cannot delete branch '" + name + "'");
    Logger::info("deleted branch " + name);
    return Result::success("Deleted branch '" + name + "' (was " + CommitManager::shortHash(tip) + ")");
}

std::vector<BranchInfo> BranchManager::list() const {
    std::vector<BranchInfo> out;
    for (const auto& name : storage_.refs.listBranches()) {
        BranchInfo b;
        b.name = name;
        b.commit = storage_.refs.branchCommit(name);
        b.current = (name == storage_.refs.currentBranch());
        Commit c;
        if (!b.commit.empty() && commits_.read(b.commit, c)) b.subject = CommitManager::firstLine(c.message);
        out.push_back(b);
    }
    return out;
}

Result BranchManager::switchTo(const std::string& target, bool force) {
    std::string branch, commit;
    if (storage_.refs.branchExists(target)) { branch = target; commit = storage_.refs.branchCommit(target); }
    else commit = commits_.resolve(target);
    if (commit.empty()) return Result::fail("unknown branch or commit: " + target);
    if (!branch.empty() && branch == storage_.refs.currentBranch())
        return Result::success("Already on '" + branch + "'");

    Snapshot::Entries current, wanted;
    Commit wantedCommit;
    if (!Snapshot::ofCommit(storage_.objects, storage_.refs.headCommit(), current) ||
        !Snapshot::ofCommit(storage_.objects, commit, wanted) || !commits_.read(commit, wantedCommit))
        return Result::fail("cannot read the commits involved");

    if (!force && storage_.index.entries() != current)
        return Result::fail("You have staged changes. Commit them or unstage them first (or use --force).");

    Result r = tracker_.syncWorkingDir(current, wanted, force);
    if (!r.ok) return r;
    if (!storage_.index.loadFromTree(storage_.objects, wantedCommit.treeHash) || !storage_.index.save())
        return Result::fail("cannot update the staging area");
    bool ok = branch.empty() ? storage_.refs.setHeadDetached(commit) : storage_.refs.setHeadToBranch(branch);
    if (!ok) return Result::fail("cannot move HEAD");

    Logger::info("switched to " + (branch.empty() ? CommitManager::shortHash(commit) : branch));
    return Result::success(branch.empty() ? "HEAD is now detached at " + CommitManager::shortHash(commit)
                                          : "Switched to branch '" + branch + "'");
}