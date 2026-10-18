#include "core/commitmanager.h"
#include "utils/fileio.h"
#include "utils/logger.h"
#include <ctime>
#include <filesystem>
#include <map>
#include <queue>

namespace fs = std::filesystem;

CommitManager::CommitManager(Storage& storage) : storage_(storage) {}

std::string CommitManager::firstLine(const std::string& m) { return m.substr(0, m.find('\n')); }
std::string CommitManager::shortHash(const std::string& h) { return h.substr(0, 7); }

std::string CommitManager::formatTime(long long ts) {
    std::time_t t = static_cast<std::time_t>(ts);
    std::tm tmv{};
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
    return buf;
}

bool CommitManager::read(const std::string& hash, Commit& out) const {
    return storage_.objects.readCommit(hash, out);
}

Result CommitManager::commit(const std::string& message, const std::vector<std::string>& extraParents,
                             std::string* newHash, bool allowEmpty) {
    std::string msg = message;
    while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r' || msg.back() == ' ')) msg.pop_back();
    if (msg.empty()) return Result::fail("a commit message is required");

    std::string tree = storage_.index.writeTree(storage_.objects);
    if (tree.empty()) return Result::fail("cannot build the commit tree");

    const std::string parent = storage_.refs.headCommit();
    if (!allowEmpty && extraParents.empty()) {
        if (parent.empty()) {
            if (storage_.index.empty()) return Result::fail("nothing to commit: stage some files first");
        } else {
            Commit head;
            if (read(parent, head) && head.treeHash == tree) return Result::fail("nothing to commit: no staged changes");
        }
    }

    Commit c;
    c.treeHash = tree;
    if (!parent.empty()) c.parents.push_back(parent);
    for (const auto& p : extraParents) if (p != parent) c.parents.push_back(p);
    std::string name = storage_.config.get("user.name", "Unknown"), email = storage_.config.get("user.email", "");
    c.author = name + (email.empty() ? "" : " <" + email + ">");
    c.timestamp = static_cast<long long>(std::time(nullptr));
    c.message = msg;

    std::string h = storage_.objects.writeCommit(c);
    if (h.empty() || !storage_.refs.updateHead(h) || !storage_.index.save())
        return Result::fail("cannot write the commit");
    if (newHash) *newHash = h;
    Logger::info("commit " + shortHash(h) + ": " + firstLine(msg));

    std::string where = storage_.refs.currentBranch().empty() ? "detached HEAD" : storage_.refs.currentBranch();
    return Result::success("[" + where + " " + shortHash(h) + "] " + firstLine(msg));
}

std::vector<Commit> CommitManager::log(const std::string& start, size_t limit) const {
    std::vector<Commit> out;
    if (start.empty()) return out;
    std::map<std::string, Commit> cache;
    std::set<std::string> seen;
    std::priority_queue<std::pair<long long, std::string>> queue;   // newest first

    auto load = [&](const std::string& h) {
        if (cache.count(h)) return true;
        Commit c;
        if (!read(h, c)) return false;
        cache[h] = c;
        return true;
    };
    if (!load(start)) return out;
    queue.push({cache[start].timestamp, start});
    seen.insert(start);
    while (!queue.empty() && (limit == 0 || out.size() < limit)) {
        std::string h = queue.top().second;
        queue.pop();
        const Commit c = cache[h];
        out.push_back(c);
        for (const auto& p : c.parents)
            if (seen.insert(p).second && load(p)) queue.push({cache[p].timestamp, p});
    }
    return out;
}

std::set<std::string> CommitManager::ancestors(const std::string& hash, bool includeSelf) const {
    std::set<std::string> seen;
    std::vector<std::string> stack;
    if (hash.empty()) return seen;
    stack.push_back(hash);
    while (!stack.empty()) {
        std::string h = stack.back();
        stack.pop_back();
        if (!seen.insert(h).second) continue;
        Commit c;
        if (read(h, c)) for (const auto& p : c.parents) stack.push_back(p);
    }
    if (!includeSelf) seen.erase(hash);
    return seen;
}

bool CommitManager::isAncestor(const std::string& maybeAncestor, const std::string& descendant) const {
    if (maybeAncestor.empty() || descendant.empty()) return false;
    return ancestors(descendant, true).count(maybeAncestor) > 0;
}

std::string CommitManager::mergeBase(const std::string& a, const std::string& b) const {
    if (a.empty() || b.empty()) return "";
    std::set<std::string> A = ancestors(a, true), B = ancestors(b, true);
    std::vector<std::string> common;
    for (const auto& h : A) if (B.count(h)) common.push_back(h);
    if (common.empty()) return "";

    // A common commit that is an ancestor of another common commit is not the closest one
    std::set<std::string> dominated;
    std::vector<std::string> stack;
    for (const auto& h : common) {
        Commit c;
        if (read(h, c)) for (const auto& p : c.parents) stack.push_back(p);
    }
    while (!stack.empty()) {
        std::string h = stack.back();
        stack.pop_back();
        if (!dominated.insert(h).second) continue;
        Commit c;
        if (read(h, c)) for (const auto& p : c.parents) stack.push_back(p);
    }

    std::string best;
    long long bestTime = -1;
    for (const auto& h : common) {
        if (dominated.count(h)) continue;
        Commit c;
        if (!read(h, c)) continue;
        if (best.empty() || c.timestamp > bestTime) { best = h; bestTime = c.timestamp; }
    }
    return best;
}

std::string CommitManager::resolveBase(const std::string& name) const {
    if (name.empty()) return "";
    if (name == "HEAD") return storage_.refs.headCommit();
    if (storage_.refs.branchExists(name)) return storage_.refs.branchCommit(name);

    for (char c : name)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return "";
    Commit tmp;
    if (name.size() == 64) return read(name, tmp) ? name : "";
    if (name.size() < 4) return "";

    std::string found;
    int count = 0;
    for (const auto& f : FileIO::listFiles(storage_.dir() + "/objects")) {
        fs::path p(f);
        std::string h = p.parent_path().filename().string() + p.filename().string();
        if (h.compare(0, name.size(), name) == 0 && read(h, tmp)) { found = h; ++count; }
    }
    return count == 1 ? found : "";
}

std::string CommitManager::resolve(const std::string& spec) const {
    std::string name = spec;
    int back = 0;
    size_t tilde = name.find('~');
    if (tilde != std::string::npos) {
        std::string num = name.substr(tilde + 1);
        name = name.substr(0, tilde);
        try { back = num.empty() ? 1 : std::stoi(num); } catch (...) { return ""; }
        if (back < 0) return "";
    }
    std::string hash = resolveBase(name);
    for (int i = 0; i < back && !hash.empty(); ++i) {
        Commit c;
        if (!read(hash, c) || c.parents.empty()) return "";
        hash = c.parents[0];
    }
    return hash;
}