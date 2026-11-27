#pragma once

// VR-1B WP3-HOST-01: PAE's own host measurement protocol.
//
// Every measurement below is a PAE-DEFINED SURFACE MEASUREMENT on the pinned
// MakeHuman base mesh. None of them is an ANSUR or MakeHuman official
// measurement; the acceptance-caliber alignment is a later WP3-Production
// prerequisite. Definitions (plane position, band, regions, direction, unit,
// vertex set, formula) are documented in measurements-protocol.md generated
// by the experiment runner.
//
// Circumferences are TRUE cross-section perimeters: triangles are cut by the
// measurement plane, the intersection segments are summed (and chained to
// verify closed loops). Width-only proxies are never reported as circumference.

#include "ht_common.h"
#include "ht_obj.h"
#include "ht_rig.h"

#include <string>
#include <vector>

namespace pae::wp3 {

// One closed intersection loop of the body surface with a plane.
struct SectionLoop {
    double perimeter = 0.0;     // m
    double area = 0.0;          // m^2, shoelace in the plane frame (signed->abs)
    double breadth = 0.0;       // in-plane horizontal extent (m)
    double depth = 0.0;         // in-plane secondary extent (m)
    ht::D3 centroid;
    size_t points = 0;
};

// One planar cross-section of the body surface. A horizontal cut at waist or
// hip height also cuts the hanging arms (A-pose), so the section has MULTIPLE
// loops. Torso measurements use the largest-area loop; the upper-arm section
// uses the loop nearest the bone midpoint. Summing all loops would overstate
// circumferences — that defect was found and fixed in this round.
struct Section {
    double perimeter = 0.0;     // total over all loops (m)
    std::vector<SectionLoop> loops;
    int openChains = 0;         // chains that did not close (surface defect)
    size_t points = 0;
    const SectionLoop* largestArea() const;
    const SectionLoop* nearestTo(const ht::D3& p) const;
};

// Cut the (closed) body surface with plane {normal, point}. For horizontal
// sections breadth = x extent, depth = z extent. For oblique sections the
// in-plane frame is built from the normal.
Section cutSection(const std::vector<ht::D3>& pos, const std::vector<ht::PolyFace>& faces,
                   const ht::D3& normal, const ht::D3& point);

struct Wp3Measures {
    double stature = 0.0;               // body bbox y extent (m)
    double biacromialProxy = 0.0;       // L/R upperarm01 head-joint distance (m), joint-center proxy
    double shoulderYoke = 0.0;          // shoulder band breadth, neck+thorax+shoulder-girdle only (m)
    double outerShoulder = 0.0;         // shoulder band breadth incl. upper arms (m)
    double chestBreadth = 0.0, chestDepth = 0.0, chestCircumference = 0.0;   // 72% stature plane
    double waistBreadth = 0.0, waistDepth = 0.0, waistCircumference = 0.0;   // 60% stature plane
    double hipBreadth = 0.0, hipDepth = 0.0, hipCircumference = 0.0;         // hip joint height plane
    double upperArmCircumferenceL = 0.0, upperArmBreadthL = 0.0;             // oblique mid-upper-arm section
    double shoulderJointHeight = 0.0, hipJointHeight = 0.0;                  // above soles (m)
    double upperArmLength = 0.0, forearmLength = 0.0, thighLength = 0.0, shankLength = 0.0;
    double soleY = 0.0;
};

// Plane heights follow the PAE proof caliber (fractions of stature from the
// soles); the shoulder band is shoulder-joint height - 2.5% stature with a
// 1.5% stature half-width, matching ht_measure for comparability.
Wp3Measures measureAll(const std::vector<ht::D3>& bodyPos, const std::vector<ht::Region>& regions,
                       const ht::Rig& rig, const std::vector<ht::PolyFace>& bodyFaces);

// Mean / max displacement (m) per region for a target's vertex/delta arrays.
struct RegionLinkage {
    double mean[14] = {0};   // indexed by ht::Region (Count=12, Unknown=13)
    double max[14] = {0};
    double helperMean = 0.0;
};
RegionLinkage regionLinkage(const std::vector<ht::Region>& regions, const std::vector<int>& vertex,
                            const std::vector<ht::D3>& delta, size_t bodyCount);

} // namespace pae::wp3