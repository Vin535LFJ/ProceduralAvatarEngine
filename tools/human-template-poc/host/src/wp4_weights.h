#pragma once

// WP4-HOST-RIG-01 (WP4-B): PAE skin weight generation + diagnostics.
// Reuses the PAE Distance / Heat generators (ht_rig) and adds a Region-
// constrained variant. Weights are always PAE-generated on the CURRENT-shape
// rig; the PENDING default_weights.mhw is never used. Host-only.

#include "ht_common.h"
#include "ht_obj.h"
#include "ht_rig.h"
#include "ht_topology.h"
#include "wp4_rig.h"

#include <string>
#include <vector>

namespace pae::wp4 {

enum class WeightMethod { Distance, Heat, Region };
const char* weightMethodName(WeightMethod m);

struct WeightOptions {
    int smoothingIterations = 2;   // distance
    double diffusionTime = 0.0;    // heat (0 => default)
    int heatIterations = 0;
    double heatPower = 0.0;
};

// Generate body weights (vertexCount = body vertex count) on the current rig.
ht::SkinWeights generateWeights(const ht::PolyMesh& body, const ht::Topology& topo, const RigV2& rig,
                                WeightMethod method, const WeightOptions& opt);

struct WeightDiagnostics {
    long vertices = 0;
    long unbound = 0;
    int maxInfluences = 0;
    double minSum = 0.0, maxSum = 0.0;
    double discardedMass = 0.0;       // Top-4 compression loss
    double crossSideMass = 0.0;       // weight on opposite-side bones (sided verts)
    long crossSideVertices = 0;
    long farBoundVertices = 0;        // dominant bone head farther than 3x local radius
    double meanDominantDistance = 0.0;
    double smoothnessL1 = 0.0;        // mean L1 weight difference across mesh edges
    double generationMs = 0.0;
};

WeightDiagnostics diagnoseWeights(const ht::PolyMesh& body, const ht::Topology& topo, const RigV2& rig,
                                  const ht::SkinWeights& sw);

// Per-bone weight mass + vertex count (for tables / heatmaps).
struct BoneWeightSummary {
    std::string bone;
    double mass = 0.0;
    long vertices = 0;
};
std::vector<BoneWeightSummary> boneWeightSummaries(const RigV2& rig, const ht::SkinWeights& sw);

} // namespace pae::wp4