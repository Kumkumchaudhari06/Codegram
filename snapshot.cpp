#include "core/snapshot.h"
#include "storage/index.h"
#include <algorithm>

namespace Snapshot {

bool ofCommit(const ObjectStore& store, const std::string& commitHash, Entries& out) {
    out.clear();
    if (commitHash.empty()) return true;
    Commit c;
    if (!store.readCommit(commitHash, c)) return false;
    Index tmp(".");   // used only as a tree reader, never saved
    if (!tmp.loadFromTree(store, c.treeHash)) return false;
    out = tmp.entries();
    return true;
}

std::string blobAt(const ObjectStore& store, const std::string& commitHash, const std::string& path) {
    Commit c;
    if (commitHash.empty() || !store.readCommit(commitHash, c)) return "";
    std::string treeHash = c.treeHash;
    size_t start = 0;
    while (true) {
        size_t slash = path.find('/', start);
        const bool last = (slash == std::string::npos);
        std::string part = path.substr(start, last ? std::string::npos : slash - start);
        Tree t;
        if (!store.readTree(treeHash, t)) return "";
        const TreeEntry* found = nullptr;
        for (const auto& e : t.entries)
            if (e.name == part && e.type == (last ? "blob" : "tree")) { found = &e; break; }
        if (!found) return "";
        if (last) return found->hash;
        treeHash = found->hash;
        start = slash + 1;
    }
}

static FileChange makeChange(ChangeType type, const std::string& path, const std::string& oldPath) {
    FileChange c;
    c.type = type;
    c.path = path;
    c.oldPath = oldPath;
    return c;
}

std::vector<FileChange> compare(const Entries& before, const Entries& after, bool detectRenames) {
    std::vector<FileChange> changes;
    std::vector<std::string> added, deleted;

    for (const auto& kv : after) {
        auto it = before.find(kv.first);
        if (it == before.end()) added.push_back(kv.first);
        else if (it->second != kv.second) changes.push_back(makeChange(ChangeType::Modified, kv.first, ""));
    }
    for (const auto& kv : before)
        if (!after.count(kv.first)) deleted.push_back(kv.first);

    std::vector<bool> addedUsed(added.size(), false), deletedUsed(deleted.size(), false);
    if (detectRenames) {
        for (size_t d = 0; d < deleted.size(); ++d)
            for (size_t a = 0; a < added.size(); ++a)
                if (!addedUsed[a] && before.at(deleted[d]) == after.at(added[a])) {
                    addedUsed[a] = deletedUsed[d] = true;
                    changes.push_back(makeChange(ChangeType::Renamed, added[a], deleted[d]));
                    break;
                }
    }
    for (size_t a = 0; a < added.size(); ++a)
        if (!addedUsed[a]) changes.push_back(makeChange(ChangeType::Added, added[a], ""));
    for (size_t d = 0; d < deleted.size(); ++d)
        if (!deletedUsed[d]) changes.push_back(makeChange(ChangeType::Deleted, deleted[d], ""));

    std::sort(changes.begin(), changes.end(),
              [](const FileChange& x, const FileChange& y) { return x.path < y.path; });
    return changes;
}

} // namespace Snapshot