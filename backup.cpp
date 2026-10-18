#include "utils/backup.h"
#include <algorithm>
#include <ctime>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

std::string stamp() {
    std::time_t now = std::time(nullptr);
    std::tm tmv{};
#ifdef _WIN32
    localtime_s(&tmv, &now);
#else
    localtime_r(&now, &tmv);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tmv);
    return buf;
}

bool isInside(const fs::path& p, const fs::path& root) {
    std::error_code ec;
    fs::path a = fs::weakly_canonical(p, ec);
    if (ec) return false;
    fs::path b = fs::weakly_canonical(root, ec);
    if (ec) return false;
    fs::path rel = a.lexically_relative(b);
    return !rel.empty() && *rel.begin() != "..";
}

bool copyTree(const fs::path& src, const fs::path& dst, const fs::path& skip) {
    std::error_code ec;
    fs::create_directories(dst, ec);
    if (ec) return false;

    for (auto it = fs::recursive_directory_iterator(src, ec);
         it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) return false;
        const fs::path& p = it->path();

        if (!skip.empty() && isInside(p, skip)) {
            if (it->is_directory()) it.disable_recursion_pending();
            continue;
        }

        fs::path target = dst / p.lexically_relative(src);
        if (it->is_directory()) {
            fs::create_directories(target, ec);
        } else if (it->is_regular_file()) {
            fs::create_directories(target.parent_path(), ec);
            fs::copy_file(p, target, fs::copy_options::overwrite_existing, ec);
        }
        if (ec) return false;
    }
    return !ec;
}

} // namespace

namespace Backup {

std::string create(const std::string& sourceDir, const std::string& backupRoot) {
    std::error_code ec;
    if (!fs::is_directory(sourceDir, ec)) return "";

    std::string base = backupRoot + "/snapshot_" + stamp();
    std::string name = base;
    int n = 1;
    while (fs::exists(name, ec)) name = base + "_" + std::to_string(n++);

    if (!copyTree(sourceDir, name, backupRoot)) return "";
    return name;
}

std::vector<std::string> list(const std::string& backupRoot) {
    std::vector<std::string> result;
    std::error_code ec;
    if (!fs::is_directory(backupRoot, ec)) return result;
    for (const auto& e : fs::directory_iterator(backupRoot, ec)) {
        if (e.is_directory() && e.path().filename().string().rfind("snapshot_", 0) == 0)
            result.push_back(e.path().generic_string());
    }
    std::sort(result.begin(), result.end());
    return result;
}

bool restore(const std::string& snapshotDir, const std::string& targetDir) {
    std::error_code ec;
    if (!fs::is_directory(snapshotDir, ec)) return false;   // check BEFORE deleting anything

    std::vector<fs::path> toRemove;
    if (fs::is_directory(targetDir, ec)) {
        for (const auto& e : fs::directory_iterator(targetDir, ec))
            if (e.path().filename() != "backup") toRemove.push_back(e.path());
    }
    for (const auto& p : toRemove) {
        fs::remove_all(p, ec);
        if (ec) return false;
    }
    return copyTree(snapshotDir, targetDir, fs::path());
}

} // namespace Backup