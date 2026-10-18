#include "core/diffreport.h"
#include "core/diffengine.h"

namespace DiffReport {

static bool inFilter(const std::string& p, const std::string& f) {
    if (f.empty() || f == ".") return true;
    return p == f || (p.size() > f.size() && p.compare(0, f.size(), f) == 0 && p[f.size()] == '/');
}

Report build(const Snapshot::Entries& before, const Snapshot::Entries& after,
             const Loader& loadBefore, const Loader& loadAfter, const std::string& filter) {
    Report rep;
    for (const FileChange& ch : Snapshot::compare(before, after)) {
        const std::string oldName = (ch.type == ChangeType::Renamed) ? ch.oldPath : ch.path;
        if (!inFilter(ch.path, filter) && !inFilter(oldName, filter)) continue;
        ++rep.files;

        rep.text += "diff --codegram a/" + oldName + " b/" + ch.path + "\n";
        if (ch.type == ChangeType::Added)   rep.text += "new file\n";
        if (ch.type == ChangeType::Deleted) rep.text += "deleted file\n";
        if (ch.type == ChangeType::Renamed) {
            rep.text += "rename from " + oldName + "\nrename to " + ch.path + "\n";
            continue;   // same content, nothing to compare
        }

        std::string oldText, newText;
        bool ok = true;
        if (ch.type != ChangeType::Added)   ok = ok && loadBefore(oldName, before.at(oldName), oldText);
        if (ch.type != ChangeType::Deleted) ok = ok && loadAfter(ch.path, after.at(ch.path), newText);
        if (!ok) { rep.text += "(cannot read file content)\n"; continue; }
        if (DiffEngine::isBinary(oldText) || DiffEngine::isBinary(newText)) {
            rep.text += "Binary files differ\n";
            continue;
        }

        std::vector<DiffLine> d = DiffEngine::diffLines(DiffEngine::splitLines(oldText),
                                                        DiffEngine::splitLines(newText));
        for (const auto& l : d) {
            if (l.op == LineOp::Added) ++rep.additions;
            if (l.op == LineOp::Removed) ++rep.deletions;
        }
        rep.text += std::string("--- ") + (ch.type == ChangeType::Added ? "/dev/null" : "a/" + oldName) + "\n";
        rep.text += std::string("+++ ") + (ch.type == ChangeType::Deleted ? "/dev/null" : "b/" + ch.path) + "\n";
        rep.text += DiffEngine::unifiedDiff(d);
    }
    return rep;
}

} // namespace DiffReport