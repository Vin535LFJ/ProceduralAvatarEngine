#pragma once

// WP2 read-only template loader. Pure data + stdio: no GL, no asset manager,
// no MakeHuman code. The Android runtime reads a pushed/bundled .paht file;
// host tests read the same bytes from disk.

#include "procedural/human_template/PAEHumanTemplateFormat.h"

#include <string>

namespace pae::human_template {

// Read a file and parse it with full validation (magic, version, payload
// SHA-256, index ranges, weight block). On failure `error` carries the reason
// and `out` is left empty. Never throws.
bool loadTemplateFile(const std::string& path, HumanTemplate& out, std::string& error);

// Parse already-loaded bytes (same validation as loadTemplateFile).
bool loadTemplateBytes(const std::string& bytes, HumanTemplate& out, std::string& error);

} // namespace pae::human_template