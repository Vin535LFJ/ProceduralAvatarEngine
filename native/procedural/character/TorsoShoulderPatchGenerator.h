#pragma once

#include "AnatomicalRegions.h"
#include "BodyPatchGenerator.h"
#include "GeometryBuildRequest.h"
#include "RegionWeightGenerator.h"
#include "ResolvedCharacterState.h"
#include "VariableProfileSweep.h"
#include "procedural/MeshTopology.h"

#include <string>
#include <vector>

namespace pae::character {

struct TorsoShoulderTimings {
    double stateResolutionMs = 0.0;
    double geometryMs = 0.0;
    double assemblyMs = 0.0;
    double weightMs = 0.0;
    double totalMs = 0.0;
};

struct TorsoShoulderResult {
    MeshData mesh;
    PatchGenerationReport report;
    TopologyReport topology;
    RegionWeightReport weights;
    ResolvedCharacterState state;
    GeometryBuildRequest request;
    std::vector<RegionId> vertexRegions;
    std::vector<uint64_t> stableIds;
    SweepDiagnostics frameDiagnostics;
    TorsoShoulderTimings timings;
    bool oq03Applicable = false;
    std::string error;
    bool valid = false;
};

class TorsoShoulderPatchGenerator {
public:
    static TorsoShoulderResult generate(const shape::v2::CharacterDefinition& definition,
                                        const GeometryBuildRequest& request,
                                        const BodyJoints* legacyJoints = nullptr);
};

} // namespace pae::character