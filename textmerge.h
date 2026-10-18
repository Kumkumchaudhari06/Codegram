#pragma once
#include <string>
#include <vector>

// Three-way merge of text: base + ours + theirs
namespace TextMerge {

struct Segment {
    bool conflict = false;
    std::vector<std::string> lines;                  // plain text (when not a conflict)
    std::vector<std::string> base, ours, theirs;     // the three versions (when a conflict)
};

struct Merged {
    std::vector<Segment> segments;
    bool trailingNewline = true;
    size_t conflictCount() const;
    bool clean() const { return conflictCount() == 0; }
};

// One conflict block as the user sees it in the file with markers
struct Hunk {
    int index = 0;
    int startLine = 0;   // line of "<<<<<<<" in the marker file (1-based)
    int endLine = 0;     // line of ">>>>>>>"
    std::vector<std::string> base, ours, theirs;
};

enum class Choice { Ours, Theirs, Both };

struct Resolution {
    Choice choice = Choice::Ours;
    bool manual = false;                       // use manualLines instead of a choice
    std::vector<std::string> manualLines;
};

Merged merge3(const std::string& base, const std::string& ours, const std::string& theirs);

// The merged file with <<<<<<< ======= >>>>>>> markers around every conflict
std::string renderMarkers(const Merged& m, const std::string& oursLabel, const std::string& theirsLabel,
                          std::vector<Hunk>* hunks = nullptr);
// The merged file with every conflict decided (res must have one entry per conflict)
bool renderResolved(const Merged& m, const std::vector<Resolution>& res, std::string& out);
bool hasMarkers(const std::string& text);

}