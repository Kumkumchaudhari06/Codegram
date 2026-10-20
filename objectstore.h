#pragma once
#include <string>
#include "storage/objects.h"

class ObjectStore {
public:
    // repoDir is the .codegram folder, e.g. "myproject/.codegram"
    explicit ObjectStore(const std::string& repoDir);

    // Computes the hash an object would get, without writing anything. Returns "" for a bad type.
    static std::string hashObject(const std::string& type, const std::string& content);

    // Generic: type is "blob", "tree" or "commit". Returns the hash, or "" on failure.
    std::string writeObject(const std::string& type, const std::string& content);
    bool readObject(const std::string& hash, std::string& type, std::string& content) const;
    bool exists(const std::string& hash) const;

    std::string writeBlob(const std::string& content);
    bool readBlob(const std::string& hash, std::string& out) const;

    std::string writeTree(const Tree& tree);
    bool readTree(const std::string& hash, Tree& out) const;

    std::string writeCommit(const Commit& commit);
    bool readCommit(const std::string& hash, Commit& out) const;

private:
    std::string pathFor(const std::string& hash) const;
    std::string objectsDir_;
};