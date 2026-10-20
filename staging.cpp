#include "core/staging.h"
#include "storage/index.h"
#include <set>

Staging::Staging(Storage& storage, FileTracker& tracker) : storage_(storage), tracker_(tracker) {}

std::string Staging::normalizeSpec(const std::string& raw) {
    std::string s = Index::normalizePath(raw);
    while (s.size() > 1 && s.back() == '/') s.pop_back();
    return s.empty() ? "." : s;
}

bool Staging::matches(const std::string& path, const std::string& spec) {
    if (spec == ".") return true;
    return path == spec || (path.size() > spec.size() && path.compare(0, spec.size(), spec) == 0 &&
                            path[spec.size()] == '/');
}

Result Staging::add(const std::string& raw, std::vector<std::string>* touched) {
    const std::string spec = normalizeSpec(raw);
    if (spec != "." && !Index::isValidPath(spec)) return Result::fail("invalid path: " + raw);

    Snapshot::Entries work = tracker_.scan();
    std::vector<std::string> toStage, toRemove;
    for (const auto& kv : work)
        if (matches(kv.first, spec)) toStage.push_back(kv.first);
    for (const auto& kv : storage_.index.entries())
        if (matches(kv.first, spec) && !work.count(kv.first) && !tracker_.exists(kv.first))
            toRemove.push_back(kv.first);   // tracked file that was deleted

    if (toStage.empty() && toRemove.empty() && spec != ".") {
        if (tracker_.exists(spec)) return Result::fail("'" + raw + "' is ignored by .codegramignore or has no files");
        return Result::fail("'" + raw + "' did not match any file");
    }

    for (const auto& p : toRemove) {
        storage_.index.remove(p);
        if (touched) touched->push_back(p);
    }
    int staged = 0;
    for (const auto& p : toStage) {
        if (touched) touched->push_back(p);
        if (storage_.index.hashOf(p) == work[p]) continue;   // already staged exactly like this
        std::string content;
        if (!tracker_.readFile(p, content)) return Result::fail("cannot read " + p);
        std::string h = storage_.objects.writeBlob(content);
        if (h.empty()) return Result::fail("cannot store " + p);
        if (!storage_.index.add(p, h)) return Result::fail("cannot stage '" + p + "' (file/folder name clash)");
        ++staged;
    }
    if (!storage_.index.save()) return Result::fail("cannot save the staging area");
    return Result::success("Staged " + std::to_string(staged) + " file(s), removed " +
                           std::to_string(toRemove.size()) + " deleted file(s).");
}

Result Staging::unstage(const std::string& raw) {
    const std::string spec = normalizeSpec(raw);
    if (spec != "." && !Index::isValidPath(spec)) return Result::fail("invalid path: " + raw);

    Snapshot::Entries head;
    if (!Snapshot::ofCommit(storage_.objects, storage_.refs.headCommit(), head))
        return Result::fail("cannot read the last commit");

    std::set<std::string> paths;
    for (const auto& kv : storage_.index.entries()) if (matches(kv.first, spec)) paths.insert(kv.first);
    for (const auto& kv : head) if (matches(kv.first, spec)) paths.insert(kv.first);
    if (paths.empty()) return Result::fail("'" + raw + "' did not match any staged or tracked file");

    std::vector<std::string> differing;
    for (const auto& p : paths) {
        auto h = head.find(p);
        if (storage_.index.hashOf(p) != (h == head.end() ? std::string() : h->second)) differing.push_back(p);
    }
    for (const auto& p : differing) storage_.index.remove(p);
    for (const auto& p : differing) {
        auto h = head.find(p);
        if (h != head.end() && !storage_.index.add(p, h->second))
            return Result::fail("cannot restore '" + p + "' in the staging area");
    }
    if (!storage_.index.save()) return Result::fail("cannot save the staging area");
    return Result::success("Unstaged " + std::to_string(differing.size()) + " file(s).");
}