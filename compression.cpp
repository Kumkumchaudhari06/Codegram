#include "utils/compression.h"
#include <algorithm>
#include <cstdint>

// Format:  "CGZ1" + 4-byte original size + blocks
//   0x00 <count> <bytes...>        literal block (1..255 bytes)
//   0x01 <offset hi> <offset lo> <length>   copy from earlier output

namespace {

const size_t WINDOW    = 4095;
const size_t MIN_MATCH = 4;
const size_t MAX_MATCH = 255;

void flushLiterals(std::string& out, std::string& lit) {
    size_t pos = 0;
    while (pos < lit.size()) {
        size_t n = std::min<size_t>(255, lit.size() - pos);
        out.push_back(static_cast<char>(0x00));
        out.push_back(static_cast<char>(n));
        out.append(lit, pos, n);
        pos += n;
    }
    lit.clear();
}

} // namespace

namespace Compression {

std::string compress(const std::string& data) {
    std::string out = "CGZ1";
    uint32_t size = static_cast<uint32_t>(data.size());
    for (int i = 3; i >= 0; --i)
        out.push_back(static_cast<char>((size >> (i * 8)) & 0xff));

    std::string lit;
    size_t i = 0;
    while (i < data.size()) {
        size_t bestLen = 0, bestOff = 0;
        size_t start = (i > WINDOW) ? i - WINDOW : 0;

        for (size_t j = start; j < i; ++j) {
            size_t l = 0;
            while (l < MAX_MATCH && i + l < data.size() && data[j + l] == data[i + l]) ++l;
            if (l > bestLen) { bestLen = l; bestOff = i - j; }
        }

        if (bestLen >= MIN_MATCH) {
            flushLiterals(out, lit);
            out.push_back(static_cast<char>(0x01));
            out.push_back(static_cast<char>((bestOff >> 8) & 0xff));
            out.push_back(static_cast<char>(bestOff & 0xff));
            out.push_back(static_cast<char>(bestLen));
            i += bestLen;
        } else {
            lit.push_back(data[i]);
            ++i;
        }
    }
    flushLiterals(out, lit);
    return out;
}

bool decompress(const std::string& packed, std::string& out) {
    out.clear();
    if (packed.size() < 8 || packed.compare(0, 4, "CGZ1") != 0) return false;

    uint32_t expected = 0;
    for (int i = 4; i < 8; ++i)
        expected = (expected << 8) | static_cast<unsigned char>(packed[i]);

    size_t p = 8;
    while (p < packed.size()) {
        unsigned char type = static_cast<unsigned char>(packed[p++]);
        if (type == 0x00) {
            if (p >= packed.size()) return false;
            size_t n = static_cast<unsigned char>(packed[p++]);
            if (n == 0 || p + n > packed.size()) return false;
            out.append(packed, p, n);
            p += n;
        } else if (type == 0x01) {
            if (p + 3 > packed.size()) return false;
            size_t off = (static_cast<unsigned char>(packed[p]) << 8) |
                          static_cast<unsigned char>(packed[p + 1]);
            size_t len = static_cast<unsigned char>(packed[p + 2]);
            p += 3;
            if (off == 0 || len == 0 || off > out.size()) return false;
            size_t from = out.size() - off;
            for (size_t k = 0; k < len; ++k) out.push_back(out[from + k]);
        } else {
            return false;
        }
        if (out.size() > expected) return false;
    }
    return out.size() == expected;
}

} // namespace Compression