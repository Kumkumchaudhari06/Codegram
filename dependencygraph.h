#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>

class DependencyGraph {
public:
    void addNode(const std::string& name);
    void addEdge(const std::string& from, const std::string& to);   // from depends on to
    bool hasNode(const std::string& name) const;
    size_t nodeCount() const;

    std::vector<std::string> dependenciesOf(const std::string& name) const;
    std::vector<std::string> dependentsOf(const std::string& name) const;

    // Every node that directly or indirectly depends on `changed` (DFS)
    std::vector<std::string> impactOf(const std::string& changed) const;

    // A dependency path from -> ... -> to, or empty if none (DFS)
    std::vector<std::string> findPath(const std::string& from, const std::string& to) const;

    // Scans .h/.hpp/.c/.cc/.cpp files under dir (no trailing slash) for #include "..."
    // Node names are paths relative to dir. Returns the number of nodes.
    size_t buildFromDirectory(const std::string& dir);

private:
    bool dfs(const std::string& cur, const std::string& target,
             std::set<std::string>& visited, std::vector<std::string>& path) const;

    std::map<std::string, std::set<std::string>> deps_;    // node -> what it depends on
    std::map<std::string, std::set<std::string>> rdeps_;   // node -> what depends on it
};