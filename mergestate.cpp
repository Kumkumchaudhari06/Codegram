#include "merge/mergestate.h"
#include "core/diffengine.h"
#include "storage/index.h"
#include "utils/fileio.h"
#include <filesystem>

namespace {

bool okHash(const std::string& h) {
    if (h == "-") return true;
    if (h.size() != 64) return false;
    for (char c : h) if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}
std::string enc(const std::string& h) { return h.empty() ? "-" : h; }
std::string dec(const std::string& h) { return h == "-" ? "" : h; }

} // namespace

MergeStateStore::MergeStateStore(const std::string& repoDir) : file_(repoDir + "/MERGE_STATE") {}

bool MergeStateStore::exists() const { return FileIO::exists(file_); }

bool MergeStateStore::clear() const {
    std::error_code ec;
    std::filesystem::remove(file_, ec);
    return !ec;
}

bool MergeStateStore::save(const MergeState& s) const {
    std::string out = "ours " + s.oursCommit + "\ntheirs " + s.theirsCommit + "\nbase " + enc(s.baseCommit) +
                      "\nours-label " + s.oursLabel + "\ntheirs-label " + s.theirsLabel + "\n";
    for (const auto& c : s.conflicts)
        out += "conflict " + enc(c.baseBlob) + " " + enc(c.oursBlob) + " " + enc(c.theirsBlob) + " " + c.path + "\n";
    return FileIO::writeFile(file_, out);
}

bool MergeStateStore::load(MergeState& out) const {
    std::string raw;
    if (!FileIO::readFile(file_, raw)) return false;
    MergeState s;
    for (std::string line : DiffEngine::splitLines(raw)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t sp = line.find(' ');
        if (sp == std::string::npos) continue;
        std::string key = line.substr(0, sp), rest = line.substr(sp + 1);
        if (key == "ours") s.oursCommit = rest;
        else if (key == "theirs") s.theirsCommit = rest;
        else if (key == "base") s.baseCommit = dec(rest);
        else if (key == "ours-label") s.oursLabel = rest;
        else if (key == "theirs-label") s.theirsLabel = rest;
        else if (key == "conflict") {
            std::string tok[3];
            size_t pos = 0;
            for (int i = 0; i < 3; ++i) {
                size_t e = rest.find(' ', pos);
                if (e == std::string::npos) return false;
                tok[i] = rest.substr(pos, e - pos);
                pos = e + 1;
                if (!okHash(tok[i])) return false;
            }
            ConflictEntry c;
            c.baseBlob = dec(tok[0]); c.oursBlob = dec(tok[1]); c.theirsBlob = dec(tok[2]);
            c.path = rest.substr(pos);
            if (!Index::isValidPath(c.path)) return false;
            s.conflicts.push_back(c);
        }
    }
    if (!okHash(s.oursCommit) || !okHash(s.theirsCommit) || s.oursCommit == "-" || s.theirsCommit == "-") return false;
    out = s;
    return true;
}