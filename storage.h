#pragma once
#include <string>
#include "storage/objectstore.h"
#include "storage/refstore.h"
#include "storage/index.h"
#include "storage/config.h"

class Storage {
private:
    std::string dir_;   // declared first so it is ready before the members below

public:
    // repoDir is the .codegram folder, e.g. "myproject/.codegram"
    explicit Storage(const std::string& repoDir);

    // Creates the folder layout, HEAD, default config and an empty index.
    // Returns false if the repository already exists.
    bool init(const std::string& defaultBranch = "main");

    // Loads the config and the index and starts the log file. False if there is no repository.
    bool open();

    bool isInitialized() const;
    const std::string& dir() const;

    ObjectStore objects;
    RefStore refs;
    Index index;
    Config config;
};