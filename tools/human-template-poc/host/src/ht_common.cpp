#include "ht_common.h"

#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <sstream>

namespace pae::ht {

double distanceToSegment(const D3& p, const D3& a, const D3& b) {
    const D3 ab = b - a;
    const double len2 = ab.lengthSq();
    if (len2 < 1e-18) return (p - a).length();
    double t = (p - a).dot(ab) / len2;
    t = std::max(0.0, std::min(1.0, t));
    return (p - (a + ab * t)).length();
}

bool readFile(const std::string& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    out = ss.str();
    return true;
}

bool readFileBytes(const std::string& path, std::vector<uint8_t>& out) {
    std::string text;
    if (!readFile(path, text)) return false;
    out.assign(text.begin(), text.end());
    return true;
}

bool writeFile(const std::string& path, const std::string& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(out);
}

bool fileExists(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return static_cast<bool>(in);
}

bool makeDirs(const std::string& path) {
    // Plain host path; used only by the prototype on its own out/ tree.
    if (path.empty()) return false;
    std::string current;
    for (size_t i = 0; i < path.size(); ++i) {
        current += path[i];
        if (path[i] == '/' || path[i] == '\\') {
            const std::string dir = current.substr(0, current.size() - 1);
            if (!dir.empty() && dir != "." && dir != "..") {
                std::string cmd = "mkdir -p \"" + dir + "\"";
                if (std::system(cmd.c_str()) != 0 && !fileExists(dir + "/")) return false;
            }
        }
    }
    std::string cmd = "mkdir -p \"" + path + "\"";
    return std::system(cmd.c_str()) == 0;
}

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> out;
    const char* p = line.c_str();
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
        const char* start = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') ++p;
        if (p > start) out.emplace_back(start, p);
    }
    return out;
}

bool parseLong(const std::string& s, long& out) {
    if (s.empty()) return false;
    char* end = nullptr;
    const long v = std::strtol(s.c_str(), &end, 10);
    if (end == s.c_str() || *end != '\0') return false;
    out = v;
    return true;
}

bool parseDouble(const std::string& s, double& out) {
    if (s.empty()) return false;
    char* end = nullptr;
    const double v = std::strtod(s.c_str(), &end);
    if (end == s.c_str() || *end != '\0') return false;
    out = v;
    return true;
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

std::string fileNameOf(const std::string& path) {
    const size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::string fmt(const char* format, ...) {
    char buffer[2048];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    return buffer;
}

double nowMs() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

std::string utcNow() {
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &now);
#else
    gmtime_r(&now, &tm);
#endif
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buffer;
}

// ---------------------------------------------------------------------------
// SHA-256 (FIPS 180-4). Compact reference implementation.
// ---------------------------------------------------------------------------

namespace {

struct Sha256 {
    uint32_t h[8];
    uint64_t bitLength = 0;
    uint8_t block[64];
    size_t blockUsed = 0;

    Sha256() {
        static const uint32_t init[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                         0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
        for (int i = 0; i < 8; ++i) h[i] = init[i];
    }

    static uint32_t rotr(uint32_t v, int n) { return (v >> n) | (v << (32 - n)); }

    void transform(const uint8_t* p) {
        static const uint32_t k[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(p[i * 4]) << 24) | (static_cast<uint32_t>(p[i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(p[i * 4 + 2]) << 8) | static_cast<uint32_t>(p[i * 4 + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            const uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; ++i) {
            const uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            const uint32_t ch = (e & f) ^ (~e & g);
            const uint32_t t1 = hh + S1 + ch + k[i] + w[i];
            const uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const uint32_t t2 = S0 + maj;
            hh = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }

    void update(const uint8_t* data, size_t size) {
        bitLength += static_cast<uint64_t>(size) * 8;
        for (size_t i = 0; i < size; ++i) {
            block[blockUsed++] = data[i];
            if (blockUsed == 64) {
                transform(block);
                blockUsed = 0;
            }
        }
    }

    std::string finalHex() {
        const uint64_t messageBits = bitLength;
        uint8_t pad = 0x80;
        update(&pad, 1);
        pad = 0x00;
        while (blockUsed != 56) update(&pad, 1);
        uint8_t lengthBytes[8];
        for (int i = 0; i < 8; ++i) lengthBytes[i] = static_cast<uint8_t>(messageBits >> (56 - i * 8));
        // Length is appended directly (not counted as message bits).
        for (int i = 0; i < 8; ++i) {
            block[blockUsed++] = lengthBytes[i];
            if (blockUsed == 64) {
                transform(block);
                blockUsed = 0;
            }
        }
        static const char* hex = "0123456789abcdef";
        std::string out;
        for (int i = 0; i < 8; ++i) {
            for (int shift = 24; shift >= 0; shift -= 8) {
                const uint8_t byte = static_cast<uint8_t>(h[i] >> shift);
                out += hex[byte >> 4];
                out += hex[byte & 15];
            }
        }
        return out;
    }
};

} // namespace

std::string sha256Hex(const std::vector<uint8_t>& bytes) {
    Sha256 sha;
    sha.update(bytes.data(), bytes.size());
    return sha.finalHex();
}

std::string sha256HexFile(const std::string& path) {
    std::vector<uint8_t> bytes;
    if (!readFileBytes(path, bytes)) return {};
    return sha256Hex(bytes);
}

} // namespace pae::ht
