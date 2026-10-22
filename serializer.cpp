#include "storage/serializer.h"
#include <sstream>
#include <algorithm>

namespace Serializer {

// Tree format, one entry per line:  <type> <hash> <name>
std::string serializeTree(const Tree& tree) {
    std::vector<TreeEntry> sorted = tree.entries;
    // Sort by name so the same folder always produces the same hash
    std::sort(sorted.begin(), sorted.end(),
              [](const TreeEntry& a, const TreeEntry& b) { return a.name < b.name; });

    std::ostringstream out;
    for (const auto& e : sorted)
        out << e.type << ' ' << e.hash << ' ' << e.name << '\n';
    return out.str();
}

bool parseTree(const std::string& data, Tree& out) {
    out.entries.clear();
    std::istringstream in(data);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        size_t p1 = line.find(' ');
        if (p1 == std::string::npos) return false;
        size_t p2 = line.find(' ', p1 + 1);
        if (p2 == std::string::npos) return false;

        TreeEntry e;
        e.type = line.substr(0, p1);
        e.hash = line.substr(p1 + 1, p2 - p1 - 1);
        e.name = line.substr(p2 + 1);   // name is last, so it may contain spaces
        out.entries.push_back(e);
    }
    return true;
}

// Commit format:
//   tree <hash>
//   parent <hash>      (zero or more)
//   author <name>
//   time <number>
//   <blank line>
//   <message>
std::string serializeCommit(const Commit& c) {
    std::ostringstream out;
    out << "tree " << c.treeHash << '\n';
    for (const auto& p : c.parents)
        out << "parent " << p << '\n';
    out << "author " << c.author << '\n';
    out << "time " << c.timestamp << '\n';
    out << '\n' << c.message;
    return out.str();
}

bool parseCommit(const std::string& data, Commit& out) {
    out = Commit();

    // Header and message are separated by the first blank line
    size_t split = data.find("\n\n");
    std::string header = (split == std::string::npos) ? data : data.substr(0, split);
    if (split != std::string::npos)
        out.message = data.substr(split + 2);

    std::istringstream in(header);
    std::string line;
    while (std::getline(in, line)) {
        size_t sp = line.find(' ');
        if (sp == std::string::npos) continue;
        std::string key = line.substr(0, sp);
        std::string val = line.substr(sp + 1);

        if (key == "tree")        out.treeHash = val;
        else if (key == "parent") out.parents.push_back(val);
        else if (key == "author") out.author = val;
        else if (key == "time") {
            try { out.timestamp = std::stoll(val); }
            catch (...) { return false; }
        }
    }
    return !out.treeHash.empty();   // a commit without a tree is invalid
}

} // namespace Serializer