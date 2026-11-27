#include "procedural/human_template/HumanTemplateLoader.h"

#include <cstdio>

namespace pae::human_template {

bool loadTemplateBytes(const std::string& bytes, HumanTemplate& out, std::string& error) {
    out = HumanTemplate{};
    return parseTemplate(bytes, out, error);
}

bool loadTemplateFile(const std::string& path, HumanTemplate& out, std::string& error) {
    out = HumanTemplate{};
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) {
        error = "cannot open template file: " + path;
        return false;
    }
    std::string bytes;
    char buffer[65536];
    size_t n = 0;
    while ((n = std::fread(buffer, 1, sizeof(buffer), f)) > 0) bytes.append(buffer, n);
    const bool readError = std::ferror(f) != 0;
    std::fclose(f);
    if (readError) {
        error = "read error: " + path;
        return false;
    }
    if (bytes.empty()) {
        error = "empty template file: " + path;
        return false;
    }
    return parseTemplate(bytes, out, error);
}

} // namespace pae::human_template