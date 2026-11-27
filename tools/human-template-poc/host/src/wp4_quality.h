#pragma once

// WP4-HOST-RIG-01 (WP4-E): per-joint-region deformation quality. Global checks
// reuse PAE's own checkers via wp3_quality; this module adds joint-local masks
// and metrics so the report never hides a bad joint behind a body average.

#include "ht_common.h"
#include "ht_obj.h"
#include "ht_topology.h"
#include "wp4_rig.h"

#include <string>
#include <vector>

namespace pae::wp4 {

struct JointRegion {
    std::string name;
    ht::D3 center;
    double radius = 0.05;   // metres; 0 => use region label mask instead
    int labelMask = -1;     // ht::Region ordinal for finger/toe masks, -1 = distance
    char side = 0;          // 'L'/'R' for label masks
};

// Shoulder+axilla, elbow, wrist, hip+groin, knee, fingers, toes (both sides).
std::vector<JointRegion> jointRegions(const RigV2& rig, const JointSet& js);

std::vector<bool> regionMask(const ht::PolyMesh& body, const std::vector<ht::D3>& rest,
                             const std::vector<ht::Region>& labels, const JointRegion& reg);

struct RegionMetrics {
    std::string name;
    long vertices = 0, faces = 0;
    long flippedFaces = 0;
    double minNormalDot = 1.0;
    double edgeRatioP99 = 1.0, edgeRatioMax = 1.0;
    double areaRatio = 1.0;
};

RegionMetrics evaluateRegion(const ht::PolyMesh& body, const ht::Topology& topo,
                             const std::vector<ht::D3>& rest, const std::vector<ht::D3>& posed,
                             const std::vector<bool>& mask, const std::string& name);

} // namespace pae::wp4