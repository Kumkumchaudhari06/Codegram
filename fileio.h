#pragma once
#include <string>
#include <vector>

namespace FileIO {
    bool exists(const std::string& path);
    bool makeDir(const std::string& path);                       // creates parent dirs too
    bool writeFile(const std::string& path, const std::string& content);
    bool readFile(const std::string& path, std::string& out);    // false if it fails
    std::vector<std::string> listFiles(const std::string& dir);  // recursive, files only
}