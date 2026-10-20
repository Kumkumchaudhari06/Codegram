#pragma once
#include <map>
#include <string>
#include <vector>

class Config {
public:
    // repoDir is the .codegram folder; settings are saved as repoDir/config
    explicit Config(const std::string& repoDir);

    // Letters, digits, . _ - only; no leading/trailing dot, no ".."
    static bool isValidKey(const std::string& key);

    // load() treats a missing file as empty. A damaged file returns false and
    // leaves the current values unchanged.
    bool load();
    bool save() const;

    // The value is trimmed and may not contain line breaks
    bool set(const std::string& key, const std::string& value);
    bool remove(const std::string& key);
    bool has(const std::string& key) const;
    std::string get(const std::string& key, const std::string& fallback = "") const;
    std::vector<std::string> keys() const;   // sorted

private:
    std::string file_;
    std::map<std::string, std::string> values_;
};