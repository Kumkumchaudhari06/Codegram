#include "storage/objectstore.h"
#include "storage/serializer.h"
#include "utils/fileio.h"
#include "utils/sha256.h"
#include "utils/compression.h"

namespace {

// Only 64 lowercase hex characters are accepted, which also blocks
// path tricks such as "../" in a hash.
bool validHash(const std::string& h) {
    if (h.size() != 64) return false;
    for (char c : h)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}

} // namespace

ObjectStore::ObjectStore(const std::string& repoDir)
    : objectsDir_(repoDir + "/objects") {}

std::string ObjectStore::pathFor(const std::string& hash) const {
    return objectsDir_ + "/" + hash.substr(0, 2) + "/" + hash.substr(2);
}

std::string ObjectStore::hashObject(const std::string& type, const std::string& content) {
    if (type.empty() || type.find(' ') != std::string::npos) return "";
    return Sha256::hash(type + " " + std::to_string(content.size()) + "\n" + content);
}

std::string ObjectStore::writeObject(const std::string& type, const std::string& content) {
    std::string hash = hashObject(type, content);
    if (hash.empty()) return "";

    std::string full = type + " " + std::to_string(content.size()) + "\n" + content;

    if (exists(hash)) return hash;   // same content is stored only once
    if (!FileIO::writeFile(pathFor(hash), Compression::compress(full))) return "";
    return hash;
}

bool ObjectStore::readObject(const std::string& hash, std::string& type,
                             std::string& content) const {
    if (!validHash(hash)) return false;

    std::string packed, full;
    if (!FileIO::readFile(pathFor(hash), packed)) return false;
    if (!Compression::decompress(packed, full)) return false;
    if (Sha256::hash(full) != hash) return false;   // file was corrupted or edited

    size_t nl = full.find('\n');
    if (nl == std::string::npos) return false;

    std::string header = full.substr(0, nl);
    size_t sp = header.find(' ');
    if (sp == std::string::npos) return false;

    size_t size = 0;
    try { size = std::stoull(header.substr(sp + 1)); }
    catch (...) { return false; }

    std::string body = full.substr(nl + 1);
    if (body.size() != size) return false;

    type = header.substr(0, sp);
    content = body;
    return true;
}

bool ObjectStore::exists(const std::string& hash) const {
    return validHash(hash) && FileIO::exists(pathFor(hash));
}

std::string ObjectStore::writeBlob(const std::string& content) {
    return writeObject("blob", content);
}

bool ObjectStore::readBlob(const std::string& hash, std::string& out) const {
    std::string type;
    return readObject(hash, type, out) && type == "blob";
}

std::string ObjectStore::writeTree(const Tree& tree) {
    return writeObject("tree", Serializer::serializeTree(tree));
}

bool ObjectStore::readTree(const std::string& hash, Tree& out) const {
    std::string type, content;
    if (!readObject(hash, type, content) || type != "tree") return false;
    return Serializer::parseTree(content, out);
}

std::string ObjectStore::writeCommit(const Commit& commit) {
    return writeObject("commit", Serializer::serializeCommit(commit));
}

bool ObjectStore::readCommit(const std::string& hash, Commit& out) const {
    std::string type, content;
    if (!readObject(hash, type, content) || type != "commit") return false;
    if (!Serializer::parseCommit(content, out)) return false;
    out.hash = hash;
    return true;
}