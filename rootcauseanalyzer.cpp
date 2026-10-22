#include "rootcause/rootcauseanalyzer.h"
#include <algorithm>
#include <cctype>

namespace {

std::string stripSpaces(const std::string& s) {
    std::string r;
    for (unsigned char c : s)
        if (!std::isspace(c)) r.push_back(static_cast<char>(c));
    return r;
}

std::string describe(const std::string& label, const SideChange& c) {
    std::string h = c.commitHash.substr(0, 7);
    return label + " (commit " + h + " by " + c.author + ": \"" + c.message + "\")";
}

} // namespace

std::vector<RootCause> RootCauseAnalyzer::analyze(const ConflictEvidence& e) const {
    std::vector<RootCause> out;

    auto add = [&](const std::string& reason, int score, const std::string& text) {
        RootCause r;
        r.file = e.file;
        r.startLine = e.startLine;
        r.endLine = e.endLine;
        r.reason = reason;
        r.score = score;
        r.explanation = text;
        out.push_back(r);
    };

    std::string where = e.file + " lines " + std::to_string(e.startLine) + "-" +
                        std::to_string(e.endLine);
    std::string both = describe("ours", e.ours) + " and " + describe("theirs", e.theirs);

    bool oursMod = e.ours.kind == "modified", theirsMod = e.theirs.kind == "modified";
    bool oursDel = e.ours.kind == "deleted",  theirsDel = e.theirs.kind == "deleted";
    bool oursRen = e.ours.kind == "renamed",  theirsRen = e.theirs.kind == "renamed";

    if (oursMod && theirsMod) {
        if (e.oursText == e.theirsText) {
            add("identical-edit", 10,
                "Both branches made the same change at " + where + "; it should resolve automatically. " + both);
        } else if (stripSpaces(e.oursText) == stripSpaces(e.theirsText)) {
            add("whitespace-only", 30,
                "The two sides differ only in spaces or line breaks at " + where + ". " + both);
        } else {
            add("same-lines-edited", 85,
                "Both branches edited the same lines differently at " + where + ". " + both);
        }
    } else if ((oursDel && theirsMod) || (theirsDel && oursMod)) {
        add("delete-vs-modify", 90,
            "One branch deleted what the other branch changed at " + where + ". " + both);
    } else if ((oursRen && theirsMod) || (theirsRen && oursMod)) {
        add("rename-vs-modify", 80,
            "One branch renamed the file while the other edited it at " + where + ". " + both);
    } else {
        add("structural-change", 60,
            "Both branches changed the structure of " + e.file + " in incompatible ways. " + both);
    }

    if (graph_ && graph_->hasNode(e.file)) {
        std::vector<std::string> impact = graph_->impactOf(e.file);
        if (!impact.empty()) {
            std::string names;
            for (size_t i = 0; i < impact.size() && i < 3; ++i)
                names += (i ? ", " : "") + impact[i];
            add("downstream-impact", 40,
                std::to_string(impact.size()) + " other file(s) depend on " + e.file +
                ", for example " + names + ". Resolve this conflict carefully.");
        }
    }

    std::stable_sort(out.begin(), out.end(),
                     [](const RootCause& a, const RootCause& b) { return a.score > b.score; });
    return out;
}