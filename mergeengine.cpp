#include "merge/mergeengine.h"
#include "core/diffengine.h"
#include "merge/textmerge.h"
#include "utils/logger.h"
#include <set>

namespace {
std::string get(const Snapshot::Entries& m, const std::string& k) {
    auto it = m.find(k);
    return it == m.end() ? "" : it->second;
}
}

MergeEngine::MergeEngine(Storage& s, FileTracker& t, CommitManager& c)
    : storage_(s), tracker_(t), commits_(c), states_(s.dir()) {}

bool MergeEngine::inProgress() const { return states_.exists(); }
bool MergeEngine::state(MergeState& out) const { return states_.load(out); }
bool MergeEngine::clearState() { return states_.clear(); }

bool MergeEngine::setIndex(const Snapshot::Entries& entries) {
    storage_.index.clear();
    for (const auto& kv : entries)
        if (!storage_.index.add(kv.first, kv.second)) return false;
    return storage_.index.save();
}

bool MergeEngine::buildPlan(const Snapshot::Entries& B, const Snapshot::Entries& O,
                            const Snapshot::Entries& T, Plan& plan, std::string& err) {
    std::set<std::string> paths;
    for (const auto& kv : B) paths.insert(kv.first);
    for (const auto& kv : O) paths.insert(kv.first);
    for (const auto& kv : T) paths.insert(kv.first);

    for (const auto& p : paths) {
        const std::string b = get(B, p), o = get(O, p), t = get(T, p);
        if (o == t) { if (!o.empty()) plan.result[p] = o; continue; }   // same on both sides
        if (b == o) { if (!t.empty()) plan.result[p] = t; continue; }   // only theirs changed it
        if (b == t) { if (!o.empty()) plan.result[p] = o; continue; }   // only ours changed it

        ConflictEntry entry;
        entry.path = p; entry.baseBlob = b; entry.oursBlob = o; entry.theirsBlob = t;
        if (o.empty() || t.empty()) { plan.conflicts.push_back(entry); continue; }   // deleted vs changed

        std::string bt, ot, tt;
        if ((!b.empty() && !storage_.objects.readBlob(b, bt)) || !storage_.objects.readBlob(o, ot) ||
            !storage_.objects.readBlob(t, tt)) {
            err = "cannot read the stored versions of '" + p + "'";
            return false;
        }
        if (DiffEngine::isBinary(bt) || DiffEngine::isBinary(ot) || DiffEngine::isBinary(tt)) {
            plan.conflicts.push_back(entry);
            continue;
        }
        TextMerge::Merged m = TextMerge::merge3(bt, ot, tt);   // auto merge
        if (!m.clean()) { plan.conflicts.push_back(entry); continue; }
        std::string h = storage_.objects.writeBlob(TextMerge::renderMarkers(m, "", ""));
        if (h.empty()) { err = "cannot store the merged file '" + p + "'"; return false; }
        plan.result[p] = h;
        plan.autoMerged.push_back(p);
    }

    for (const auto& kv : plan.result) {   // a file on one side and a folder of the same name on the other
        size_t pos = 0;
        while ((pos = kv.first.find('/', pos)) != std::string::npos) {
            if (plan.result.count(kv.first.substr(0, pos))) {
                err = "file/folder name clash at '" + kv.first.substr(0, pos) + "'";
                return false;
            }
            ++pos;
        }
    }
    return true;
}

MergeOutcome MergeEngine::merge(const std::string& target, const std::string& message, bool dryRun) {
    MergeOutcome out;
    auto failWith = [&](const std::string& m) { out.kind = MergeOutcome::Failed; out.message = m; return out; };

    if (inProgress())
        return failWith("A merge is already in progress. Resolve the conflicts and commit, or run 'merge --abort'.");
    const std::string oursHash = storage_.refs.headCommit();
    if (oursHash.empty()) return failWith("There are no commits on the current branch yet.");
    const std::string theirsHash = storage_.refs.branchExists(target) ? storage_.refs.branchCommit(target)
                                                                      : commits_.resolve(target);
    if (theirsHash.empty()) return failWith("Unknown branch or commit: " + target);

    if (commits_.isAncestor(theirsHash, oursHash)) {
        out.kind = MergeOutcome::UpToDate;
        out.message = "Already up to date.";
        return out;
    }
    if (!dryRun) {
        Status st = tracker_.computeStatus();
        if (!st.staged.empty() || !st.unstaged.empty())
            return failWith("You have uncommitted changes. Commit or discard them before merging.");
    }

    Snapshot::Entries oursE, theirsE, baseE;
    if (!Snapshot::ofCommit(storage_.objects, oursHash, oursE) ||
        !Snapshot::ofCommit(storage_.objects, theirsHash, theirsE))
        return failWith("Cannot read the commits to merge.");
    const std::string current = storage_.refs.currentBranch();

    // Fast-forward: our branch has nothing theirs lacks
    if (commits_.isAncestor(oursHash, theirsHash)) {
        out.kind = MergeOutcome::FastForward;
        if (dryRun) { out.message = "This merge would fast-forward."; return out; }
        Result r = tracker_.syncWorkingDir(oursE, theirsE, false);
        if (!r.ok) return failWith(r.message);
        Commit c;
        if (!commits_.read(theirsHash, c) || !storage_.index.loadFromTree(storage_.objects, c.treeHash) ||
            !storage_.index.save() || !storage_.refs.updateHead(theirsHash))
            return failWith("Cannot move the branch forward.");
        out.newCommit = theirsHash;
        out.message = "Fast-forward to " + CommitManager::shortHash(theirsHash);
        Logger::info("fast-forward to " + CommitManager::shortHash(theirsHash));
        return out;
    }

    const std::string baseHash = commits_.mergeBase(oursHash, theirsHash);
    if (!Snapshot::ofCommit(storage_.objects, baseHash, baseE)) return failWith("Cannot read the common ancestor.");

    Plan plan;
    std::string err;
    if (!buildPlan(baseE, oursE, theirsE, plan, err)) return failWith(err);
    out.autoMerged = plan.autoMerged;
    for (const auto& c : plan.conflicts) out.conflicted.push_back(c.path);

    if (dryRun) {
        out.kind = plan.conflicts.empty() ? MergeOutcome::Merged : MergeOutcome::Conflicts;
        out.message = plan.conflicts.empty() ? "This merge would complete without conflicts."
                                             : "This merge would conflict in " + std::to_string(plan.conflicts.size()) + " file(s).";
        return out;
    }

    // ---- no conflicts: apply and commit ----
    if (plan.conflicts.empty()) {
        Result r = tracker_.syncWorkingDir(oursE, plan.result, false);
        if (!r.ok) return failWith(r.message);
        if (!setIndex(plan.result)) return failWith("Cannot update the staging area.");
        std::string msg = message.empty() ? "Merge " + target + " into " + (current.empty() ? "HEAD" : current) : message;
        std::string hash;
        Result cr = commits_.commit(msg, std::vector<std::string>(1, theirsHash), &hash, true);
        if (!cr.ok) return failWith(cr.message);
        out.kind = MergeOutcome::Merged;
        out.newCommit = hash;
        out.message = "Merged '" + target + "' without conflicts (" + std::to_string(plan.autoMerged.size()) +
                      " file(s) auto-merged).";
        Logger::info("merged " + target);
        return out;
    }

    // ---- conflicts: apply the clean parts, write conflict markers, remember what is left ----
    for (const auto& c : plan.conflicts) {
        std::string cur;
        if (tracker_.exists(c.path) && tracker_.readFile(c.path, cur) &&
            (c.oursBlob.empty() || FileTracker::hashContent(cur) != c.oursBlob))
            return failWith("The file '" + c.path + "' would be overwritten by the merge. Move or commit it first.");
    }
    Snapshot::Entries target2 = plan.result;
    for (const auto& c : plan.conflicts) if (!c.oursBlob.empty()) target2[c.path] = c.oursBlob;
    Result r = tracker_.syncWorkingDir(oursE, target2, false);
    if (!r.ok) return failWith(r.message);
    if (!setIndex(target2)) return failWith("Cannot update the staging area.");

    MergeState st;
    st.oursCommit = oursHash; st.theirsCommit = theirsHash; st.baseCommit = baseHash;
    st.oursLabel = current.empty() ? "HEAD" : current;
    st.theirsLabel = target;
    st.conflicts = plan.conflicts;

    for (const auto& c : plan.conflicts) {
        std::string b, o, t;
        bool haveO = !c.oursBlob.empty() && storage_.objects.readBlob(c.oursBlob, o);
        bool haveT = !c.theirsBlob.empty() && storage_.objects.readBlob(c.theirsBlob, t);
        if (!c.baseBlob.empty()) storage_.objects.readBlob(c.baseBlob, b);
        if (haveO && haveT) {
            if (!DiffEngine::isBinary(b) && !DiffEngine::isBinary(o) && !DiffEngine::isBinary(t))
                tracker_.writeFile(c.path, TextMerge::renderMarkers(TextMerge::merge3(b, o, t), st.oursLabel, st.theirsLabel));
        } else if (!haveO && haveT) {
            tracker_.writeFile(c.path, t);   // we deleted it, they changed it: show their version
        }
    }
    if (!states_.save(st)) return failWith("Cannot save the merge state.");

    out.kind = MergeOutcome::Conflicts;
    out.message = "Automatic merge stopped: " + std::to_string(plan.conflicts.size()) +
                  " file(s) need a decision. Use 'conflicts', 'rootcause' and 'resolve', then 'commit'.";
    Logger::warn("merge of " + target + " has conflicts");
    return out;
}

Result MergeEngine::abort() {
    MergeState st;
    if (!states_.load(st)) return Result::fail("No merge in progress.");
    Snapshot::Entries headE, before = storage_.index.entries();
    Commit c;
    if (!Snapshot::ofCommit(storage_.objects, st.oursCommit, headE) || !commits_.read(st.oursCommit, c))
        return Result::fail("Cannot read the commit the merge started from.");
    for (const auto& e : st.conflicts) if (e.oursBlob.empty()) tracker_.removeFile(e.path);
    Result r = tracker_.syncWorkingDir(before, headE, true);
    if (!r.ok) return r;
    if (!storage_.index.loadFromTree(storage_.objects, c.treeHash) || !storage_.index.save())
        return Result::fail("Cannot restore the staging area.");
    states_.clear();
    Logger::info("merge aborted");
    return Result::success("Merge aborted. Everything is back to the last commit.");
}