#include "procedural/shape/ImageWriter.h"

#include <algorithm>
#include <cstdio>

namespace pae::shape {
namespace {

void appendU32BE(std::vector<uint8_t>& out, uint32_t value) {
    out.push_back(static_cast<uint8_t>(value >> 24));
    out.push_back(static_cast<uint8_t>(value >> 16));
    out.push_back(static_cast<uint8_t>(value >> 8));
    out.push_back(static_cast<uint8_t>(value));
}

uint32_t crc32(const uint8_t* data, size_t length) {
    static uint32_t table[256];
    static bool ready = false;
    if (!ready) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        ready = true;
    }
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < length; ++i) crc = table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

void appendChunk(std::vector<uint8_t>& out, const char type[4],
                 const std::vector<uint8_t>& payload) {
    appendU32BE(out, static_cast<uint32_t>(payload.size()));
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), payload.begin(), payload.end());
    // The CRC covers the chunk type AND the payload.
    std::vector<uint8_t> combined;
    combined.reserve(4 + payload.size());
    combined.insert(combined.end(), type, type + 4);
    combined.insert(combined.end(), payload.begin(), payload.end());
    appendU32BE(out, crc32(combined.data(), combined.size()));
}

uint32_t adler32(const uint8_t* data, size_t length) {
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < length; ++i) {
        a = (a + data[i]) % 65521u;
        b = (b + a) % 65521u;
    }
    return (b << 16) | a;
}

} // namespace

void RgbImage::clear(uint8_t r, uint8_t g, uint8_t b) {
    pixels.assign(static_cast<size_t>(std::max(0, width)) *
                  static_cast<size_t>(std::max(0, height)) * 3u, 0);
    for (size_t i = 0; i < pixels.size(); i += 3) {
        pixels[i] = r;
        pixels[i + 1] = g;
        pixels[i + 2] = b;
    }
}

void RgbImage::set(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < 0 || y < 0 || x >= width || y >= height) return;
    const size_t i = (static_cast<size_t>(y) * static_cast<size_t>(width) +
                      static_cast<size_t>(x)) * 3u;
    if (i + 2 >= pixels.size()) return;
    pixels[i] = r;
    pixels[i + 1] = g;
    pixels[i + 2] = b;
}

bool writePng(const std::string& path, const RgbImage& image) {
    if (image.width <= 0 || image.height <= 0) return false;
    const size_t rowBytes = static_cast<size_t>(image.width) * 3u;

    // Scanlines: filter type 0 (None) followed by RGB bytes.
    std::vector<uint8_t> raw;
    raw.reserve((rowBytes + 1) * static_cast<size_t>(image.height));
    for (int y = 0; y < image.height; ++y) {
        raw.push_back(0);
        const size_t offset = static_cast<size_t>(y) * rowBytes;
        if (offset + rowBytes <= image.pixels.size()) {
            raw.insert(raw.end(), image.pixels.begin() + static_cast<long>(offset),
                       image.pixels.begin() + static_cast<long>(offset + rowBytes));
        }
    }

    // zlib stream with stored DEFLATE blocks (no compression).
    std::vector<uint8_t> zlib;
    zlib.push_back(0x78);
    zlib.push_back(0x01);
    size_t offset = 0;
    do {
        const size_t remaining = raw.size() - offset;
        const size_t take = std::min<size_t>(remaining, 65535u);
        const uint16_t len = static_cast<uint16_t>(take);
        const bool final = (offset + take == raw.size());
        zlib.push_back(final ? 0x01 : 0x00);
        zlib.push_back(static_cast<uint8_t>(len & 0xFFu));
        zlib.push_back(static_cast<uint8_t>(len >> 8));
        const uint16_t nlen = static_cast<uint16_t>(~len);
        zlib.push_back(static_cast<uint8_t>(nlen & 0xFFu));
        zlib.push_back(static_cast<uint8_t>(nlen >> 8));
        zlib.insert(zlib.end(), raw.begin() + static_cast<long>(offset),
                    raw.begin() + static_cast<long>(offset + take));
        offset += take;
    } while (offset < raw.size());
    const uint32_t checksum = adler32(raw.data(), raw.size());
    zlib.push_back(static_cast<uint8_t>(checksum >> 24));
    zlib.push_back(static_cast<uint8_t>(checksum >> 16));
    zlib.push_back(static_cast<uint8_t>(checksum >> 8));
    zlib.push_back(static_cast<uint8_t>(checksum));

    std::vector<uint8_t> png;
    png = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

    std::vector<uint8_t> ihdr;
    appendU32BE(ihdr, static_cast<uint32_t>(image.width));
    appendU32BE(ihdr, static_cast<uint32_t>(image.height));
    ihdr.push_back(8);  // bit depth
    ihdr.push_back(2);  // colour type: truecolour RGB
    ihdr.push_back(0);  // compression
    ihdr.push_back(0);  // filter
    ihdr.push_back(0);  // interlace
    appendChunk(png, "IHDR", ihdr);
    appendChunk(png, "IDAT", zlib);
    appendChunk(png, "IEND", {});

    std::FILE* file = std::fopen(path.c_str(), "wb");
    if (!file) return false;
    const bool ok = std::fwrite(png.data(), 1, png.size(), file) == png.size();
    std::fclose(file);
    return ok;
}

} // namespace pae::shape
