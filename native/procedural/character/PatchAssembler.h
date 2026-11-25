#pragma once

#include "SurfacePatch.h"

#include <vector>

namespace pae::character {

PatchAssemblyResult assemblePatches(const std::vector<SurfacePatch>& patches);

} // namespace pae::character