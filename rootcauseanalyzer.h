#pragma once
#include <string>
#include <vector>
#include "utils/dependencygraph.h"

struct SideChange {
    std::string commitHash;
    std::string author;
    std::string message;
    std::string kind;   // "modified", "deleted" or "renamed"
};

struct ConflictEvidence {
    std::string file;
    int startLine = 0;
    int endLine = 0;
    std::string oursText;
    std::string theirsText;
    SideChange ours;
    SideChange theirs;
};

struct RootCause {
    std::string file;
    int startLine = 0;
    int endLine = 0;
    std::string reason;        // short rule name
    int score = 0;             // 0-100, higher = more likely the real cause
    std::string explanation;   // sentence shown to the user
};

class RootCauseAnalyzer {
public:
    // graph is optional; when given, downstream impact is reported too
    explicit RootCauseAnalyzer(const DependencyGraph* graph = nullptr) : graph_(graph) {}

    // Causes sorted from most to least likely
    std::vector<RootCause> analyze(const ConflictEvidence& e) const;

private:
    const DependencyGraph* graph_;
};