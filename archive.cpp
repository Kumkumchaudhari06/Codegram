#include "utils/archive.h"
#include "utils/fileio.h"
#include "utils/compression.h"
#include <cstdint>
#include <filesystem>
#include <utility>
#include <vector>

// Format: "CGAR" + count(4) + [ pathLen(4) path size(4) data ]...
// The whole thing is optionally compressed (then it starts with "CGZ1").

namespace fs = std::filesystem;

namespace {

void putU32(std::string& s, uint32_t v) {
    for (int i = 3; i >= 0; --i) s.push_back(static_cast<char>((v >> (i * 8)) & 0xff));
}

bool getU32(const std::string& s, size_t& p, uint32_t& v) {
    if (p + 4 > s.size()) return false;
    v = 0;
    for (int i = 0; i < 4; ++i) v = (v << 8) | static_cast<unsigned char>(s[p++]);
    return true;
}

// Rejects empty, absolute, and ".." paths so an archive cannot write outside destDir
bool safeRelative(const std::string& rel) {
    if (rel.empty()) return false;
    fs::path p(rel);
    if (p.is_absolute() || p.has_root_name() || p.has_root_directory()) return false;
    for (const auto& part : p)
        if (part == ".." || part.empty()) return false;
    return true;
}

} // namespace

namespace Archive {

bool exportDir(const std::string& srcDir, const std::string& archiveFile, bool compress) {
    std::error_code ec;
    if (!fs::is_directory(srcDir, ec)) return false;
    fs::path archivePath = fs::weakly_canonical(archiveFile, ec);

    std::vector<std::pair<std::string, std::string>> files;
    for (const auto& f : FileIO::listFiles(srcDir)) {
        std::string rel = fs::path(f).lexically_relative(srcDir).generic_string();
        if (rel.rfind("backup/", 0) == 0) continue;
        std::error_code e2;
        if (fs::weakly_canonical(f, e2) == archivePath) continue;   // never pack the archive itself

        std::string data;
        if (!FileIO::readFile(f, data)) return false;
        files.emplace_back(rel, std::move(data));
    }

    std::string blob = "CGAR";
    putU32(blob, static_cast<uint32_t>(files.size()));
    for (const auto& f : files) {
        putU32(blob, static_cast<uint32_t>(f.first.size()));
        blob += f.first;
        putU32(blob, static_cast<uint32_t>(f.second.size()));
        blob += f.second;
    }
    return FileIO::writeFile(archiveFile, compress ? Compression::compress(blob) : blob);
}

bool importArchive(const std::string& archiveFile, const std::string& destDir) {
    std::string blob;
    if (!FileIO::readFile(archiveFile, blob)) return false;

    if (blob.compare(0, 4, "CGZ1") == 0) {
        std::string raw;
        if (!Compression::decompress(blob, raw)) return false;
        blob = std::move(raw);
    }
    if (blob.size() < 8 || blob.compare(0, 4, "CGAR") != 0) return false;

    size_t p = 4;
    uint32_t count = 0;
    if (!getU32(blob, p, count)) return false;

    // Pass 1: parse and validate everything before writing anything
    std::vector<std::pair<std::string, std::string>> files;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t pathLen = 0, size = 0;
        if (!getU32(blob, p, pathLen) || p + pathLen > blob.size()) return false;
        std::string rel = blob.substr(p, pathLen);
        p += pathLen;
        if (!safeRelative(rel)) return false;
        if (!getU32(blob, p, size) || p + size > blob.size()) return false;
        files.emplace_back(rel, blob.substr(p, size));
        p += size;
    }

    // Pass 2: write
    for (const auto& f : files)
        if (!FileIO::writeFile(destDir + "/" + f.first, f.second)) return false;
    return true;
}

} // namespace Archive