#include "utils/dependencyGraph.h"
#include "utils/FileIO.h"
#include <filesystem>
#include <sstream>
#include <stack>

namespace fs = std::filesystem;

void DependencyGraph::addNode(const std::string& name) {
    deps_[name];
    rdeps_[name];
}

void DependencyGraph::addEdge(const std::string& from, const std::string& to) {
    addNode(from);
    addNode(to);
    deps_[from].insert(to);
    rdeps_[to].insert(from);
}

bool DependencyGraph::hasNode(const std::string& name) const {
    return deps_.count(name) > 0;
}

size_t DependencyGraph::nodeCount() const { return deps_.size(); }

std::vector<std::string> DependencyGraph::dependenciesOf(const std::string& name) const {
    auto it = deps_.find(name);
    if (it == deps_.end()) return {};
    return std::vector<std::string>(it->second.begin(), it->second.end());
}

std::vector<std::string> DependencyGraph::dependentsOf(const std::string& name) const {
    auto it = rdeps_.find(name);
    if (it == rdeps_.end()) return {};
    return std::vector<std::string>(it->second.begin(), it->second.end());
}

std::vector<std::string> DependencyGraph::impactOf(const std::string& changed) const {
    if (!hasNode(changed)) return {};
    std::set<std::string> visited{changed};
    std::stack<std::string> st;
    st.push(changed);
    while (!st.empty()) {
        std::string cur = st.top();
        st.pop();
        for (const auto& d : rdeps_.at(cur))
            if (visited.insert(d).second) st.push(d);
    }
    visited.erase(changed);
    return std::vector<std::string>(visited.begin(), visited.end());
}

bool DependencyGraph::dfs(const std::string& cur, const std::string& target,
                          std::set<std::string>& visited, std::vector<std::string>& path) const {
    if (cur == target) return true;
    for (const auto& next : deps_.at(cur)) {
        if (visited.insert(next).second) {
            path.push_back(next);
            if (dfs(next, target, visited, path)) return true;
            path.pop_back();
        }
    }
    return false;
}

std::vector<std::string> DependencyGraph::findPath(const std::string& from,
                                                   const std::string& to) const {
    if (!hasNode(from) || !hasNode(to)) return {};
    std::set<std::string> visited{from};
    std::vector<std::string> path{from};
    if (dfs(from, to, visited, path)) return path;
    return {};
}

size_t DependencyGraph::buildFromDirectory(const std::string& dir) {
    std::map<std::string, std::string> files;   // relative name -> full path
    for (const auto& f : FileIO::listFiles(dir)) {
        std::string ext = fs::path(f).extension().string();
        if (ext != ".h" && ext != ".hpp" && ext != ".c" && ext != ".cc" && ext != ".cpp")
            continue;
        std::string rel = fs::path(f).lexically_relative(dir).generic_string();
        files[rel] = f;
        addNode(rel);
    }

    for (const auto& entry : files) {
        const std::string& rel = entry.first;
        std::string content;
        if (!FileIO::readFile(entry.second, content)) continue;

        std::istringstream in(content);
        std::string line;
        while (std::getline(in, line)) {
            size_t s = line.find_first_not_of(" \t");
            if (s == std::string::npos || line.compare(s, 8, "#include") != 0) continue;
            size_t q1 = line.find('"');
            if (q1 == std::string::npos) continue;
            size_t q2 = line.find('"', q1 + 1);
            if (q2 == std::string::npos) continue;

            std::string inc = line.substr(q1 + 1, q2 - q1 - 1);
            std::string c1 = fs::path(inc).lexically_normal().generic_string();
            std::string c2 = (fs::path(rel).parent_path() / inc).lexically_normal().generic_string();

            if (files.count(c1) && c1 != rel)      addEdge(rel, c1);
            else if (files.count(c2) && c2 != rel) addEdge(rel, c2);
        }
    }
    return nodeCount();
}