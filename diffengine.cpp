#include "core/diffengine.h"
#include <algorithm>
#include <cstdint>

namespace {
const size_t MAX_TABLE_CELLS = 40000000;   // above this the middle part is treated as replaced
}

namespace DiffEngine {

std::vector<std::string> splitLines(const std::string& text, bool* endsWithNewline) {
    std::vector<std::string> lines;
    size_t start = 0;
    while (start < text.size()) {
        size_t nl = text.find('\n', start);
        if (nl == std::string::npos) { lines.push_back(text.substr(start)); break; }
        lines.push_back(text.substr(start, nl - start));
        start = nl + 1;
    }
    if (endsWithNewline) *endsWithNewline = !text.empty() && text.back() == '\n';
    return lines;
}

std::string joinLines(const std::vector<std::string>& lines, bool trailingNewline) {
    std::string out;
    for (size_t i = 0; i < lines.size(); ++i) {
        out += lines[i];
        if (i + 1 < lines.size() || trailingNewline) out += '\n';
    }
    return out;
}

bool isBinary(const std::string& text) { return text.find('\0') != std::string::npos; }

std::vector<std::pair<int, int>> matchLines(const std::vector<std::string>& a,
                                            const std::vector<std::string>& b) {
    std::vector<std::pair<int, int>> result;
    const int n = static_cast<int>(a.size()), m = static_cast<int>(b.size());

    int prefix = 0;
    while (prefix < n && prefix < m && a[prefix] == b[prefix]) ++prefix;
    int suffix = 0;
    while (suffix < n - prefix && suffix < m - prefix && a[n - 1 - suffix] == b[m - 1 - suffix]) ++suffix;

    for (int i = 0; i < prefix; ++i) result.push_back({i, i});

    const int an = n - prefix - suffix, bm = m - prefix - suffix;
    if (an > 0 && bm > 0 && static_cast<size_t>(an + 1) * static_cast<size_t>(bm + 1) <= MAX_TABLE_CELLS) {
        std::vector<uint32_t> L(static_cast<size_t>(an + 1) * (bm + 1), 0);
        auto at = [&](int i, int j) -> uint32_t& { return L[static_cast<size_t>(i) * (bm + 1) + j]; };
        for (int i = an - 1; i >= 0; --i)
            for (int j = bm - 1; j >= 0; --j)
                at(i, j) = (a[prefix + i] == b[prefix + j]) ? at(i + 1, j + 1) + 1
                                                            : std::max(at(i + 1, j), at(i, j + 1));
        int i = 0, j = 0;
        while (i < an && j < bm) {
            if (a[prefix + i] == b[prefix + j]) { result.push_back({prefix + i, prefix + j}); ++i; ++j; }
            else if (at(i + 1, j) >= at(i, j + 1)) ++i;
            else ++j;
        }
    }

    for (int t = 0; t < suffix; ++t) result.push_back({n - suffix + t, m - suffix + t});
    return result;
}

std::vector<DiffLine> diffLines(const std::vector<std::string>& a, const std::vector<std::string>& b) {
    std::vector<DiffLine> out;
    int i = 0, j = 0;
    auto emit = [&](LineOp op, const std::string& text) {
        DiffLine d;
        d.op = op;
        d.text = text;
        if (op != LineOp::Added) d.oldLine = i + 1;
        if (op != LineOp::Removed) d.newLine = j + 1;
        out.push_back(d);
    };
    for (const auto& mt : matchLines(a, b)) {
        while (i < mt.first)  { emit(LineOp::Removed, a[i]); ++i; }
        while (j < mt.second) { emit(LineOp::Added, b[j]); ++j; }
        emit(LineOp::Same, a[i]);
        ++i; ++j;
    }
    while (i < static_cast<int>(a.size())) { emit(LineOp::Removed, a[i]); ++i; }
    while (j < static_cast<int>(b.size())) { emit(LineOp::Added, b[j]); ++j; }
    return out;
}

std::string unifiedDiff(const std::vector<DiffLine>& d, int context) {
    std::string out;
    const int n = static_cast<int>(d.size());
    int i = 0;
    while (i < n) {
        while (i < n && d[i].op == LineOp::Same) ++i;
        if (i >= n) break;

        const int start = std::max(0, i - context);
        int lastChange = i, k = i;
        while (k < n) {
            if (d[k].op != LineOp::Same) { lastChange = k; ++k; continue; }
            int run = k;
            while (run < n && d[run].op == LineOp::Same) ++run;
            if (run < n && run - k <= 2 * context) k = run;   // next change is close: same hunk
            else break;
        }
        const int end = std::min(n - 1, lastChange + context);

        int oldCount = 0, newCount = 0, oldStart = 0, newStart = 0;
        for (int x = start; x <= end; ++x) {
            if (d[x].op != LineOp::Added) ++oldCount;
            if (d[x].op != LineOp::Removed) ++newCount;
            if (oldStart == 0 && d[x].oldLine > 0) oldStart = d[x].oldLine;
            if (newStart == 0 && d[x].newLine > 0) newStart = d[x].newLine;
        }
        if (oldCount == 0)
            for (int x = start - 1; x >= 0; --x) if (d[x].oldLine > 0) { oldStart = d[x].oldLine; break; }
        if (newCount == 0)
            for (int x = start - 1; x >= 0; --x) if (d[x].newLine > 0) { newStart = d[x].newLine; break; }

        out += "@@ -" + std::to_string(oldStart) + "," + std::to_string(oldCount) +
               " +" + std::to_string(newStart) + "," + std::to_string(newCount) + " @@\n";
        for (int x = start; x <= end; ++x) {
            out += (d[x].op == LineOp::Added) ? '+' : (d[x].op == LineOp::Removed) ? '-' : ' ';
            out += d[x].text;
            out += '\n';
        }
        i = end + 1;
    }
    return out;
}

} // namespace DiffEngine