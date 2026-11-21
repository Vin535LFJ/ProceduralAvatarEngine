#pragma once

// M11-E: parametric head + face feature generation. The head is a profile-
// modulated sphere (forehead/temple/cheek/jaw/chin/back-head shape functions)
// instead of a bare ellipsoid; features are procedural geometry driven by
// FaceParams. Every parameter participates in generation.

#include "core/math/Math.h"
#include "procedural/MeshData.h"

#include <vector>

namespace pae::face {

struct FaceParams {
    const char* headShape = "oval"; // oval | round | square
    float jawWidth = 0.82f;         // jaw width relative to cheek width
    float chinShape = 0.42f;        // chin prominence (0 round .. 1 pointed)
    float cheekVolume = 0.68f;      // cheek fullness
    float eyeSpacing = 0.92f;       // eye spacing factor on head width
    float eyeSize = 1.0f;           // eye size multiplier
    float browHeight = 0.56f;       // brow height fraction (0 hairline .. 1 eyes)
    float noseWidth = 0.72f;        // nose width factor
    float noseBridge = 0.64f;       // nose bridge prominence
    float mouthWidth = 0.78f;       // mouth width factor
};

struct FacePiece {
    MeshData mesh;
    Vec3 center{0.0f, 0.0f, 0.0f}; // weight anchor (head bone)
    uint16_t materialId = 0;       // 0 skin, 1 hair, 3 eye, 4 ink
};

// Profile-modulated head mesh (radial ring grid, analytic-ish normals).
MeshData generateHeadMesh(const FaceParams& params, Vec3 headCenter,
                          float headWidth, float headHeight,
                          uint32_t radialSegments, uint32_t rings);

// Facial features assembled as independent pieces (eyes, brows, nose,
// mouth, ears) anchored to the head bone.
std::vector<FacePiece> generateFaceFeatures(const FaceParams& params,
                                            Vec3 headCenter, float headWidth,
                                            float headHeight, uint32_t radial);

} // namespace pae::face
