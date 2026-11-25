#pragma once

// M11-E: facial feature generation. The features are separate pieces (eyes,
// brows, nose tip, mouth) anchored to the head bone.
//
// R4 change: the features are no longer placed at assumed depths relative to
// an ellipsoid. They are projected onto the *actual* extracted head surface
// (see HeadAnatomy.h), so an eye can no longer float in front of the face or
// sink behind the brow ridge.

#include "core/math/Math.h"
#include "procedural/MeshData.h"
#include "procedural/humanoid/HeadAnatomy.h"

#include <vector>

namespace pae::face {

struct FaceParams {
    const char* headShape = "oval"; // oval | round | square
    float jawWidth = 0.86f;         // jaw width relative to cheek width (natural)
    float chinShape = 0.34f;        // chin prominence (0 round .. 1 pointed)
    float cheekVolume = 0.62f;      // cheek fullness
    float eyeSpacing = 0.95f;       // eye spacing factor on head width
    float eyeSize = 0.78f;          // natural adult eye size, not cartoon-large
    float browHeight = 0.52f;       // brow height fraction (0 hairline .. 1 eyes)
    float noseWidth = 0.68f;        // nose width factor
    float noseBridge = 0.72f;       // nose bridge prominence
    float mouthWidth = 0.82f;       // mouth width factor
};

struct FacePiece {
    MeshData mesh;
    Vec3 center{0.0f, 0.0f, 0.0f}; // weight anchor (head bone)
    uint16_t materialId = 0;       // 0 skin, 1 hair, 3 eye, 4 ink
};

// Facial features assembled as independent pieces (eyes, brows, mouth)
// placed on the landmarks resolved from the extracted head surface.
std::vector<FacePiece> generateFaceFeatures(const FaceParams& params,
                                            const head::HeadLandmarks& landmarks,
                                            uint32_t radial);

} // namespace pae::face
