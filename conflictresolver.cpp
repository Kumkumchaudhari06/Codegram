#include "merge/conflictresolver.h"
#include "core/diffengine.h"
#include "utils/logger.h"

namespace {
int findEntry(const MergeState& st, const std::string& path) {
    for (size_t i = 0; i < st.conflicts.size(); ++i) if (st.conflicts[i].path == path) return static_cast<int>(i);
    return -1;
}
}

ConflictResolver::ConflictResolver(Storage& s, FileTracker& t) : storage_(s), tracker_(t), states_(s.dir()) {}

bool ConflictResolver::active() const { return states_.exists(); }

std::vector<std::string> ConflictResolver::conflictedPaths() const {
    std::vector<std::string> out;
    MergeState st;
    if (states_.load(st)) for (const auto& c : st.conflicts) out.push_back(c.path);
    return out;
}

size_t ConflictResolver::remaining() const {
    MergeState st;
    return states_.load(st) ? st.conflicts.size() : 0;
}

bool ConflictResolver::view(const std::string& path, ConflictView& out) const {
    MergeState st;
    if (!states_.load(st)) return false;
    int idx = findEntry(st, path);
    if (idx < 0) return false;
    const ConflictEntry& e = st.conflicts[idx];

    out = ConflictView();
    out.path = path;
    out.oursDeleted = e.oursBlob.empty();
    out.theirsDeleted = e.theirsBlob.empty();
    std::string b, o, t;
    if (!e.baseBlob.empty()) storage_.objects.readBlob(e.baseBlob, b);
    if (!e.oursBlob.empty()) storage_.objects.readBlob(e.oursBlob, o);
    if (!e.theirsBlob.empty()) storage_.objects.readBlob(e.theirsBlob, t);
    out.binary = DiffEngine::isBinary(b) || DiffEngine::isBinary(o) || DiffEngine::isBinary(t);
    if (!out.binary) { out.baseText = b; out.oursText = o; out.theirsText = t; }
    if (!out.binary && !out.oursDeleted && !out.theirsDeleted)
        out.markerText = TextMerge::renderMarkers(TextMerge::merge3(b, o, t), st.oursLabel, st.theirsLabel, &out.hunks);
    return true;
}

Result ConflictResolver::finish(const std::string& path, bool deleteFile, const std::string& content) {
    MergeState st;
    if (!states_.load(st)) return Result::fail("No merge in progress.");
    int idx = findEntry(st, path);
    if (idx < 0) return Result::fail("'" + path + "' is not in conflict.");

    if (deleteFile) {
        tracker_.removeFile(path);
        storage_.index.remove(path);
    } else {
        std::string h = storage_.objects.writeBlob(content);
        if (h.empty() || !tracker_.writeFile(path, content)) return Result::fail("Cannot write '" + path + "'.");
        if (!storage_.index.add(path, h)) return Result::fail("Cannot stage '" + path + "'.");
    }
    storage_.index.save();
    st.conflicts.erase(st.conflicts.begin() + idx);
    states_.save(st);
    Logger::info("resolved " + path);

    std::string msg = "Resolved '" + path + "'. ";
    msg += st.conflicts.empty() ? "All conflicts are resolved: run 'commit' to finish the merge."
                                : std::to_string(st.conflicts.size()) + " conflict file(s) left.";
    return Result::success(msg);
}

Result ConflictResolver::resolveWholeFile(const std::string& path, TextMerge::Choice choice) {
    ConflictView v;
    MergeState st;
    if (!view(path, v) || !states_.load(st)) return Result::fail("'" + path + "' is not in conflict.");
    const ConflictEntry& e = st.conflicts[findEntry(st, path)];

    if (v.oursDeleted || v.theirsDeleted || v.binary) {   // the whole file is the unit
        if (choice == TextMerge::Choice::Both)
            return Result::fail("'Both' only works when both sides have a text version of the file.");
        const std::string& blob = (choice == TextMerge::Choice::Ours) ? e.oursBlob : e.theirsBlob;
        if (blob.empty()) return finish(path, true, "");   // that side deleted the file
        std::string content;
        if (!storage_.objects.readBlob(blob, content)) return Result::fail("Cannot read the stored version.");
        return finish(path, false, content);
    }
    TextMerge::Merged m = TextMerge::merge3(v.baseText, v.oursText, v.theirsText);
    std::vector<TextMerge::Resolution> res(m.conflictCount());
    for (auto& r : res) r.choice = choice;
    std::string out;
    if (!TextMerge::renderResolved(m, res, out)) return Result::fail("Cannot build the resolved file.");
    return finish(path, false, out);
}

Result ConflictResolver::resolveHunks(const std::string& path, const std::vector<TextMerge::Resolution>& decisions) {
    ConflictView v;
    if (!view(path, v)) return Result::fail("'" + path + "' is not in conflict.");
    if (v.binary || v.oursDeleted || v.theirsDeleted)
        return Result::fail("'" + path + "' has no conflict blocks: choose ours or theirs for the whole file.");
    TextMerge::Merged m = TextMerge::merge3(v.baseText, v.oursText, v.theirsText);
    std::string out;
    if (!TextMerge::renderResolved(m, decisions, out))
        return Result::fail("This file has " + std::to_string(m.conflictCount()) + " conflict block(s): give one decision for each.");
    return finish(path, false, out);
}

Result ConflictResolver::resolveWithContent(const std::string& path, const std::string& content) {
    if (TextMerge::hasMarkers(content)) return Result::fail("The text still contains conflict markers.");
    return finish(path, false, content);
}

Result ConflictResolver::markResolved(const std::string& path) {
    if (!tracker_.exists(path)) return finish(path, true, "");   // the user deleted the file
    std::string content;
    if (!tracker_.readFile(path, content)) return Result::fail("Cannot read '" + path + "'.");
    return resolveWithContent(path, content);
}