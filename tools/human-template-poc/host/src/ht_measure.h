#pragma once

// Body measurements and region-impact statistics for the shape-target proof.
//
// Measurement planes are PAE's own definition for this proof (fractions of
// stature measured from the soles), NOT MakeHuman's measurement planes. They
// are recomputed on every deformed mesh so that they follow the shape.

#include "ht_common.h"
#include "ht_rig.h"

#include <string>
#include <vector>

namespace pae::ht {

struct Measures {
    double stature = 0.0;
    double bideltoidWidth = 0.0;   // shoulder joint height - 2.5 % stature
    double chestWidth = 0.0, chestDepth = 0.0;    // 72 % stature, thorax region only
    double waistWidth = 0.0, waistDepth = 0.0;    // 60 % stature, abdomen region only
    double hipWidth = 0.0;                        // hip joint height, pelvis + thigh
    double upperArmLength = 0.0, forearmLength = 0.0, thighLength = 0.0, shankLength = 0.0;  // joint-to-joint
    double shoulderJointHeight = 0.0, hipJointHeight = 0.0;  // above the soles
};

Measures measureBody(const std::vector<D3>& pos, const std::vector<Region>& regions, const Rig& rig);

// Share of a target's total L1 displacement that lands in each region.
// Vertices beyond the body (helper / joint geometry) are not in any region;
// their share of the displacement is returned through helperShare.
std::vector<double> regionDisplacementShare(const std::vector<Region>& regions, const std::vector<int>& vertex,
                                            const std::vector<D3>& delta, double* helperShare = nullptr);
// Mean displacement length (same unit as delta) of the body vertices of each
// region. Unbiased by region vertex density, unlike the L1 share.
std::vector<double> regionMeanDisplacement(const std::vector<Region>& regions, const std::vector<int>& vertex,
                                           const std::vector<D3>& delta);
// Bounding box (metres) of the body vertices a target moves by more than 1e-6.
bool displacedBoundingBox(const std::vector<D3>& pos, const std::vector<int>& vertex, const std::vector<D3>& delta,
                          D3& lo, D3& hi);

} // namespace pae::ht
