
#ifndef DEPENDENCYGRAPH_H
#define DEPENDENCYGRAPH_H

#include <string>
#include <vector>
#include <map>
using namespace std;

class DependencyGraph
{
private:
    map<string, vector<string>> graph;

public:
    void addDependency(const string& a, const string& b);
    vector<string> getDependencies(const string& a);
};

#endif