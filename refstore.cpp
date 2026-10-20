#include "storage/refstore.h"
#include "utils/fileio.h"
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

const std::string HEAD_PREFIX = "ref: refs/heads/";

bool validHash(const std::string& h) {
    if (h.size() != 64) return false;
    for (char c : h)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}

std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

bool readTrimmed(const std::string& path, std::string& out) {
    std::string raw;
    if (!FileIO::readFile(path, raw)) return false;
    out = trim(raw);
    return true;
}

bool isFile(const std::string& path) {
    std::error_code ec;
    return fs::is_regular_file(path, ec);
}

} // namespace

RefStore::RefStore(const std::string& repoDir)
    : headFile_(repoDir + "/HEAD"), headsDir_(repoDir + "/refs/heads") {}

std::string RefStore::branchPath(const std::string& name) const {
    return headsDir_ + "/" + name;
}

bool RefStore::isValidBranchName(const std::string& name) {
    if (name.empty() || name.size() > 200 || name == "HEAD") return false;
    if (name.front() == '-' || name.front() == '/' || name.back() == '/') return false;
    if (name.find("..") != std::string::npos) return false;

    for (char c : name) {
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-' || c == '/';
        if (!ok) return false;
    }

    // Check each part between slashes
    size_t start = 0;
    while (true) {
        size_t slash = name.find('/', start);
        std::string part = name.substr(start, slash == std::string::npos ? std::string::npos
                                                                         : slash - start);
        if (part.empty() || part[0] == '.') return false;
        if (part.size() >= 5 && part.compare(part.size() - 5, 5, ".lock") == 0) return false;
        if (slash == std::string::npos) break;
        start = slash + 1;
    }
    return true;
}

// ---------- HEAD ----------

bool RefStore::initHead(const std::string& branch) {
    if (!FileIO::makeDir(headsDir_)) return false;
    return setHeadToBranch(branch);
}

bool RefStore::setHeadToBranch(const std::string& branch) {
    if (!isValidBranchName(branch)) return false;
    return FileIO::writeFile(headFile_, HEAD_PREFIX + branch + "\n");
}

bool RefStore::setHeadDetached(const std::string& commitHash) {
    if (!validHash(commitHash)) return false;
    return FileIO::writeFile(headFile_, commitHash + "\n");
}

bool RefStore::isDetached() const {
    std::string s;
    return readTrimmed(headFile_, s) && validHash(s);
}

std::string RefStore::currentBranch() const {
    std::string s;
    if (!readTrimmed(headFile_, s)) return "";
    if (s.compare(0, HEAD_PREFIX.size(), HEAD_PREFIX) != 0) return "";
    std::string name = s.substr(HEAD_PREFIX.size());
    return isValidBranchName(name) ? name : "";
}

std::string RefStore::headCommit() const {
    std::string branch = currentBranch();
    if (!branch.empty()) return branchCommit(branch);

    std::string s;
    if (readTrimmed(headFile_, s) && validHash(s)) return s;
    return "";
}

bool RefStore::updateHead(const std::string& commitHash) {
    std::string branch = currentBranch();
    if (!branch.empty()) return setBranch(branch, commitHash);
    if (isDetached()) return setHeadDetached(commitHash);
    return false;
}

// ---------- Branches ----------

bool RefStore::branchExists(const std::string& name) const {
    return isValidBranchName(name) && isFile(branchPath(name));
}

std::string RefStore::branchCommit(const std::string& name) const {
    if (!isValidBranchName(name)) return "";
    std::string s;
    if (readTrimmed(branchPath(name), s) && validHash(s)) return s;
    return "";
}

bool RefStore::setBranch(const std::string& name, const std::string& commitHash) {
    if (!isValidBranchName(name) || !validHash(commitHash)) return false;
    return FileIO::writeFile(branchPath(name), commitHash + "\n");
}

bool RefStore::createBranch(const std::string& name, const std::string& commitHash) {
    if (branchExists(name)) return false;
    return setBranch(name, commitHash);
}

bool RefStore::deleteBranch(const std::string& name) {
    if (!branchExists(name)) return false;
    if (name == currentBranch()) return false;   // never delete the branch you are on

    std::error_code ec;
    fs::remove(branchPath(name), ec);
    if (ec) return false;

    // Remove empty folders left behind (e.g. "feature/" after deleting "feature/login")
    fs::path stop(headsDir_);
    fs::path p = fs::path(branchPath(name)).parent_path();
    while (p != stop && fs::is_empty(p, ec) && !ec) {
        fs::remove(p, ec);
        if (ec) break;
        p = p.parent_path();
    }
    return true;
}

std::vector<std::string> RefStore::listBranches() const {
    std::vector<std::string> result;
    for (const auto& f : FileIO::listFiles(headsDir_)) {
        std::string rel = fs::path(f).lexically_relative(headsDir_).generic_string();
        if (isValidBranchName(rel)) result.push_back(rel);
    }
    std::sort(result.begin(), result.end());
    return result;
}