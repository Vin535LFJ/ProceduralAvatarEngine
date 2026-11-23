#pragma once

// M12-A: the human upper body expressed as a single composed signed-distance
// field. Region primitives are anchored to the BodyJoints landmarks so the
// extracted mesh aligns with the 22-bone bind pose exactly. Paired regions
// (clavicles/deltoids/arms) are modeled once on +X and mirrored by the
// evaluation helper; the same region fields provide semantic skin weights.

#include "core/math/Math.h"
#include "skeleton/Skeleton.h"

#include <array>
#include <functional>

namespace pae::anatomy {

// All fields participate in the composed body surface (M12 §6.1).
struct HumanAnatomyParameters {
    // Overall proportions
    float torsoHeight = 1.0f;      // torso vertical scale
    float shoulderWidth = 1.0f;    // shoulder-line scale
    float chestWidth = 1.0f;
    float chestDepth = 1.0f;
    float waistWidth = 1.0f;
    float waistDepth = 1.0f;
    float pelvisWidth = 1.0f;

    // Torso shape
    float ribCageVolume = 1.0f;
    float abdominalVolume = 1.0f;
    float backDepth = 0.5f;        // 0 flat .. 1 round back carve
    float waistCurvature = 0.5f;   // waist pinch strength

    // Shoulder
    float clavicleWidth = 1.0f;
    float deltoidVolume = 1.0f;
    float shoulderSlope = 1.0f;    // deltoid vertical drop factor
    float axillaDepth = 0.6f;      // armpit blend sharpness

    // Arms
    float upperArmLength = 1.0f;
    float upperArmVolume = 1.0f;
    float elbowWidth = 1.0f;
    float forearmLength = 1.0f;
    float forearmVolume = 1.0f;
    float wristWidth = 1.0f;

    // Shape style
    float stylization = 1.0f;      // cartoon exaggeration of region volumes
    float smoothness = 1.0f;       // global smooth-union k multiplier
};

// Region identifiers for field-based skin weights.
enum class BodyRegion : int {
    RibCage = 0, Abdomen, Pelvis, Clavicle, Deltoid,
    UpperArm, Forearm, Neck, Head, RegionCount
};
constexpr int RegionCount = static_cast<int>(BodyRegion::RegionCount);

// Signed distance of the composed upper-body surface (negative inside).
// Left/right symmetry is guaranteed internally (paired regions mirrored).
float upperBodySdf(const HumanAnatomyParameters& params, const BodyJoints& joints, Vec3 p);

// Same field with the joint-derived primitives resolved ONCE and captured in
// the returned closure. extractSurfaceNets evaluates the field hundreds of
// thousands of times; rebuilding the primitive set per call was the dominant
// cost of the generation step.
std::function<float(Vec3)> makeUpperBodyField(const HumanAnatomyParameters& params,
                                             const BodyJoints& joints);

// Region field values at a point, evaluated on the point's own side
// (paired regions resolved left/right by the sign of p.x).
std::array<float, RegionCount> regionFields(const HumanAnatomyParameters& params,
                                            const BodyJoints& joints, Vec3 p);

// Maps a region to its primary skeleton bone for the given side (+1 left, -1 right).
uint16_t regionBone(BodyRegion region, int side);

} // namespace pae::anatomy
