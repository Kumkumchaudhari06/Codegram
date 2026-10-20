#include "storage/config.h"
#include "utils/Fileio.h"
#include <sstream>

namespace {

std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

bool validValue(const std::string& v) {
    for (char c : v)
        if (c == '\n' || c == '\r' || c == '\0') return false;
    return true;
}

} // namespace

Config::Config(const std::string& repoDir) : file_(repoDir + "/config") {}

bool Config::isValidKey(const std::string& key) {
    if (key.empty() || key.size() > 100) return false;
    if (key.front() == '.' || key.back() == '.') return false;
    if (key.find("..") != std::string::npos) return false;
    for (char c : key) {
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
        if (!ok) return false;
    }
    return true;
}

bool Config::load() {
    if (!FileIO::exists(file_)) {   // a new repo has no config yet
        values_.clear();
        return true;
    }
    std::string raw;
    if (!FileIO::readFile(file_, raw)) return false;

    std::map<std::string, std::string> parsed;
    std::istringstream in(raw);
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;   // blank lines and comments

        size_t eq = line.find('=');
        if (eq == std::string::npos) return false;

        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));
        if (!isValidKey(key)) return false;
        if (!parsed.emplace(key, val).second) return false;   // duplicate key
    }
    values_.swap(parsed);
    return true;
}

bool Config::save() const {
    std::string out = "# CODEGRAM config\n";
    for (const auto& kv : values_)
        out += kv.first + " = " + kv.second + "\n";
    return FileIO::writeFile(file_, out);
}

bool Config::set(const std::string& key, const std::string& value) {
    std::string v = trim(value);
    if (!isValidKey(key) || !validValue(v)) return false;
    values_[key] = v;
    return true;
}

bool Config::remove(const std::string& key) { return values_.erase(key) > 0; }

bool Config::has(const std::string& key) const { return values_.count(key) > 0; }

std::string Config::get(const std::string& key, const std::string& fallback) const {
    auto it = values_.find(key);
    return it == values_.end() ? fallback : it->second;
}

std::vector<std::string> Config::keys() const {
    std::vector<std::string> result;
    for (const auto& kv : values_) result.push_back(kv.first);
    return result;
}