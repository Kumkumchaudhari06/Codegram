#include "storage/storage.h"
#include "utils/fileio.h"
#include "utils/logger.h"

Storage::Storage(const std::string& repoDir)
    : dir_(repoDir), objects(repoDir), refs(repoDir), index(repoDir), config(repoDir) {}

const std::string& Storage::dir() const { return dir_; }

bool Storage::isInitialized() const {
    return FileIO::exists(dir_ + "/HEAD") &&
           FileIO::exists(dir_ + "/objects") &&
           FileIO::exists(dir_ + "/refs/heads");
}

bool Storage::init(const std::string& defaultBranch) {
    if (isInitialized()) return false;

    if (!FileIO::makeDir(dir_ + "/objects") ||
        !FileIO::makeDir(dir_ + "/refs/heads") ||
        !FileIO::makeDir(dir_ + "/logs") ||
        !FileIO::makeDir(dir_ + "/backup")) return false;

    if (!refs.initHead(defaultBranch)) return false;

    config.set("core.defaultBranch", defaultBranch);
    config.set("user.name", "Unknown");
    config.set("user.email", "");
    if (!config.save()) return false;

    index.clear();
    if (!index.save()) return false;

    Logger::init(dir_ + "/logs/codegram.log");
    Logger::info("Initialized repository in " + dir_);
    return true;
}

bool Storage::open() {
    if (!isInitialized()) return false;
    Logger::init(dir_ + "/logs/codegram.log");
    if (!config.load()) {
        Logger::error("config file is damaged");
        return false;
    }
    if (!index.load()) {
        Logger::error("index file is damaged");
        return false;
    }
    return true;
}