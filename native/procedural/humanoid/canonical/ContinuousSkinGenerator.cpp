#include "ContinuousSkinGenerator.h"

#include "core/diagnostics/Log.h"
#include "procedural/fusion/SdfFusion.h"
#include "procedural/humanoid/HeadAnatomy.h"
#include "skeleton/weights/WeightGenerator.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <limits>

namespace pae::canonical {
namespace {

struct Primitive {
    enum class Kind { Ellipsoid, SuperEllipsoid, TaperedCapsule } kind = Kind::Ellipsoid;
    Vec3 a{};
    Vec3 b{};
    Vec3 radii{};
    float radius = 0.0f;
    float endRadius = 0.0f;
    float blend = 0.012f;
};

struct BodyField {
    std::vector<Primitive> body;
    std::vector<Primitive> digits;
    std::vector<Aabb> digitBounds;
    float digitBlend = 0.006f;
};

float pointSegmentDistanceSq(Vec3 p, Vec3 a, Vec3 b) {
    const Vec3 ab = b - a;
    const float lengthSq = ab.lengthSq();
    const float t = lengthSq > 1e-10f
        ? std::clamp((p - a).dot(ab) / lengthSq, 0.0f, 1.0f) : 0.0f;
    return (p - (a + ab * t)).lengthSq();
}

bool contains(const Aabb& bounds, Vec3 point) {
    return bounds.valid && point.x >= bounds.min.x && point.x <= bounds.max.x &&
           point.y >= bounds.min.y && point.y <= bounds.max.y &&
           point.z >= bounds.min.z && point.z <= bounds.max.z;
}

void addEllipsoid(std::vector<Primitive>& primitives, Vec3 center, Vec3 radii, float blend) {
    primitives.push_back({Primitive::Kind::Ellipsoid, center, {}, radii, 0.0f, 0.0f, blend});
}

void addSuperEllipsoid(std::vector<Primitive>& primitives, Vec3 center, Vec3 radii, float blend) {
    primitives.push_back({Primitive::Kind::SuperEllipsoid, center, {}, radii, 0.0f, 0.0f, blend});
}

void addTaperedCapsule(std::vector<Primitive>& primitives, Vec3 a, Vec3 b,
                       float radius, float endRadius, float blend) {
    primitives.push_back({Primitive::Kind::TaperedCapsule, a, b, {}, radius, endRadius, blend});
}

void addCapsule(std::vector<Primitive>& primitives, Vec3 a, Vec3 b, float radius, float blend) {
    addTaperedCapsule(primitives, a, b, radius, radius, blend);
}

float primitiveDistance(const Primitive& primitive, Vec3 point) {
    if (primitive.kind == Primitive::Kind::Ellipsoid) {
        return fusion::sdEllipsoidApprox(point, primitive.a, primitive.radii);
    }
    if (primitive.kind == Primitive::Kind::SuperEllipsoid) {
        const Vec3 q{(point.x - primitive.a.x) / primitive.radii.x,
                     (point.y - primitive.a.y) / primitive.radii.y,
                     (point.z - primitive.a.z) / primitive.radii.z};
        constexpr float exponent = 2.6f;
        const float norm = std::pow(std::pow(std::fabs(q.x), exponent) +
                                    std::pow(std::fabs(q.y), exponent) +
                                    std::pow(std::fabs(q.z), exponent), 1.0f / exponent);
        return std::min(primitive.radii.x, std::min(primitive.radii.y, primitive.radii.z)) *
               (norm - 1.0f);
    }
    const Vec3 ab = primitive.b - primitive.a;
    const float lengthSq = ab.lengthSq();
    const float t = lengthSq > 1e-10f
        ? std::clamp((point - primitive.a).dot(ab) / lengthSq, 0.0f, 1.0f) : 0.0f;
    const float radius = primitive.radius + (primitive.endRadius - primitive.radius) * t;
    return (point - (primitive.a + ab * t)).length() - radius;
}

BodyField buildBodyField(const BodyParameters& p, const BodyJoints& j, float scale,
                         bool includeDigits) {
    BodyField field;
    std::vector<Primitive>& primitives = field.body;
    primitives.reserve(32);
    const float torsoBlend = 0.018f * scale;
    const float jointBlend = 0.014f * scale;
    const Vec3 chestCenter = (j.spine + j.chest) * 0.5f;
    addSuperEllipsoid(primitives, j.hips + Vec3{0.0f, 0.035f * scale, 0.0f},
                      {p.hipWidth * scale * 0.53f, 0.145f * scale,
                       p.hipWidth * p.pelvisShape.hipDepth * scale * 0.85f}, torsoBlend);
    addSuperEllipsoid(primitives, (j.hips + j.spine) * 0.5f + Vec3{0.0f, 0.035f * scale, 0.0f},
                      {p.waistWidth * scale * 0.53f, 0.180f * scale,
                       p.waistWidth * scale * 0.31f}, torsoBlend);
    addSuperEllipsoid(primitives, chestCenter,
                      {p.chestWidth * scale * 0.54f, 0.200f * scale,
                       p.chestWidth * p.torsoShape.chestDepth * scale * 0.30f}, torsoBlend);
    addTaperedCapsule(primitives, j.neck - Vec3{0.0f, 0.055f * scale, 0.0f},
                      j.neck + Vec3{0.0f, 0.025f * scale, 0.0f},
                      0.058f * scale, 0.050f * scale, jointBlend);

    auto addDigit = [&](Vec3 a, Vec3 b, float radius, float endRadius) {
        if (!includeDigits) return;
        field.digits.push_back({Primitive::Kind::TaperedCapsule, a, b, {},
                                radius, endRadius, 0.0f});
        const float margin = radius + 0.012f * scale;
        Aabb bounds;
        bounds.expand(a - Vec3{margin, margin, margin});
        bounds.expand(a + Vec3{margin, margin, margin});
        bounds.expand(b - Vec3{margin, margin, margin});
        bounds.expand(b + Vec3{margin, margin, margin});
        field.digitBounds.push_back(bounds);
    };

    auto addArm = [&](const Vec3& shoulder, const Vec3& elbow, const Vec3& wrist,
                      const Vec3& hand, int side) {
        const float armRadius = p.armRadius * scale;
        const float elbowBlend = 0.018f * scale;
        addSuperEllipsoid(primitives, shoulder + Vec3{side * 0.008f * scale, -0.020f * scale, 0.0f},
                          {0.055f * scale * p.armShape.deltoidVolume,
                           0.064f * scale * p.armShape.deltoidVolume,
                           0.045f * scale * p.armShape.deltoidVolume}, jointBlend);
        addTaperedCapsule(primitives, shoulder, elbow,
                          armRadius * p.armShape.upperArmWidth,
                          armRadius * p.armShape.elbowShape * 0.78f, elbowBlend);
        addTaperedCapsule(primitives, elbow, wrist,
                          armRadius * p.armShape.elbowShape * 0.82f,
                          armRadius * p.armShape.wristPinch, elbowBlend);
        addCapsule(primitives, wrist, hand, armRadius * p.armShape.wristPinch, jointBlend);
        addEllipsoid(primitives, hand - Vec3{0.0f, 0.022f * scale, 0.0f},
                     {0.044f * scale * p.handScale, 0.059f * scale * p.handScale,
                      0.027f * scale * p.handScale}, 0.006f * scale);

        const float fingerX[4] = {-0.030f, -0.010f, 0.010f, 0.030f};
        const float fingerLength[4] = {0.052f, 0.064f, 0.060f, 0.048f};
        for (size_t finger = 0; finger < 4; ++finger) {
            const Vec3 start = hand + Vec3{side * fingerX[finger] * scale,
                                           -0.048f * scale, 0.001f * scale};
            const Vec3 end = start + Vec3{side * 0.002f * scale,
                                          -fingerLength[finger] * scale * p.handScale,
                                          0.006f * scale};
            addDigit(start, end, (0.0072f - finger * 0.0004f) * scale,
                     (0.0060f - finger * 0.0003f) * scale);
        }
        const Vec3 thumbStart = hand + Vec3{-side * 0.037f * scale,
                                            -0.018f * scale, 0.004f * scale};
        const Vec3 thumbEnd = hand + Vec3{-side * 0.061f * scale,
                                          -0.063f * scale, 0.020f * scale};
        addDigit(thumbStart, thumbEnd, 0.0095f * scale, 0.0075f * scale);
    };
    addArm(j.leftShoulder, j.leftElbow, j.leftWrist, j.leftHand, 1);
    addArm(j.rightShoulder, j.rightElbow, j.rightWrist, j.rightHand, -1);

    auto addLeg = [&](const Vec3& hip, const Vec3& knee, const Vec3& ankle,
                      const Vec3& toe, int side) {
        const float legRadius = p.legRadius * scale;
        addTaperedCapsule(primitives, hip, knee,
                          legRadius * p.legShape.thighVolume,
                          legRadius * p.legShape.kneeShape * 0.72f, jointBlend);
        addEllipsoid(primitives, knee,
                     {legRadius * p.legShape.kneeShape * 0.68f,
                      legRadius * p.legShape.kneeShape * 0.76f,
                      legRadius * p.legShape.kneeShape * 0.66f}, jointBlend);
        addTaperedCapsule(primitives, knee, ankle,
                          legRadius * p.legShape.calfVolume * 0.72f,
                          legRadius * p.legShape.anklePinch, jointBlend);
        addEllipsoid(primitives, ankle - Vec3{0.0f, 0.030f * scale, 0.005f * scale},
                     {p.footWidth * scale * 0.44f, 0.040f * scale,
                      0.055f * scale}, 0.012f * scale);
        addEllipsoid(primitives, ankle - Vec3{0.0f, 0.034f * scale,
                                              p.footLength * scale * 0.30f},
                     {p.footWidth * scale * 0.52f, 0.034f * scale,
                      p.footLength * scale * 0.35f}, 0.012f * scale);
        addCapsule(primitives, ankle, toe, legRadius * p.legShape.anklePinch, 0.012f * scale);

        const float toeX[5] = {-0.040f, -0.020f, 0.0f, 0.020f, 0.040f};
        const float toeLength[5] = {0.045f, 0.042f, 0.038f, 0.033f, 0.028f};
        for (size_t digit = 0; digit < 5; ++digit) {
            const Vec3 start = ankle + Vec3{side * toeX[digit] * scale,
                                            -0.052f * scale,
                                            p.footLength * scale * 0.55f};
            const Vec3 end = start + Vec3{0.0f, -0.002f * scale,
                                          toeLength[digit] * scale};
            addDigit(start, end, (0.0095f - digit * 0.0004f) * scale,
                     (0.0080f - digit * 0.0004f) * scale);
        }
    };
    addLeg(j.leftHip, j.leftKnee, j.leftAnkle, j.leftToe, 1);
    addLeg(j.rightHip, j.rightKnee, j.rightAnkle, j.rightToe, -1);
    return field;
}

uint16_t materialForRegion(const Vertex& vertex, const BodyParameters& p,
                           const BodyJoints& joints, float scale) {
    if (vertex.position.y > joints.neck.y - 0.025f * scale) return 1;
    if (vertex.position.y < joints.hips.y - 0.06f * scale) {
        return vertex.position.y < joints.leftAnkle.y + 0.06f * scale ? 4 : 3;
    }
    if (std::fabs(vertex.position.x) > p.shoulderWidth * scale * 0.56f &&
        vertex.position.y < joints.leftShoulder.y + 0.04f * scale) return 2;
    return 0;
}

void assignCanonicalWeights(MeshData& mesh, const Skeleton& skeleton, bool heatmap) {
    const auto boneDistance = [&skeleton](Vec3 point, uint16_t bone) {
        const BoneDefinition& definition = skeleton.bones[bone];
        return pointSegmentDistanceSq(point, definition.head, definition.tail);
    };
    const auto chainDistance = [&boneDistance](Vec3 point, uint16_t a, uint16_t b, uint16_t c) {
        return std::min({boneDistance(point, a), boneDistance(point, b), boneDistance(point, c)});
    };

    for (Vertex& vertex : mesh.vertices) {
        float leftArm = chainDistance(vertex.position, BoneLeftShoulder,
                                      BoneLeftUpperArm, BoneLeftLowerArm);
        float rightArm = chainDistance(vertex.position, BoneRightShoulder,
                                       BoneRightUpperArm, BoneRightLowerArm);
        leftArm = std::fmin(leftArm, boneDistance(vertex.position, BoneLeftHand));
        rightArm = std::fmin(rightArm, boneDistance(vertex.position, BoneRightHand));
        float leftLeg = chainDistance(vertex.position, BoneLeftUpperLeg,
                                      BoneLeftLowerLeg, BoneLeftFoot);
        float rightLeg = chainDistance(vertex.position, BoneRightUpperLeg,
                                       BoneRightLowerLeg, BoneRightFoot);
        leftLeg = std::fmin(leftLeg, boneDistance(vertex.position, BoneLeftToe));
        rightLeg = std::fmin(rightLeg, boneDistance(vertex.position, BoneRightToe));
        const float torso = chainDistance(vertex.position, BoneHips, BoneSpine, BoneChest);
        const float head = std::fmin(boneDistance(vertex.position, BoneNeck),
                                     boneDistance(vertex.position, BoneHead));
        const float distances[6] = {head, leftArm, rightArm, leftLeg, rightLeg, torso};
        size_t region = 0;
        for (size_t candidate = 1; candidate < 6; ++candidate) {
            if (distances[candidate] < distances[region]) region = candidate;
        }

        std::array<uint16_t, 5> candidates{};
        size_t candidateCount = 0;
        switch (region) {
            case 0:
                candidates = {BoneNeck, BoneHead, 0, 0, 0};
                candidateCount = 2;
                break;
            case 1:
                candidates = {BoneChest, BoneLeftShoulder, BoneLeftUpperArm,
                              BoneLeftLowerArm, BoneLeftHand};
                candidateCount = 5;
                break;
            case 2:
                candidates = {BoneChest, BoneRightShoulder, BoneRightUpperArm,
                              BoneRightLowerArm, BoneRightHand};
                candidateCount = 5;
                break;
            case 3:
                candidates = {BoneHips, BoneLeftUpperLeg, BoneLeftLowerLeg,
                              BoneLeftFoot, BoneLeftToe};
                candidateCount = 5;
                break;
            case 4:
                candidates = {BoneHips, BoneRightUpperLeg, BoneRightLowerLeg,
                              BoneRightFoot, BoneRightToe};
                candidateCount = 5;
                break;
            default:
                candidates = {BoneHips, BoneSpine, BoneChest, 0, 0};
                candidateCount = 3;
                break;
        }

        std::array<float, 4> best{};
        std::array<uint16_t, 4> bones{};
        for (size_t index = 0; index < candidateCount; ++index) {
            const uint16_t bone = candidates[index];
            const float distanceSq = boneDistance(vertex.position, bone);
            const float weight = 1.0f / (distanceSq + 0.0004f);
            for (size_t slot = 0; slot < best.size(); ++slot) {
                if (weight > best[slot] ||
                    (weight == best[slot] && weight > 0.0f && bone < bones[slot])) {
                    for (size_t shift = best.size() - 1; shift > slot; --shift) {
                        best[shift] = best[shift - 1];
                        bones[shift] = bones[shift - 1];
                    }
                    best[slot] = weight;
                    bones[slot] = bone;
                    break;
                }
            }
        }
        const float sum = best[0] + best[1] + best[2] + best[3];
        const float inverse = sum > 1e-8f ? 1.0f / sum : 0.0f;
        vertex.joints = {bones[0], bones[1], bones[2], bones[3]};
        vertex.weights = {best[0] * inverse, best[1] * inverse,
                          best[2] * inverse, best[3] * inverse};
        if (heatmap) {
            Vec3 color{};
            for (size_t slot = 0; slot < 4; ++slot) {
                color = color + weights::boneColor(bones[slot]) * best[slot] * inverse;
            }
            vertex.color = Vec4{color.x, color.y, color.z, vertex.color.w};
        } else {
            vertex.color = Vec4{0.72f, 0.72f, 0.72f, vertex.color.w};
        }
    }
}

bool fillSmallBoundaryHoles(MeshData& mesh, size_t& holeCount) {
    const std::vector<BoundaryLoop> loops = extractBoundaryLoops(mesh);
    if (loops.empty()) return true;
    std::vector<Vec3> normalAdditions(mesh.vertices.size());
    for (const BoundaryLoop& loop : loops) {
        if (loop.vertexIds.size() < 3 || loop.vertexIds.size() > 64) return false;
        Vec3 center{};
        Vec3 outward{};
        Vec3 loopNormal{};
        float diameterSq = 0.0f;
        for (size_t i = 0; i < loop.vertexIds.size(); ++i) {
            const Vertex& vertex = mesh.vertices[loop.vertexIds[i]];
            center = center + vertex.position;
            outward = outward + vertex.normal;
            loopNormal = loopNormal + vertex.position.cross(
                mesh.vertices[loop.vertexIds[(i + 1) % loop.vertexIds.size()]].position);
            for (size_t j = i + 1; j < loop.vertexIds.size(); ++j) {
                diameterSq = std::fmax(diameterSq,
                    (vertex.position - mesh.vertices[loop.vertexIds[j]].position).lengthSq());
            }
        }
        if (diameterSq > 0.08f * 0.08f) return false;
        center = center * (1.0f / static_cast<float>(loop.vertexIds.size()));
        const Vec3 inward = outward.lengthSq() > 1e-12f
            ? outward.normalized() * -1.0f
            : loopNormal.normalized(Vec3{0.0f, 0.0f, 1.0f}) * -1.0f;
        center = center + inward * 0.001f;
        bool capValid = false;
        for (size_t attempt = 0; attempt < 4 && !capValid; ++attempt) {
            capValid = true;
            for (size_t i = 0; i < loop.vertexIds.size(); ++i) {
                const Vec3& a = mesh.vertices[loop.vertexIds[i]].position;
                const Vec3& b = mesh.vertices[loop.vertexIds[(i + 1) % loop.vertexIds.size()]].position;
                if ((a - center).cross(b - center).lengthSq() < 1e-14f) {
                    capValid = false;
                    break;
                }
            }
            if (!capValid) center = center + inward * 0.002f;
        }
        if (!capValid) return false;

        std::array<float, BoneCount> accumulated{};
        for (uint32_t id : loop.vertexIds) {
            const Vertex& source = mesh.vertices[id];
            const uint16_t bones[4] = {source.joints.x, source.joints.y,
                                       source.joints.z, source.joints.w};
            const float values[4] = {source.weights.x, source.weights.y,
                                     source.weights.z, source.weights.w};
            for (size_t slot = 0; slot < 4; ++slot) {
                if (bones[slot] < BoneCount && std::isfinite(values[slot]) && values[slot] > 0.0f) {
                    accumulated[bones[slot]] += values[slot];
                }
            }
        }
        std::array<float, 4> best{};
        std::array<uint16_t, 4> bones{};
        for (uint16_t bone = 0; bone < BoneCount; ++bone) {
            for (size_t slot = 0; slot < best.size(); ++slot) {
                if (accumulated[bone] > best[slot] ||
                    (accumulated[bone] == best[slot] && accumulated[bone] > 0.0f && bone < bones[slot])) {
                    for (size_t shift = best.size() - 1; shift > slot; --shift) {
                        best[shift] = best[shift - 1];
                        bones[shift] = bones[shift - 1];
                    }
                    best[slot] = accumulated[bone];
                    bones[slot] = bone;
                    break;
                }
            }
        }
        const float sum = best[0] + best[1] + best[2] + best[3];
        if (!(sum > 1e-8f)) return false;
        Vertex cap;
        cap.position = center;
        cap.joints = {bones[0], bones[1], bones[2], bones[3]};
        cap.weights = {best[0] / sum, best[1] / sum, best[2] / sum, best[3] / sum};
        cap.color = mesh.vertices[loop.vertexIds.front()].color;
        const uint32_t centerId = static_cast<uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back(cap);
        normalAdditions.push_back({});
        const bool reverse = loopNormal.dot(outward) < 0.0f;
        for (size_t i = 0; i < loop.vertexIds.size(); ++i) {
            const size_t next = (i + 1) % loop.vertexIds.size();
            const uint32_t a = reverse ? loop.vertexIds[next] : loop.vertexIds[i];
            const uint32_t b = reverse ? loop.vertexIds[i] : loop.vertexIds[next];
            mesh.indices.insert(mesh.indices.end(), {centerId, a, b});
            const Vec3 face = (mesh.vertices[a].position - cap.position).cross(
                mesh.vertices[b].position - cap.position);
            normalAdditions[centerId] = normalAdditions[centerId] + face;
            normalAdditions[a] = normalAdditions[a] + face;
            normalAdditions[b] = normalAdditions[b] + face;
        }
        ++holeCount;
    }
    for (size_t i = 0; i < normalAdditions.size(); ++i) {
        if (normalAdditions[i].lengthSq() <= 1e-16f) continue;
        mesh.vertices[i].normal = (mesh.vertices[i].normal + normalAdditions[i]).normalized(
            mesh.vertices[i].normal);
    }
    return true;
}

} // namespace

ContinuousSkinResult generateContinuousSkin(const BodyParameters& params, bool weightHeatmap) {
    const auto started = std::chrono::steady_clock::now();
    ContinuousSkinResult result;
    const BodyParameters p = clampBodyParameters(params);
    const float estimatedHeight = p.upperLegLength + p.lowerLegLength + 0.08f +
                                  p.torsoLength + p.neckLength + p.headHeight;
    const float scale = std::clamp(p.height / std::fmax(estimatedHeight, 0.1f), 0.5f, 1.8f);
    result.joints = deriveJoints(p, scale);
    result.skeleton = buildHumanoidSkeleton(result.joints);
    const BodyJoints& j = result.joints;
    const BodyField anatomy = buildBodyField(p, j, scale, p.detailLevel > 0);
    head::HeadParams headParams;
    headParams.width = p.headWidth * scale;
    headParams.height = p.headHeight * scale;
    headParams.depth = p.headHeight * scale * 0.81f;
    headParams.jawWidth = p.face.jawWidth;
    headParams.chinShape = p.face.chinShape;
    headParams.cheekVolume = p.face.cheekVolume;
    headParams.noseWidth = p.face.noseWidth;
    headParams.noseBridge = p.face.noseBridge;
    headParams.eyeSpacing = p.face.eyeSpacing;
    headParams.eyeSize = p.face.eyeSize;
    headParams.browHeight = p.face.browHeight;
    headParams.mouthWidth = p.face.mouthWidth;
    headParams.neckRadius = 0.055f * scale;
    const head::HeadField headField = head::buildHeadField(headParams, j.head, j.neck.y, nullptr);
    Aabb headBounds;
    headBounds.expand({-headParams.width * 0.70f, j.neck.y - 0.04f * scale,
                       -headParams.depth * 0.75f});
    headBounds.expand({headParams.width * 0.70f,
                       j.head.y + headParams.height * 0.62f,
                       headParams.depth * 0.75f});
    const auto field = [&anatomy, &headField, &headBounds](Vec3 point) {
        float distance = std::numeric_limits<float>::max();
        for (const Primitive& primitive : anatomy.body) {
            const float value = primitiveDistance(primitive, point);
            if (distance == std::numeric_limits<float>::max()) distance = value;
            else distance = fusion::sdSmoothUnion(distance, value, primitive.blend);
        }
        bool nearDigits = false;
        for (const Aabb& bounds : anatomy.digitBounds) {
            if (contains(bounds, point)) {
                nearDigits = true;
                break;
            }
        }
        if (nearDigits) {
            float digitDistance = std::numeric_limits<float>::max();
            for (const Primitive& digit : anatomy.digits) {
                digitDistance = std::fmin(digitDistance, primitiveDistance(digit, point));
            }
            distance = fusion::sdSmoothUnion(distance, digitDistance, anatomy.digitBlend);
        }
        if (contains(headBounds, point)) {
            distance = fusion::sdSmoothUnion(distance, headField(point), 0.010f);
        }
        return distance;
    };

    const float detail = std::clamp(static_cast<float>(p.detailLevel), 0.0f, 3.0f);
    const float minY = -0.08f * scale;
    const float maxY = j.head.y + p.headHeight * scale * 0.58f + 0.14f * scale;
    fusion::FusionVolume volume;
    volume.center = {0.0f, (minY + maxY) * 0.5f, p.footLength * scale * 0.08f};
    volume.size = {2.0f * (std::fabs(j.leftWrist.x) + 0.18f * scale),
                   maxY - minY,
                   std::fmax(0.48f * scale, p.footLength * scale + 0.36f * scale)};
    volume.resolution = {static_cast<int>(150 + detail * 18),
                         static_cast<int>(210 + detail * 26),
                         static_cast<int>(72 + detail * 10)};
    const auto extractionStarted = std::chrono::steady_clock::now();
    result.canonical.mesh = fusion::extractSurfaceNets(volume, field, nullptr, true);
    result.extractionMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - extractionStarted).count();
    if (result.canonical.mesh.vertices.empty()) {
        PAE_LOGE("PAE_GEOMETRY", "continuous skin extraction produced no geometry");
        return result;
    }
    std::vector<uint32_t> nonDegenerateIndices;
    nonDegenerateIndices.reserve(result.canonical.mesh.indices.size());
    for (size_t i = 0; i + 2 < result.canonical.mesh.indices.size(); i += 3) {
        const uint32_t a = result.canonical.mesh.indices[i];
        const uint32_t b = result.canonical.mesh.indices[i + 1];
        const uint32_t c = result.canonical.mesh.indices[i + 2];
        const Vec3 pa = result.canonical.mesh.vertices[a].position;
        const Vec3 pb = result.canonical.mesh.vertices[b].position;
        const Vec3 pc = result.canonical.mesh.vertices[c].position;
        if ((pb - pa).cross(pc - pa).lengthSq() < 1e-14f) continue;
        nonDegenerateIndices.insert(nonDegenerateIndices.end(), {a, b, c});
    }
    result.canonical.mesh.indices = std::move(nonDegenerateIndices);
    for (Vertex& vertex : result.canonical.mesh.vertices) {
        vertex.color = Vec4{0.72f, 0.72f, 0.72f,
                            static_cast<float>(materialForRegion(vertex, p, j, scale))};
    }
    const auto weightStarted = std::chrono::steady_clock::now();
    assignCanonicalWeights(result.canonical.mesh, result.skeleton, weightHeatmap);
    result.weightMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - weightStarted).count();
    fillSmallBoundaryHoles(result.canonical.mesh, result.repairedBoundaryHoles);
    if (!result.canonical.mesh.subMeshes.empty()) {
        result.canonical.mesh.subMeshes[0].indexCount =
            static_cast<uint32_t>(result.canonical.mesh.indices.size());
    }
    result.canonical.topology = analyzeTopology(result.canonical.mesh);
    result.canonical.weights = validateWeights(result.canonical.mesh);
    result.canonical.boundaryLoops = extractBoundaryLoops(result.canonical.mesh);
    const auto intersectionStarted = std::chrono::steady_clock::now();
    result.canonical.selfIntersections = detectSelfIntersections(result.canonical.mesh);
    result.selfIntersectionMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - intersectionStarted).count();
    result.canonical.valid = result.canonical.topology.components == 1 &&
                             result.canonical.topology.boundaryEdges == 0 &&
                             result.canonical.topology.nonManifoldEdges == 0 &&
                             result.canonical.topology.degenerateTriangles == 0 &&
                             result.canonical.topology.duplicateVertices == 0 &&
                             result.canonical.topology.invalidNormals == 0 &&
                             result.canonical.weights.invalidVertices == 0 &&
                             result.canonical.selfIntersections.intersectingPairs == 0;
    result.generationMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - started).count();
    const TopologyReport& topology = result.canonical.topology;
    PAE_LOGI("PAE_GEOMETRY",
             "P1 continuous skin: verts=%zu tris=%ld components=%d boundary=%d nonmanifold=%d "
             "degen=%d dup=%d badnorm=%d weights_bad=%zu self_intersections=%zu candidates=%zu holes=%zu "
             "extraction_ms=%.1f weight_ms=%.1f intersection_ms=%.1f total_ms=%.1f",
             topology.vertices, topology.triangles, topology.components, topology.boundaryEdges,
             topology.nonManifoldEdges, topology.degenerateTriangles, topology.duplicateVertices,
             topology.invalidNormals, result.canonical.weights.invalidVertices,
             result.canonical.selfIntersections.intersectingPairs,
             result.canonical.selfIntersections.candidatePairs,
             result.repairedBoundaryHoles, result.extractionMs, result.weightMs,
             result.selfIntersectionMs, result.generationMs);
    return result;
}

} // namespace pae::canonical
