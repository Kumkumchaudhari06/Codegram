#pragma once
#include <map>
#include <string>
#include "storage/objectstore.h"

class Index {
public:
    // repoDir is the .codegram folder; the index is saved as repoDir/index
    explicit Index(const std::string& repoDir);

    // Turns "\" into "/" and removes a leading "./"
    static std::string normalizePath(const std::string& path);

    // Relative paths only: no empty parts, no "." or "..", no ":" or line breaks,
    // and nothing inside ".codegram"
    static bool isValidPath(const std::string& path);
        // Aliases used by Staging / BranchManager / ConflictResolver
    static bool validPath(const std::string& path) { return isValidPath(path); }
    bool get(const std::string& path, std::string& hashOut) const {
        hashOut = hashOf(path);
        return !hashOut.empty();
    }
    bool set(const std::string& path, const std::string& blobHash) { return add(path, blobHash); }
    // load() treats a missing file as an empty index. If the file is damaged it
    // returns false and leaves the current entries unchanged.
    bool load();
    bool save() const;

    // Fails for an invalid path, an invalid hash, or a file/folder clash
    // (for example "a" when "a/b" is already staged)
    bool add(const std::string& path, const std::string& blobHash);
    bool remove(const std::string& path);
    void clear();

    bool has(const std::string& path) const;
    std::string hashOf(const std::string& path) const;   // "" if not staged
    size_t size() const;
    bool empty() const;
    const std::map<std::string, std::string>& entries() const;

    // Builds nested tree objects from the flat entries, returns the root tree hash ("" on failure)
    std::string writeTree(ObjectStore& store) const;

    // Replaces the entries with the contents of a tree (recursive)
    bool loadFromTree(const ObjectStore& store, const std::string& treeHash);

private:
    std::string indexFile_;
    std::map<std::string, std::string> entries_;   // path -> blob hash, sorted by path
};