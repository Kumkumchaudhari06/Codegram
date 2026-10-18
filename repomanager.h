#pragma once
#include <string>
#include "core/types.h"

class RepoManager {
public:
    // Looks in startDir and its parent folders for a ".codegram" folder; returns the project folder or ""
    static std::string findRoot(const std::string& startDir);
    static Result init(const std::string& dir, const std::string& defaultBranch = "main");
    static std::string cleanPath(const std::string& dir);   // absolute, "/" separators, no trailing "/"
};