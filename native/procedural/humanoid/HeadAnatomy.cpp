#include "HeadAnatomy.h"

#include "procedural/fusion/SdfFusion.h"
#include "procedural/surfaces/ParametricSurfaces.h"

#include <algorithm>
#include <cmath>

namespace pae::head {

namespace {

constexpr float kPi = 3.14159265f;

// ---------------------------------------------------------------------------
// Signed distance helpers (negative inside, matching fusion::extractSurfaceNets).
// ---------------------------------------------------------------------------

float sdEllipsoid(Vec3 p, Vec3 c, Vec3 r) {
    const Vec3 q{(p.x - c.x) / r.x, (p.y - c.y) / r.y, (p.z - c.z) / r.z};
    const float k = std::min(std::min(r.x, r.y), r.z);
    return k * (q.length() - 1.0f);
}

float sdSphere(Vec3 p, Vec3 c, float r) {
    return (p - c).length() - r;
}

// Capsule with independent end radii (tapered limb).
float sdRoundCone(Vec3 p, Vec3 a, float ra, Vec3 b, float rb) {
    const Vec3 ab = b - a;
    const float l2 = ab.lengthSq();
    const float h = l2 > 1e-10f ? std::clamp((p - a).dot(ab) / l2, 0.0f, 1.0f) : 0.0f;
    const Vec3 closest = a + ab * h;
    return (p - closest).length() - (ra + (rb - ra) * h);
}

float sdSmoothUnion(float a, float b, float k) {
    const float h = std::clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
    return b * (1.0f - h) + a * h - k * h * (1.0f - h);
}

float sdSmoothSubtraction(float a, float b, float k) {
    const float nb = -b;
    const float h = std::clamp(0.5f - 0.5f * (nb - a) / k, 0.0f, 1.0f);
    return nb * (1.0f - h) + a * h - k * h * (1.0f - h);
}

// Bounding sphere test: a point outside (radius + k) cannot be influenced by
// the feature, so the whole primitive can be skipped. This is what keeps a
// head field with ~20 primitives affordable at 4 mm voxels.
struct Bound {
    Vec3 center;
    float radius;
};

Bound boundEllipsoid(Vec3 c, Vec3 r) { return Bound{c, r.length()}; }

Bound boundSegment(Vec3 a, Vec3 b, float r) {
    return Bound{(a + b) * 0.5f, (a - b).length() * 0.5f + r};
}

bool prune(const Bound& b, float k, Vec3 p) {
    const float reach = b.radius + k;
    return (p - b.center).lengthSq() > reach * reach;
}

float prunedUnion(float a, float b, float k, const Bound& bound, Vec3 p) {
    if (prune(bound, k, p)) return a;
    return sdSmoothUnion(a, b, k);
}

// ---------------------------------------------------------------------------
// The resolved primitive set. Built once, evaluated many times.
// ---------------------------------------------------------------------------

struct HeadPrimitives {
    float rx = 0.084f;
    float ry = 0.117f;
    float rz = 0.095f;

    Vec3 craniumC, craniumR;
    Vec3 faceC, faceR;
    Vec3 jawC, jawR;
    Vec3 mandibleA, mandibleB;
    float mandibleR;
    Vec3 chinC, chinR;
    Vec3 zygC;
    float zygR;
    Vec3 gonC;
    float gonR;
    Vec3 browA, browB;
    float browR;
    Vec3 noseA, noseB;
    float noseR;
    Vec3 noseTipC;
    float noseTipR;
    Vec3 alaC;
    float alaR;
    Vec3 upperLipC, upperLipR;
    Vec3 lowerLipC, lowerLipR;
    Vec3 earC, earR;
    Vec3 neckA, neckB;
    float neckRa, neckRb;
    Vec3 socketC, socketR;
    Vec3 eyeC, eyeR;
    Vec3 upperLidA, upperLidB, lowerLidA, lowerLidB;
    float lidR;
};

HeadPrimitives buildPrimitives(const HeadParams& p, float neckBaseLocalY) {
    HeadPrimitives m;
    m.rx = p.width * 0.5f;
    m.ry = p.height * 0.5f;
    m.rz = p.depth * 0.5f;
    const float rx = m.rx, ry = m.ry, rz = m.rz;

    // Skull vault. The widest point of a head sits above the ears, so the
    // vault is shifted up while the mass below it steps back in.
    m.craniumC = Vec3{0.0f, 0.20f * ry, -0.11f * rz};
    m.craniumR = Vec3{0.99f * rx, 0.80f * ry, 0.97f * rz};

    // Mid-face: maxilla and the orbital mass around the eyes.
    m.faceC = Vec3{0.0f, -0.14f * ry, 0.12f * rz};
    m.faceR = Vec3{0.82f * rx, 0.60f * ry, 0.64f * rz};

    // Mandible body: the mass that carries the jaw back to the ear.
    m.jawC = Vec3{0.0f, -0.56f * ry, -0.02f * rz};
    m.jawR = Vec3{0.74f * rx * (0.90f + 0.10f * p.jawWidth), 0.32f * ry, 0.60f * rz};

    // Mandible lower border: a V from the jaw angle forward to the chin. This
    // is what keeps the chin from collapsing into a single point.
    m.mandibleA = Vec3{0.52f * rx * p.jawWidth, -0.50f * ry, -0.04f * rz};
    m.mandibleB = Vec3{0.0f, -0.80f * ry, 0.20f * rz};
    m.mandibleR = 0.22f * rx;

    // Chin.
    m.chinC = Vec3{0.0f, -0.80f * ry, 0.24f * rz};
    m.chinR = Vec3{0.26f * rx * (0.85f + 0.30f * p.chinShape), 0.20f * ry, 0.26f * rz};

    // Zygomatic arch (cheekbone).
    m.zygC = Vec3{0.56f * rx, -0.08f * ry, 0.30f * rz};
    m.zygR = 0.21f * rx * (0.80f + 0.40f * p.cheekVolume);

    // Gonion (jaw angle).
    m.gonC = Vec3{0.50f * rx * p.jawWidth, -0.50f * ry, -0.04f * rz};
    m.gonR = 0.19f * rx;

    // Brow ridge: a low arc across the front, not a bar.
    m.browA = Vec3{-0.42f * rx, 0.08f * ry, 0.54f * rz};
    m.browB = Vec3{0.42f * rx, 0.08f * ry, 0.54f * rz};
    m.browR = 0.10f * rx;

    // Nose: bridge, tip and alae.
    m.noseA = Vec3{0.0f, 0.14f * ry, 0.54f * rz};
    m.noseB = Vec3{0.0f, -0.21f * ry, 0.76f * rz};
    m.noseR = 0.110f * rx * (0.75f + 0.35f * p.noseBridge);
    m.noseTipC = Vec3{0.0f, -0.26f * ry, 0.84f * rz};
    m.noseTipR = 0.150f * rx;
    m.alaC = Vec3{0.130f * rx * p.noseWidth, -0.30f * ry, 0.74f * rz};
    m.alaR = 0.105f * rx * p.noseWidth;

    // Lips.
    m.upperLipC = Vec3{0.0f, -0.48f * ry, 0.56f * rz};
    m.upperLipR = Vec3{0.19f * rx * p.mouthWidth, 0.055f * ry, 0.10f * rz};
    m.lowerLipC = Vec3{0.0f, -0.66f * ry, 0.54f * rz};
    m.lowerLipR = Vec3{0.17f * rx * p.mouthWidth, 0.055f * ry, 0.10f * rz};

    // Ear.
    m.earC = Vec3{0.94f * rx, -0.02f * ry, -0.14f * rz};
    m.earR = Vec3{0.075f * rx, 0.20f * ry, 0.15f * rz};

    // Neck: from inside the jaw down to the shoulders, flaring into the
    // trapezius so the neck-to-shoulder transition is one surface.
    m.neckA = Vec3{0.0f, -0.66f * ry, -0.16f * rz};
    m.neckRa = p.neckRadius * 0.88f;
    m.neckB = Vec3{0.0f, neckBaseLocalY - 0.004f, -0.22f * rz};
    m.neckRb = p.neckRadius;

    // Eye socket depression. The eyeball is unioned after the carve so the
    // socket frames the eye instead of erasing it.
    m.socketC = Vec3{0.36f * rx * p.eyeSpacing, -0.02f * ry, 0.60f * rz};
    m.socketR = Vec3{0.18f * rx, 0.060f * ry, 0.18f * rz};
    m.eyeC = Vec3{0.36f * rx * p.eyeSpacing, -0.02f * ry, 0.62f * rz};
    m.eyeR = Vec3{0.135f * rx * p.eyeSize, 0.065f * ry * p.eyeSize,
                  0.10f * rz * p.eyeSize};
    const float eyeX = m.eyeC.x;
    const float eyeY = m.eyeC.y;
    m.upperLidA = Vec3{eyeX - 0.17f * rx, eyeY + 0.070f * ry, 0.64f * rz};
    m.upperLidB = Vec3{eyeX + 0.17f * rx, eyeY + 0.070f * ry, 0.64f * rz};
    m.lowerLidA = Vec3{eyeX - 0.16f * rx, eyeY - 0.065f * ry, 0.62f * rz};
    m.lowerLidB = Vec3{eyeX + 0.16f * rx, eyeY - 0.065f * ry, 0.62f * rz};
    m.lidR = 0.045f * rx;

    return m;
}

// Ray-march the field from a point inside the head until it crosses the
// surface, then bisect for precision. This is how every facial landmark is
// anchored to the real geometry instead of an assumed depth.
Vec3 projectToSurface(const HeadField& field, Vec3 originLocal, Vec3 dirLocal, float maxDist) {
    const Vec3 dir = dirLocal.normalized(Vec3{0.0f, 0.0f, 1.0f});
    constexpr float kStep = 0.0012f;
    float previous = field(originLocal);
    for (float t = kStep; t <= maxDist; t += kStep) {
        const float d = field(originLocal + dir * t);
        if (d >= 0.0f && previous < 0.0f) {
            float lo = t - kStep;
            float hi = t;
            for (int i = 0; i < 14; ++i) {
                const float mid = 0.5f * (lo + hi);
                if (field(originLocal + dir * mid) < 0.0f) lo = mid; else hi = mid;
            }
            return originLocal + dir * (0.5f * (lo + hi));
        }
        previous = d;
    }
    return originLocal + dir * maxDist;
}

} // namespace

// ---------------------------------------------------------------------------
// Field construction
// ---------------------------------------------------------------------------

// The local-space field, resolved once. Shared by the head and the hair
// shell so the hair can never drift away from the skull it sits on.
using LocalField = std::function<float(Vec3)>;

LocalField buildLocalField(const HeadPrimitives& m, float neckBaseLocalY) {
    const Bound craniumBound = boundEllipsoid(m.craniumC, m.craniumR);
    const Bound faceBound = boundEllipsoid(m.faceC, m.faceR);
    const Bound jawBound = boundEllipsoid(m.jawC, m.jawR);
    const Bound mandibleBound = boundSegment(m.mandibleA, m.mandibleB, m.mandibleR);
    const Bound chinBound = boundEllipsoid(m.chinC, m.chinR);
    const Bound zygBound = boundEllipsoid(m.zygC, Vec3{m.zygR, m.zygR, m.zygR});
    const Bound gonBound = boundEllipsoid(m.gonC, Vec3{m.gonR, m.gonR, m.gonR});
    const Bound browBound = boundSegment(m.browA, m.browB, m.browR);
    const Bound noseBound = boundSegment(m.noseA, m.noseB, m.noseR);
    const Bound noseTipBound = boundEllipsoid(m.noseTipC, Vec3{m.noseTipR, m.noseTipR, m.noseTipR});
    const Bound alaBound = boundEllipsoid(m.alaC, Vec3{m.alaR, m.alaR, m.alaR});
    const Bound lipBound = boundEllipsoid(m.upperLipC, m.upperLipR);
    const Bound lowerLipBound = boundEllipsoid(m.lowerLipC, m.lowerLipR);
    const Bound earBound = boundEllipsoid(m.earC, m.earR);
    const Bound neckBound = boundSegment(m.neckA, m.neckB, m.neckRa);
    const Bound socketBound = boundEllipsoid(m.socketC, m.socketR);
    const Bound eyeBound = boundEllipsoid(m.eyeC, m.eyeR);
    const Bound upperLidBound = boundSegment(m.upperLidA, m.upperLidB, m.lidR);
    const Bound lowerLidBound = boundSegment(m.lowerLidA, m.lowerLidB, m.lidR);

    return [=](Vec3 p) -> float {
        constexpr float kFar = 1e6f;
        const float kUnion = 0.030f;
        const float kUnionSmall = 0.016f;

        float d = prune(craniumBound, kUnion, p)
            ? kFar
            : sdEllipsoid(p, m.craniumC, m.craniumR);
        d = prunedUnion(d, sdEllipsoid(p, m.faceC, m.faceR), kUnion, faceBound, p);
        d = prunedUnion(d, sdEllipsoid(p, m.jawC, m.jawR), kUnion, jawBound, p);
        d = prunedUnion(d, sdRoundCone(p, m.mandibleA, m.mandibleR, m.mandibleB, m.mandibleR),
                        kUnionSmall, mandibleBound, p);
        d = prunedUnion(d, sdEllipsoid(p, m.chinC, m.chinR), kUnionSmall, chinBound, p);
        d = prunedUnion(d, sdSphere(p, m.zygC, m.zygR), kUnionSmall, zygBound, p);
        d = prunedUnion(d, sdSphere(p, m.gonC, m.gonR), kUnionSmall, gonBound, p);
        d = prunedUnion(d, sdRoundCone(p, m.browA, m.browR, m.browB, m.browR),
                        kUnionSmall, browBound, p);
        d = prunedUnion(d, sdRoundCone(p, m.noseA, m.noseR, m.noseB, m.noseR),
                        kUnionSmall, noseBound, p);
        d = prunedUnion(d, sdSphere(p, m.noseTipC, m.noseTipR), kUnionSmall, noseTipBound, p);
        d = prunedUnion(d, sdSphere(p, m.alaC, m.alaR), kUnionSmall, alaBound, p);
        d = prunedUnion(d, sdEllipsoid(p, m.upperLipC, m.upperLipR), kUnionSmall, lipBound, p);
        d = prunedUnion(d, sdEllipsoid(p, m.lowerLipC, m.lowerLipR), kUnionSmall,
                        lowerLipBound, p);
        d = prunedUnion(d, sdEllipsoid(p, m.earC, m.earR), kUnionSmall, earBound, p);
        d = prunedUnion(d, sdRoundCone(p, m.neckA, m.neckRa, m.neckB, m.neckRb),
                        0.040f, neckBound, p);

        // The neck is cut flat at the torso's neck ring. Without this the
        // round-cone cap bulges below the sampling volume and the extracted
        // surface is left open (boundary edges) at the neck root.
        d = std::max(d, neckBaseLocalY - p.y);

        // Carved detail. The shallow socket is cut before the eyeball is
        // added, leaving a lid-sized rim rather than a large crater.
        if (!prune(socketBound, 0.004f, p)) {
            d = sdSmoothSubtraction(d, sdEllipsoid(p, m.socketC, m.socketR), 0.004f);
        }
        d = prunedUnion(d, sdEllipsoid(p, m.eyeC, m.eyeR), 0.003f, eyeBound, p);
        d = prunedUnion(d, sdRoundCone(p, m.upperLidA, m.lidR, m.upperLidB, m.lidR),
                        0.002f, upperLidBound, p);
        d = prunedUnion(d, sdRoundCone(p, m.lowerLidA, m.lidR, m.lowerLidB, m.lidR),
                        0.002f, lowerLidBound, p);
        return d;
    };
}

float hairlineY(const HeadParams& params, float localZ) {
    // The hairline runs high across the forehead, dips at the temples and
    // drops to the nape at the back of the skull.
    const float ry = params.height * 0.5f;
    const float rz = params.depth * 0.5f;
    if (localZ >= 0.0f) {
        const float t = std::clamp(localZ / std::max(rz, 1e-4f), 0.0f, 1.0f);
        return ry * (0.26f + 0.20f * t);
    }
    const float t = std::clamp(-localZ / std::max(rz, 1e-4f), 0.0f, 1.0f);
    return ry * (0.26f - 0.78f * t);
}

HeadField buildHeadField(const HeadParams& params, Vec3 headCenter, float neckBaseY,
                          HeadLandmarks* outLandmarks) {
    const float neckBaseLocalY = neckBaseY - headCenter.y;
    const HeadPrimitives m = buildPrimitives(params, neckBaseLocalY);
    const LocalField local = buildLocalField(m, neckBaseLocalY);

    // World-space field. The left/right pairs are mirrored explicitly so the
    // head stays symmetric by construction.
    auto world = [local, headCenter](Vec3 p) -> float {
        const Vec3 q = p - headCenter;
        const float left = local(q);
        const float right = local(Vec3{-q.x, q.y, q.z});
        return std::min(left, right);
    };

    if (outLandmarks) {
        HeadLandmarks lm;
        lm.rx = m.rx;
        lm.ry = m.ry;
        lm.rz = m.rz;

        const float eyeX = 0.36f * m.rx * params.eyeSpacing;
        const float eyeY = -0.02f * m.ry;
        const float browY = eyeY + m.ry * (0.10f + 0.12f * params.browHeight);
        for (int i = 0; i < 2; ++i) {
            const float side = i == 0 ? 1.0f : -1.0f;
            const Vec3 eyeOrigin{side * eyeX, eyeY, 0.0f};
            const Vec3 eyeLocal = projectToSurface(world, headCenter + eyeOrigin,
                                                   Vec3{0.0f, 0.0f, 1.0f}, 0.12f);
            lm.eyeCenter[i] = eyeLocal;

            const Vec3 browOrigin{side * eyeX * 1.02f, browY, 0.0f};
            lm.browCenter[i] = projectToSurface(world, headCenter + browOrigin,
                                                 Vec3{0.0f, 0.0f, 1.0f}, 0.12f);

            const Vec3 earOrigin{side * m.rx * 0.60f, -0.02f * m.ry, -0.14f * m.rz};
            lm.earCenter[i] = projectToSurface(world, headCenter + earOrigin,
                                                Vec3{side, 0.0f, 0.0f}, 0.08f);
        }
        lm.noseTip = projectToSurface(world, headCenter + Vec3{0.0f, -0.22f * m.ry, 0.0f},
                                      Vec3{0.0f, -0.15f, 1.0f}, 0.14f);
        lm.noseBridgeTop = projectToSurface(world, headCenter + Vec3{0.0f, 0.12f * m.ry, 0.0f},
                                             Vec3{0.0f, 0.0f, 1.0f}, 0.14f);
        lm.mouthCenter = projectToSurface(world, headCenter + Vec3{0.0f, -0.57f * m.ry, 0.0f},
                                          Vec3{0.0f, 0.0f, 1.0f}, 0.14f);
        lm.chin = projectToSurface(world, headCenter + Vec3{0.0f, -0.86f * m.ry, 0.0f},
                                   Vec3{0.0f, -0.45f, 1.0f}, 0.12f);
        lm.crown = headCenter + Vec3{0.0f, m.ry * 1.00f, -0.02f * m.rz};
        lm.eyeRadius = m.rx * 0.072f * params.eyeSize;
        *outLandmarks = lm;
    }
    return world;
}

MeshData generateHeadMesh(const HeadParams& params, Vec3 headCenter, float neckBaseY,
                           HeadLandmarks* outLandmarks, uint32_t resolution) {
    HeadField field = buildHeadField(params, headCenter, neckBaseY, outLandmarks);

    // Bounding box: ears out to the sides, the neck down to the shoulders,
    // the crown up and the occiput back.
    const float rx = params.width * 0.5f;
    const float ry = params.height * 0.5f;
    const float rz = params.depth * 0.5f;
    const float neckBaseLocalY = neckBaseY - headCenter.y;
    const float topY = headCenter.y + ry * 1.12f;
    const float bottomY = headCenter.y + neckBaseLocalY - 0.04f;
    const float heightY = topY - bottomY;

    fusion::FusionVolume volume;
    volume.center = Vec3{headCenter.x, (topY + bottomY) * 0.5f, headCenter.z};
    volume.size = Vec3{rx * 2.40f, heightY, rz * 2.50f};
    const uint32_t cells = std::max(24u, resolution);
    // Cells per axis are sized from the physical extent so the voxel size
    // stays roughly isotropic whatever the head proportions are.
    const float voxel = 0.0042f;
    volume.resolution = {
        std::max(16, static_cast<int>(volume.size.x / voxel)),
        std::max(24, static_cast<int>(volume.size.y / voxel)),
        std::max(16, static_cast<int>(volume.size.z / voxel))};
    (void)cells;

    MeshData mesh = fusion::extractSurfaceNets(volume, field, nullptr);
    if (!mesh.vertices.empty() && mesh.subMeshes.empty()) {
        SubMesh sub;
        sub.indexCount = static_cast<uint32_t>(mesh.indices.size());
        mesh.subMeshes.push_back(sub);
    }
    return mesh;
}

MeshData generateHairShell(const HeadParams& params, Vec3 headCenter, float neckBaseY,
                            float thickness, uint32_t resolution) {
    const float neckBaseLocalY = neckBaseY - headCenter.y;
    const HeadPrimitives m = buildPrimitives(params, neckBaseLocalY);
    const LocalField local = buildLocalField(m, neckBaseLocalY);

    // The shell is the skull surface pushed outward by `thickness`, intersected
    // with the scalp region above the hairline. Because it reuses the head
    // field, the hair cannot float above the skull and the old brim ring
    // (the equator of a full ellipsoid cap) cannot reappear.
    auto field = [local, headCenter, &params, thickness](Vec3 p) -> float {
        const Vec3 q = p - headCenter;
        const float left = local(q);
        const float right = local(Vec3{-q.x, q.y, q.z});
        const float skull = std::min(left, right);
        // The band between the skull surface and its outward offset. Using
        // (skull - thickness) alone would enclose the whole head interior.
        const float shell = std::max(-skull, skull - thickness);
        // Scalp region: the hair exists above the hairline only.
        const float scalp = hairlineY(params, q.z) - q.y;
        return std::max(shell, scalp);
    };

    const float rx = params.width * 0.5f;
    const float ry = params.height * 0.5f;
    const float rz = params.depth * 0.5f;
    const float topY = headCenter.y + ry * 1.16f;
    const float bottomY = headCenter.y + neckBaseLocalY - 0.04f;

    fusion::FusionVolume volume;
    volume.center = Vec3{headCenter.x, (topY + bottomY) * 0.5f, headCenter.z};
    volume.size = Vec3{rx * 2.40f, topY - bottomY, rz * 2.50f};
    const float voxel = 0.0050f;
    volume.resolution = {
        std::max(16, static_cast<int>(volume.size.x / voxel)),
        std::max(24, static_cast<int>(volume.size.y / voxel)),
        std::max(16, static_cast<int>(volume.size.z / voxel))};
    (void)resolution;

    MeshData mesh = fusion::extractSurfaceNets(volume, field, nullptr);
    if (!mesh.vertices.empty() && mesh.subMeshes.empty()) {
        SubMesh sub;
        sub.indexCount = static_cast<uint32_t>(mesh.indices.size());
        mesh.subMeshes.push_back(sub);
    }
    return mesh;
}

MeshData generateHairStrands(const HeadParams& params, Vec3 headCenter, float neckBaseY,
                             int strandCount, float strandWidth, uint32_t radial) {
    const float neckBaseLocalY = neckBaseY - headCenter.y;
    const HeadPrimitives m = buildPrimitives(params, neckBaseLocalY);
    const LocalField local = buildLocalField(m, neckBaseLocalY);
    auto field = [local, headCenter](Vec3 p) -> float {
        const Vec3 q = p - headCenter;
        return std::min(local(q), local(Vec3{-q.x, q.y, q.z}));
    };

    const float rx = params.width * 0.5f;
    const float rz = params.depth * 0.5f;
    const int count = std::max(6, strandCount);

    MeshData mesh;
    // A strand is a tapered tube whose points are projected onto the skull,
    // so it lies on the head instead of hovering over it.
    auto emitStrand = [&](float azimuth, float startDrop, float length, float width) {
        std::vector<Vec3> points;
        const Vec3 radialDir{std::sin(azimuth), 0.0f, std::cos(azimuth)};
        for (int s = 0; s < 4; ++s) {
            const float t = static_cast<float>(s) / 3.0f;
            // Start just above the hairline and walk down the scalp.
            const float localZ = std::cos(azimuth) * rz * 0.55f;
            const float y = hairlineY(params, localZ) - startDrop - length * t;
            const Vec3 origin = headCenter + Vec3{radialDir.x * rx * 0.30f, y,
                                                  radialDir.z * rz * 0.30f};
            const Vec3 surface = projectToSurface(field, origin, radialDir, 0.12f);
            points.push_back(surface + radialDir * (0.0012f + 0.0016f * (1.0f - t)));
        }
        // Build the tube from the sampled points.
        std::vector<Vec3> centerline;
        std::vector<float> radii;
        for (size_t i = 0; i < points.size(); ++i) {
            centerline.push_back(points[i]);
            radii.push_back(width * (1.0f - 0.72f * (static_cast<float>(i) /
                                                     static_cast<float>(points.size() - 1))));
        }
        const uint32_t segments = std::max(6u, radial / 2);
        const MeshData tube = generateSweepSurface(centerline, radii, segments);
        const uint32_t first = static_cast<uint32_t>(mesh.vertices.size());
        for (const Vertex& v : tube.vertices) mesh.vertices.push_back(v);
        for (uint32_t index : tube.indices) mesh.indices.push_back(first + index);
    };

    // Front fringe, then the sides, then the back.
    const int frontCount = std::max(4, count / 2);
    for (int i = 0; i < frontCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(frontCount - 1);
        const float azimuth = (-0.95f + t * 1.90f);
        emitStrand(azimuth, 0.004f, 0.055f, strandWidth);
    }
    const int sideCount = std::max(4, count / 4);
    for (int side = -1; side <= 1; side += 2) {
        for (int i = 0; i < sideCount; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(sideCount - 1);
            const float azimuth = side * (1.35f + t * 0.75f);
            emitStrand(azimuth, 0.010f, 0.075f, strandWidth * 0.9f);
        }
    }
    const int backCount = std::max(4, count / 3);
    for (int i = 0; i < backCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(backCount - 1);
        const float azimuth = kPi + (-0.85f + t * 1.70f);
        emitStrand(azimuth, 0.006f, 0.090f, strandWidth * 0.8f);
    }

    Aabb bounds;
    for (const Vertex& v : mesh.vertices) bounds.expand(v.position);
    mesh.bounds = bounds;
    if (!mesh.subMeshes.empty()) {
        for (SubMesh& sub : mesh.subMeshes) sub.indexCount = static_cast<uint32_t>(mesh.indices.size());
    } else {
        SubMesh sub;
        sub.indexCount = static_cast<uint32_t>(mesh.indices.size());
        mesh.subMeshes.push_back(sub);
    }
    return mesh;
}

} // namespace pae::head
