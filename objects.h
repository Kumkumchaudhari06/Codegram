#pragma once
#include <string>
#include <vector>

struct TreeEntry {
    std::string type;   // "blob" (file) or "tree" (folder)
    std::string hash;
    std::string name;
};

struct Tree {
    std::vector<TreeEntry> entries;
};

struct Commit {
    std::string hash;                  // filled in after hashing, not stored inside the text
    std::string treeHash;
    std::vector<std::string> parents;  // 0 = first commit, 2 = merge commit
    std::string author;
    long long timestamp = 0;
    std::string message;
};