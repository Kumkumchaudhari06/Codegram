#pragma once
#include <string>
#include <utility>
#include <vector>

enum class LineOp { Same, Added, Removed };

struct DiffLine {
    LineOp op = LineOp::Same;
    std::string text;
    int oldLine = 0;   // 1-based, 0 when the line is not in the old text
    int newLine = 0;
};

namespace DiffEngine {
    std::vector<std::string> splitLines(const std::string& text, bool* endsWithNewline = nullptr);
    std::string joinLines(const std::vector<std::string>& lines, bool trailingNewline);
    bool isBinary(const std::string& text);

    // Longest common subsequence of lines: pairs of (index in a, index in b), increasing
    std::vector<std::pair<int, int>> matchLines(const std::vector<std::string>& a,
                                                const std::vector<std::string>& b);
    std::vector<DiffLine> diffLines(const std::vector<std::string>& a, const std::vector<std::string>& b);
    // The "@@ -a,b +c,d @@" hunks with context lines
    std::string unifiedDiff(const std::vector<DiffLine>& diff, int context = 3);
}