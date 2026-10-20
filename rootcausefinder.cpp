#include "rootcause/rootcausefinder.h"
#include "core/diffengine.h"
#include "core/snapshot.h"
#include "utils/dependencygraph.h"

RootCauseFinder::RootCauseFinder(const std::string& root, Storage& s, CommitManager& c, ConflictResolver& r)
    : root_(root), storage_(s), commits_(c), resolver_(r), states_(s.dir()) {}

// The newest commit on a branch (back to the merge base) that changed the file
SideChange RootCauseFinder::lastChange(const std::string& tip, const std::string& base, const std::string& path) const {
    SideChange sc;
    std::string cur = tip;
    for (int guard = 0; guard < 10000 && !cur.empty() && cur != base; ++guard) {
        Commit c;
        if (!commits_.read(cur, c)) break;
        std::string here = Snapshot::blobAt(storage_.objects, cur, path);
        std::string before = c.parents.empty() ? "" : Snapshot::blobAt(storage_.objects, c.parents[0], path);
        if (here != before) {
            sc.commitHash = cur; sc.author = c.author; sc.message = CommitManager::firstLine(c.message);
            return sc;
        }
        cur = c.parents.empty() ? "" : c.parents[0];
    }
    Commit c;
    if (commits_.read(tip, c)) { sc.commitHash = tip; sc.author = c.author; sc.message = CommitManager::firstLine(c.message); }
    return sc;
}

static std::string joinText(const std::vector<std::string>& lines) { return DiffEngine::joinLines(lines, false); }

Result RootCauseFinder::explain(const std::string& path, std::vector<RootCause>& out) {
    out.clear();
    MergeState st;
    if (!states_.load(st))
        return Result::fail("No merge in progress. Run 'merge' first: the root cause finder explains the conflicts it finds.");
    const ConflictEntry* entry = nullptr;
    for (const auto& c : st.conflicts) if (c.path == path) entry = &c;
    if (!entry) return Result::fail("'" + path + "' is not in conflict.");
    ConflictView view;
    if (!resolver_.view(path, view)) return Result::fail("Cannot read the conflict.");

    DependencyGraph graph;
    graph.buildFromDirectory(root_);
    RootCauseAnalyzer analyzer(&graph);

    SideChange ours = lastChange(st.oursCommit, st.baseCommit, path);
    SideChange theirs = lastChange(st.theirsCommit, st.baseCommit, path);

    if (view.markerText.empty()) {   // binary file or deleted-vs-changed: explain the whole file
        Snapshot::Entries oursE, theirsE;
        Snapshot::ofCommit(storage_.objects, st.oursCommit, oursE);
        Snapshot::ofCommit(storage_.objects, st.theirsCommit, theirsE);
        auto kindOf = [&](const Snapshot::Entries& side, const std::string& blob) -> std::string {
            if (!blob.empty()) return "modified";
            for (const auto& kv : side)   // same content under another name = renamed
                if (kv.first != path && !entry->baseBlob.empty() && kv.second == entry->baseBlob) return "renamed";
            return "deleted";
        };
        ConflictEvidence ev;
        ev.file = path; ev.startLine = 1; ev.endLine = 1;
        ev.oursText = entry->oursBlob; ev.theirsText = entry->theirsBlob;
        ev.ours = ours; ev.ours.kind = kindOf(oursE, entry->oursBlob);
        ev.theirs = theirs; ev.theirs.kind = kindOf(theirsE, entry->theirsBlob);
        out = analyzer.analyze(ev);
        return Result::success();
    }

    for (const auto& h : view.hunks) {   // one explanation per conflict block
        ConflictEvidence ev;
        ev.file = path;
        ev.startLine = h.startLine; ev.endLine = h.endLine;
        ev.oursText = joinText(h.ours); ev.theirsText = joinText(h.theirs);
        ev.ours = ours;     ev.ours.kind = (h.ours.empty() && !h.base.empty()) ? "deleted" : "modified";
        ev.theirs = theirs; ev.theirs.kind = (h.theirs.empty() && !h.base.empty()) ? "deleted" : "modified";
        for (const auto& rc : analyzer.analyze(ev)) out.push_back(rc);
    }
    return Result::success();
}