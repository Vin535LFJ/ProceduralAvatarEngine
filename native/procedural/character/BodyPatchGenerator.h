#pragma once

#include "CharacterDefinition.h"
#include "procedural/MeshData.h"
#include "skeleton/Skeleton.h"

#include <array>
#include <string>
#include <vector>

namespace pae {

struct BoundaryReport {
    std::string id;
    uint32_t vertexCount = 0;
    float maxPositionDeviation = 0.0f;
    float maxNormalDeviationDeg = 0.0f;
    bool smoothSeam = true;
    bool valid = false;
};

struct PatchGenerationReport {
    std::vector<BoundaryReport> boundaries;
    uint32_t vertexCount = 0;
    uint32_t triangleCount = 0;
    double generationMs = 0.0;
    bool valid = false;
};

// P1-A: generates the unclothed torso + shoulder bridge + one arm from
// anatomical landmarks. Topology templates, surface patches, variable-profile
// sweeps and patch assembly are implemented in this single module to avoid
// adding classes for their own sake.
class BodyPatchGenerator {
public:
    static bool generate(const CharacterDefinition& definition,
                         const BodyJoints& joints,
                         const Skeleton& skeleton,
                         MeshData& outMesh,
                         PatchGenerationReport& report);
};

} // namespace pae
