#pragma once

// R4: the head as a dedicated anatomical surface.
//
// The previous head was a latitude/longitude ellipsoid with a jaw taper
// applied to every axis, which converges to a point at the bottom pole — the
// "cone chin" — and left the face features floating in front of a surface
// they were never anchored to. The face relief was then carved into that
// ellipsoid at a tessellation too coarse to resolve it.
//
// This module replaces that with a single signed-distance field describing
// the skull, the mid-face, the mandible, the zygomatic arches, the nose, the
// brow ridge and the neck, extracted with the project's own Surface Nets
// extractor. The result is one closed, continuous surface with no boundary
// edges, and every facial landmark is projected onto the *actual* extracted
// surface instead of being placed at an assumed depth.

#include "core/math/Math.h"
#include "procedural/MeshData.h"

#include <cstdint>
#include <functional>

namespace pae::head {

struct HeadParams {
    float width = 0.168f;   // biparietal breadth
    float height = 0.235f;  // menton -> vertex
    float depth = 0.190f;   // glabella -> occiput

    float jawWidth = 0.86f;
    float chinShape = 0.34f;
    float cheekVolume = 0.62f;
    float noseWidth = 0.68f;
    float noseBridge = 0.72f;
    float eyeSpacing = 0.95f;
    float eyeSize = 0.78f;
    float browHeight = 0.52f;
    float mouthWidth = 0.82f;

    float neckRadius = 0.058f;
};

// Facial landmarks resolved in head-local space: origin at the head centre,
// +Y up, +Z forward (the face direction), +X to the character's left.
struct HeadLandmarks {
    float rx = 0.084f;
    float ry = 0.117f;
    float rz = 0.095f;

    Vec3 eyeCenter[2]{};    // left, right — on the extracted surface
    Vec3 browCenter[2]{};
    Vec3 noseTip{};
    Vec3 noseBridgeTop{};
    Vec3 mouthCenter{};
    Vec3 earCenter[2]{};
    Vec3 chin{};
    Vec3 crown{};
    float eyeRadius = 0.009f;
};

// The distance field, kept public so the landmarks can be projected onto the
// same surface the mesh is extracted from.
using HeadField = std::function<float(Vec3)>;

// Builds the field and the landmarks for the given head placement.
// neckBaseY is the world-space height where the neck meets the shoulders.
HeadField buildHeadField(const HeadParams& params, Vec3 headCenter, float neckBaseY,
                          HeadLandmarks* outLandmarks);

// Extracts the head and neck as one closed surface.
MeshData generateHeadMesh(const HeadParams& params, Vec3 headCenter, float neckBaseY,
                           HeadLandmarks* outLandmarks, uint32_t resolution);

// The hair shell: the skull surface offset outward by `thickness` and cut at
// the hairline. Because it is derived from the same field as the head, it
// cannot float above the skull or leave a brim ring around it.
MeshData generateHairShell(const HeadParams& params, Vec3 headCenter, float neckBaseY,
                            float thickness, uint32_t resolution);

// The hairline height at a given head-local z (used to place hair strands).
float hairlineY(const HeadParams& params, float localZ);

// Hair strands that lie ON the skull. The previous generator placed its
// clumps at fixed ellipsoid coordinates, which left them floating above the
// new skull as a row of tubes. These are projected onto the extracted
// surface, so they follow the head instead of hovering over it.
MeshData generateHairStrands(const HeadParams& params, Vec3 headCenter, float neckBaseY,
                             int strandCount, float strandWidth, uint32_t radial);

} // namespace pae::head
