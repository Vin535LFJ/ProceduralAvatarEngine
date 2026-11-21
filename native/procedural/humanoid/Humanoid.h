#pragma once

#include "procedural/MeshData.h"
#include "skeleton/Skeleton.h"

namespace pae {

struct BodyParameters {
    float height = 1.70f;
    float headScale = 1.12f;   // enlarged cartoon head-to-body ratio (M10)
    float headWidth = 0.20f;
    float headHeight = 0.25f;
    float shoulderWidth = 0.42f;
    float chestWidth = 0.36f;
    float waistWidth = 0.30f;
    float hipWidth = 0.34f;
    float torsoLength = 0.52f;
    float neckLength = 0.10f;
    float upperArmLength = 0.30f;
    float lowerArmLength = 0.28f;
    float armRadius = 0.062f;
    float upperLegLength = 0.42f;
    float lowerLegLength = 0.40f;
    float legRadius = 0.082f;
    float handScale = 1.15f;
    float footLength = 0.24f;
    float footWidth = 0.10f;
    float stylization = 1.0f;
    uint32_t detailLevel = 1;
    uint64_t seed = 20261121;
};

struct HumanoidResult {
    MeshData mesh;
    Skeleton skeleton;
    BodyJoints joints;
    bool valid = false;
};

BodyParameters clampBodyParameters(const BodyParameters& params);
HumanoidResult generateHumanoid(const BodyParameters& params, bool weightHeatmap = false);

} // namespace pae
