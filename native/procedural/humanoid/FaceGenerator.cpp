#include "FaceGenerator.h"

#include "procedural/primitives/Primitives.h"
#include "procedural/surfaces/ParametricSurfaces.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace pae::face {

namespace {

constexpr float kPi = 3.14159265f;
constexpr float kTwoPi = 6.2831853f;

float clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }

float smooth(float a, float b, float x) {
    const float t = clamp01((x - a) / (b - a));
    return t * t * (3.0f - 2.0f * t);
}

float lerpF(float a, float b, float t) { return a + (b - a) * t; }

MeshData translateMesh(const MeshData& source, const Vec3& offset) {
    MeshData result = source;
    for (Vertex& vertex : result.vertices) vertex.position = vertex.position + offset;
    Aabb bounds;
    for (const Vertex& vertex : result.vertices) bounds.expand(vertex.position);
    result.bounds = bounds;
    return result;
}

} // namespace

std::vector<FacePiece> generateFaceFeatures(const FaceParams& fp,
                                            const head::HeadLandmarks& lm,
                                            uint32_t radial) {
    std::vector<FacePiece> pieces;
    auto add = [&pieces](MeshData m, Vec3 center, uint16_t materialId) {
        pieces.push_back({std::move(m), center, materialId});
    };

    // The eye opening is an almond hole in the skull surface; the sclera is
    // placed inside it and the iris just behind the front of the sclera.
    const float eyeRadius = lm.eyeRadius;
    for (int i = 0; i < 2; ++i) {
        const Vec3 surface = lm.eyeCenter[i];
        // Recess the eyeball into the socket: the front of the sclera sits a
        // little behind the surrounding surface, not in front of it.
        const Vec3 scleraCenter = surface - Vec3{0.0f, 0.0f, eyeRadius * 0.30f};
        add(translateMesh(generateEllipsoid(
                Vec3{eyeRadius * 1.10f, eyeRadius * 0.46f, eyeRadius * 0.34f}, radial, radial / 2),
            scleraCenter), scleraCenter, 3);
        add(translateMesh(generateEllipsoid(
                Vec3{eyeRadius * 0.38f, eyeRadius * 0.38f, eyeRadius * 0.16f}, radial, radial / 2),
            scleraCenter + Vec3{0.0f, 0.0f, eyeRadius * 0.26f}), scleraCenter, 4);

        // Brow: a thin arc of hair colour sitting on the brow ridge.
        const Vec3 brow = lm.browCenter[i];
        add(translateMesh(generateEllipsoid(
                Vec3{eyeRadius * 1.05f, eyeRadius * 0.12f, eyeRadius * 0.16f}, radial, radial / 2),
            brow + Vec3{0.0f, 0.0f, eyeRadius * 0.10f}), brow, 1);
    }

    // Nose: the tip is already modelled in the head field; only a small
    // cartilage highlight is added, placed on the resolved tip landmark.
    const float noseW = lm.rx * fp.noseWidth * 0.30f;
    add(translateMesh(generateEllipsoid(
            Vec3{noseW * 0.42f, noseW * 0.34f, noseW * 0.26f}, radial / 2 + 2, radial / 2),
        lm.noseTip + Vec3{0.0f, 0.0f, noseW * 0.10f}), lm.noseTip, 0);

    // Mouth: a single thin dark line in the carved slot.
    const float mouthHalfW = lm.rx * fp.mouthWidth * 0.26f;
    add(translateMesh(generateEllipsoid(
            Vec3{mouthHalfW, mouthHalfW * 0.035f, mouthHalfW * 0.05f}, radial / 2 + 2, radial / 2),
        lm.mouthCenter + Vec3{0.0f, 0.0f, mouthHalfW * 0.03f}), lm.mouthCenter, 4);

    // Ears are part of the head field now; the landmarks are kept for the
    // hair generator so sideburns can start at the correct place.
    return pieces;
}

} // namespace pae::face
