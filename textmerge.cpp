#include "merge/textmerge.h"
#include "core/diffengine.h"

namespace TextMerge {

size_t Merged::conflictCount() const {
    size_t n = 0;
    for (const auto& s : segments) if (s.conflict) ++n;
    return n;
}

namespace {

typedef std::vector<std::string> Lines;

Merged merge3Lines(const Lines& B, const Lines& O, const Lines& T) {
    Merged m;
    const int nb = static_cast<int>(B.size()), no = static_cast<int>(O.size()), nt = static_cast<int>(T.size());

    // Which line of ours / theirs each base line corresponds to (-1 = changed or removed)
    std::vector<int> mo(nb, -1), mt(nb, -1);
    for (const auto& p : DiffEngine::matchLines(B, O)) mo[p.first] = p.second;
    for (const auto& p : DiffEngine::matchLines(B, T)) mt[p.first] = p.second;

    auto pushText = [&](const Lines& lines) {
        if (lines.empty()) return;
        if (m.segments.empty() || m.segments.back().conflict) m.segments.push_back(Segment());
        m.segments.back().lines.insert(m.segments.back().lines.end(), lines.begin(), lines.end());
    };

    int pb = -1, po = -1, pt = -1;   // last line all three versions agree on
    auto flush = [&](int b1, int o1, int t1) {
        Lines bc(B.begin() + (pb + 1), B.begin() + b1);
        Lines oc(O.begin() + (po + 1), O.begin() + o1);
        Lines tc(T.begin() + (pt + 1), T.begin() + t1);
        if (bc == oc)      pushText(tc);   // only theirs changed this part (or nothing changed)
        else if (bc == tc) pushText(oc);   // only ours changed this part
        else if (oc == tc) pushText(oc);   // both made the same change
        else {                             // both changed it differently
            Segment s;
            s.conflict = true;
            s.base = bc; s.ours = oc; s.theirs = tc;
            m.segments.push_back(s);
        }
    };

    for (int i = 0; i < nb; ++i) {
        if (mo[i] < 0 || mt[i] < 0) continue;
        flush(i, mo[i], mt[i]);
        pushText(Lines(1, B[i]));
        pb = i; po = mo[i]; pt = mt[i];
    }
    flush(nb, no, nt);
    return m;
}

void append(Lines& out, const Lines& more) { out.insert(out.end(), more.begin(), more.end()); }

} // namespace

Merged merge3(const std::string& base, const std::string& ours, const std::string& theirs) {
    bool bn = false, on = false, tn = false;
    Lines B = DiffEngine::splitLines(base, &bn), O = DiffEngine::splitLines(ours, &on),
          T = DiffEngine::splitLines(theirs, &tn);
    Merged m = merge3Lines(B, O, T);
    m.trailingNewline = (on == bn) ? tn : on;   // three-way decision for the final newline too
    return m;
}

std::string renderMarkers(const Merged& m, const std::string& oursLabel, const std::string& theirsLabel,
                          std::vector<Hunk>* hunks) {
    Lines out;
    int index = 0;
    for (const auto& seg : m.segments) {
        if (!seg.conflict) { append(out, seg.lines); continue; }
        Hunk h;
        h.index = index++;
        h.startLine = static_cast<int>(out.size()) + 1;
        out.push_back("<<<<<<< " + oursLabel);
        append(out, seg.ours);
        out.push_back("=======");
        append(out, seg.theirs);
        out.push_back(">>>>>>> " + theirsLabel);
        h.endLine = static_cast<int>(out.size());
        h.base = seg.base; h.ours = seg.ours; h.theirs = seg.theirs;
        if (hunks) hunks->push_back(h);
    }
    return DiffEngine::joinLines(out, m.trailingNewline);
}

bool renderResolved(const Merged& m, const std::vector<Resolution>& res, std::string& out) {
    if (res.size() != m.conflictCount()) return false;
    Lines lines;
    size_t k = 0;
    for (const auto& seg : m.segments) {
        if (!seg.conflict) { append(lines, seg.lines); continue; }
        const Resolution& r = res[k++];
        if (r.manual) append(lines, r.manualLines);
        else if (r.choice == Choice::Ours) append(lines, seg.ours);
        else if (r.choice == Choice::Theirs) append(lines, seg.theirs);
        else { append(lines, seg.ours); append(lines, seg.theirs); }
    }
    out = DiffEngine::joinLines(lines, m.trailingNewline);
    return true;
}

bool hasMarkers(const std::string& text) {
    bool open = false, close = false;
    for (const auto& line : DiffEngine::splitLines(text)) {
        if (line.compare(0, 7, "<<<<<<<") == 0) open = true;
        if (line.compare(0, 7, ">>>>>>>") == 0) close = true;
    }
    return open && close;
}

} // namespace TextMerge