#include "storage/index.h"
#include "storage/objects.h"
#include "utils/fileio.h"
#include <memory>
#include <sstream>

namespace {

bool validHash(const std::string& h) {
    if (h.size() != 64) return false;
    for (char c : h)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}

// One part of a path (a file or folder name)
bool validComponent(const std::string& s) {
    if (s.empty() || s == "." || s == "..") return false;
    for (char c : s)
        if (c == '/' || c == '\\' || c == ':' || c == '\n' || c == '\r' || c == '\0')
            return false;
    return true;
}

// ----- building nested trees from the flat map -----

struct Node {
    std::map<std::string, std::string> files;                 // name -> blob hash
    std::map<std::string, std::unique_ptr<Node>> dirs;        // name -> subfolder
};

std::string writeNode(ObjectStore& store, const Node& n) {
    Tree t;
    for (const auto& f : n.files)
        t.entries.push_back({"blob", f.second, f.first});
    for (const auto& d : n.dirs) {
        if (n.files.count(d.first)) return "";   // same name used as file and folder
        std::string h = writeNode(store, *d.second);
        if (h.empty()) return "";
        t.entries.push_back({"tree", h, d.first});
    }
    return store.writeTree(t);
}

// ----- reading nested trees back into a flat map -----

bool flatten(const ObjectStore& store, const std::string& treeHash, const std::string& prefix,
             int depth, std::map<std::string, std::string>& out) {
    if (depth > 100) return false;

    Tree t;
    if (!store.readTree(treeHash, t)) return false;

    for (const auto& e : t.entries) {
        if (!validComponent(e.name)) return false;
        std::string path = prefix.empty() ? e.name : prefix + "/" + e.name;

        if (e.type == "blob") {
            if (!Index::isValidPath(path) || !validHash(e.hash)) return false;
            if (!out.emplace(path, e.hash).second) return false;   // duplicate name
        } else if (e.type == "tree") {
            if (!flatten(store, e.hash, path, depth + 1, out)) return false;
        } else {
            return false;
        }
    }
    return true;
}

} // namespace

Index::Index(const std::string& repoDir) : indexFile_(repoDir + "/index") {}

std::string Index::normalizePath(const std::string& path) {
    std::string p = path;
    for (char& c : p)
        if (c == '\\') c = '/';
    while (p.compare(0, 2, "./") == 0) p.erase(0, 2);
    return p;
}

bool Index::isValidPath(const std::string& path) {
    if (path.empty()) return false;

    size_t start = 0;
    bool first = true;
    while (true) {
        size_t slash = path.find('/', start);
        std::string part = path.substr(start, slash == std::string::npos ? std::string::npos
                                                                         : slash - start);
        if (!validComponent(part)) return false;       // also catches "//" and a trailing "/"
        if (first && part == ".codegram") return false;
        first = false;
        if (slash == std::string::npos) break;
        start = slash + 1;
    }
    return true;
}

bool Index::load() {
    std::string raw;
    if (!FileIO::exists(indexFile_)) {   // a new repo has no index yet
        entries_.clear();
        return true;
    }
    if (!FileIO::readFile(indexFile_, raw)) return false;

    std::map<std::string, std::string> parsed;
    std::istringstream in(raw);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        // Line format: <64-char hash> <path>
        if (line.size() < 66 || line[64] != ' ') return false;
        std::string hash = line.substr(0, 64);
        std::string path = line.substr(65);
        if (!validHash(hash) || !isValidPath(path)) return false;
        if (!parsed.emplace(path, hash).second) return false;   // duplicate path
    }
    entries_.swap(parsed);
    return true;
}

bool Index::save() const {
    std::string out;
    for (const auto& e : entries_)
        out += e.second + " " + e.first + "\n";
    return FileIO::writeFile(indexFile_, out);
}

bool Index::add(const std::string& rawPath, const std::string& blobHash) {
    std::string path = normalizePath(rawPath);
    if (!isValidPath(path) || !validHash(blobHash)) return false;

    // A path cannot be both a file and a folder
    for (const auto& e : entries_) {
        const std::string& p = e.first;
        if (p == path) continue;
        if (p.size() > path.size() && p.compare(0, path.size(), path) == 0 &&
            p[path.size()] == '/') return false;
        if (path.size() > p.size() && path.compare(0, p.size(), p) == 0 &&
            path[p.size()] == '/') return false;
    }
    entries_[path] = blobHash;
    return true;
}

bool Index::remove(const std::string& path) {
    return entries_.erase(normalizePath(path)) > 0;
}

void Index::clear() { entries_.clear(); }

bool Index::has(const std::string& path) const {
    return entries_.count(normalizePath(path)) > 0;
}

std::string Index::hashOf(const std::string& path) const {
    auto it = entries_.find(normalizePath(path));
    return it == entries_.end() ? "" : it->second;
}

size_t Index::size() const { return entries_.size(); }
bool Index::empty() const { return entries_.empty(); }

const std::map<std::string, std::string>& Index::entries() const { return entries_; }

std::string Index::writeTree(ObjectStore& store) const {
    Node root;
    for (const auto& e : entries_) {
        // Split "src/utils/util.h" into parts and walk down, creating folders as needed
        Node* cur = &root;
        size_t start = 0;
        while (true) {
            size_t slash = e.first.find('/', start);
            if (slash == std::string::npos) {
                cur->files[e.first.substr(start)] = e.second;
                break;
            }
            std::unique_ptr<Node>& next = cur->dirs[e.first.substr(start, slash - start)];
            if (!next) next.reset(new Node());
            cur = next.get();
            start = slash + 1;
        }
    }
    return writeNode(store, root);
}

bool Index::loadFromTree(const ObjectStore& store, const std::string& treeHash) {
    std::map<std::string, std::string> result;
    if (!flatten(store, treeHash, "", 0, result)) return false;
    entries_.swap(result);
    return true;
}