
#include "DependencyGraph.h"

void DependencyGraph::addDependency(const string& a, const string& b)
{
    graph[a].push_back(b);
}

vector<string> DependencyGraph::getDependencies(const string& a)
{
    if (graph.find(a) != graph.end())
        return graph[a];

    return {};
}