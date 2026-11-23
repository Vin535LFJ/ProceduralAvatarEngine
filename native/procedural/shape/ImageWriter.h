#pragma once

// Minimal, dependency-free PNG writer for the G2 grey-clay evidence.
// Writes 8-bit RGB PNGs using stored (uncompressed) DEFLATE blocks, so no
// zlib dependency is introduced into the host-only PoC targets.

#include <cstdint>
#include <string>
#include <vector>

namespace pae::shape {

struct RgbImage {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> pixels;  // width * height * 3, row major from the top

    void clear(uint8_t r, uint8_t g, uint8_t b);
    void set(int x, int y, uint8_t r, uint8_t g, uint8_t b);
};

bool writePng(const std::string& path, const RgbImage& image);

} // namespace pae::shape
