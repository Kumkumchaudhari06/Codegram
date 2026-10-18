#include "core/repomanager.h"
#include "storage/refstore.h"
#include "storage/storage.h"
#include "utils/fileio.h"
#include <filesystem>

namespace fs = std::filesystem;

std::string RepoManager::cleanPath(const std::string& dir) {
    std::string s = fs::absolute(dir).lexically_normal().generic_string();
    while (s.size() > 1 && s.back() == '/') s.pop_back();
    return s;
}

std::string RepoManager::findRoot(const std::string& startDir) {
    std::error_code ec;
    fs::path p(cleanPath(startDir));
    while (true) {
        if (fs::is_directory(p / ".codegram", ec)) return p.generic_string();
        fs::path parent = p.parent_path();
        if (parent.empty() || parent == p) return "";
        p = parent;
    }
}

Result RepoManager::init(const std::string& dir, const std::string& defaultBranch) {
    if (!RefStore::isValidBranchName(defaultBranch)) return Result::fail("invalid branch name: " + defaultBranch);
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) return Result::fail("cannot create folder " + dir);

    const std::string root = cleanPath(dir);
    Storage storage(root + "/.codegram");
    if (!storage.init(defaultBranch))
        return Result::fail(storage.isInitialized() ? "this folder is already a CODEGRAM repository"
                                                    : "cannot create the repository");
    if (!FileIO::exists(root + "/.codegramignore"))
        FileIO::writeFile(root + "/.codegramignore", "# files and folders CODEGRAM should not track\nbuild/\n*.o\n*.exe\n");
    return Result::success("Initialized empty CODEGRAM repository in " + root + "/.codegram");
}