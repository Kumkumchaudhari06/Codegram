#include "core/filetracker.h"
#include "core/diffengine.h"
#include "storage/index.h"
#include "utils/fileio.h"
#include "utils/sha256.h"
#include <filesystem>

namespace fs = std::filesystem;

FileTracker::FileTracker(const std::string& root, Storage& storage) : root_(root), storage_(storage) {}

std::string FileTracker::hashContent(const std::string& content) {
    return Sha256::hash("blob " + std::to_string(content.size()) + "\n" + content);   // same format as ObjectStore
}

std::string FileTracker::fullPath(const std::string& path) const { return root_ + "/" + path; }

std::vector<std::string> FileTracker::ignorePatterns() const {
    std::vector<std::string> out;
    std::string text;
    if (!FileIO::readFile(root_ + "/.codegramignore", text)) return out;
    for (std::string line : DiffEngine::splitLines(text)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
        size_t b = line.find_first_not_of(" \t");
        if (b == std::string::npos) continue;
        line = line.substr(b);
        if (line[0] != '#') out.push_back(line);
    }
    return out;
}

// Patterns:  build/  (folder anywhere)   *.o  (file extension)   a/b  (path)   name  (any part)
bool FileTracker::matches(const std::string& path, const std::string& pat) {
    std::vector<std::string> parts;
    size_t s = 0;
    while (true) {
        size_t sl = path.find('/', s);
        parts.push_back(path.substr(s, sl == std::string::npos ? std::string::npos : sl - s));
        if (sl == std::string::npos) break;
        s = sl + 1;
    }
    if (pat.back() == '/') {
        std::string name = pat.substr(0, pat.size() - 1);
        if (name.empty()) return false;
        if (name.find('/') != std::string::npos) return path.compare(0, name.size() + 1, name + "/") == 0;
        for (size_t i = 0; i + 1 < parts.size(); ++i) if (parts[i] == name) return true;
        return false;
    }
    if (pat.size() >= 2 && pat[0] == '*' && pat.find('/') == std::string::npos) {
        std::string suffix = pat.substr(1);
        const std::string& f = parts.back();
        return f.size() >= suffix.size() && f.compare(f.size() - suffix.size(), suffix.size(), suffix) == 0;
    }
    if (pat.find('/') != std::string::npos)
        return path == pat || path.compare(0, pat.size() + 1, pat + "/") == 0;
    for (const auto& p : parts) if (p == pat) return true;
    return false;
}

bool FileTracker::isIgnored(const std::string& path) const {
    for (const auto& p : ignorePatterns()) if (matches(path, p)) return true;
    return false;
}

Snapshot::Entries FileTracker::scan() const {
    Snapshot::Entries result;
    const std::vector<std::string> patterns = ignorePatterns();
    auto ignored = [&](const std::string& rel) {
        for (const auto& p : patterns) if (matches(rel, p)) return true;
        return false;
    };

    std::error_code ec;
    const fs::path rootPath(root_);
    for (auto it = fs::recursive_directory_iterator(rootPath, fs::directory_options::skip_permission_denied, ec);
         it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) break;
        const fs::path& p = it->path();
        std::string rel = p.lexically_relative(rootPath).generic_string();
        if (it->is_directory(ec)) {
            if (p.filename() == ".codegram" || ignored(rel + "/_")) it.disable_recursion_pending();
            continue;
        }
        if (!it->is_regular_file(ec)) continue;
        if (ignored(rel) || !Index::isValidPath(rel)) continue;
        std::string content;
        if (!FileIO::readFile(p.generic_string(), content)) continue;
        result[rel] = hashContent(content);
    }
    return result;
}

bool FileTracker::exists(const std::string& path) const {
    std::error_code ec;
    return fs::exists(fullPath(path), ec);
}

bool FileTracker::readFile(const std::string& path, std::string& out) const {
    std::error_code ec;
    if (!fs::is_regular_file(fullPath(path), ec)) return false;
    return FileIO::readFile(fullPath(path), out);
}

bool FileTracker::writeFile(const std::string& path, const std::string& content) const {
    return FileIO::writeFile(fullPath(path), content);
}

bool FileTracker::removeFile(const std::string& path) const {
    std::error_code ec;
    fs::path full(fullPath(path));
    if (!fs::exists(full, ec)) return true;
    fs::remove(full, ec);
    if (ec) return false;
    fs::path p = full.parent_path();
    while (p.string().size() > root_.size() && fs::is_empty(p, ec) && !ec) {
        fs::remove(p, ec);
        p = p.parent_path();
    }
    return true;
}

Status FileTracker::computeStatus() const {
    Status st;
    st.branch = storage_.refs.currentBranch();
    st.detached = storage_.refs.isDetached();
    st.headCommit = storage_.refs.headCommit();

    Snapshot::Entries head;
    Snapshot::ofCommit(storage_.objects, st.headCommit, head);

    st.staged = Snapshot::compare(head, storage_.index.entries());
    for (const FileChange& c : Snapshot::compare(storage_.index.entries(), scan())) {
        if (c.type == ChangeType::Added) st.untracked.push_back(c.path);   // not in the index = untracked
        else st.unstaged.push_back(c);
    }
    return st;
}

Result FileTracker::syncWorkingDir(const Snapshot::Entries& from, const Snapshot::Entries& to, bool force) const {
    std::vector<std::string> problems, toWrite, toDelete;

    for (const auto& kv : to) {
        std::string cur;
        bool has = exists(kv.first) && readFile(kv.first, cur);
        if (has && hashContent(cur) == kv.second) continue;   // already correct
        if (has) {
            auto f = from.find(kv.first);
            if (f == from.end() || hashContent(cur) != f->second) problems.push_back(kv.first);
        }
        toWrite.push_back(kv.first);
    }
    for (const auto& kv : from) {
        if (to.count(kv.first) || !exists(kv.first)) continue;
        std::string cur;
        if (readFile(kv.first, cur) && hashContent(cur) != kv.second) problems.push_back(kv.first);
        toDelete.push_back(kv.first);
    }

    if (!problems.empty() && !force) {
        std::string msg = "These files have local changes that would be overwritten or lost:\n";
        for (const auto& p : problems) msg += "  " + p + "\n";
        msg += "Commit or move them first, or use --force.";
        return Result::fail(msg);
    }
    for (const auto& p : toDelete)
        if (!removeFile(p)) return Result::fail("cannot delete " + p);
    for (const auto& p : toWrite) {
        std::string blob;
        if (!storage_.objects.readBlob(to.at(p), blob)) return Result::fail("missing object for " + p);
        if (!writeFile(p, blob)) return Result::fail("cannot write " + p);
    }
    return Result::success();
}