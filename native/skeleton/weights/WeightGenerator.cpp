#include "WeightGenerator.h"

#include <algorithm>
#include <cmath>

namespace pae::weights {

namespace {

constexpr float kDistanceEpsilonSq = 0.0004f; // 2 cm soft knee for 1/(d^2+eps^2)
constexpr float kUnitTolerance = 1e-5f;
// Blend reach for hierarchy-neighbour bones: beyond this distance a
// neighbour bone may not influence the vertex (keeps rigid regions rigid).
constexpr float kNeighborReach = 0.17f;
constexpr float kNeighborReachSq = kNeighborReach * kNeighborReach;

float pointSegmentDistanceSq(Vec3 p, Vec3 a, Vec3 b) {
    const Vec3 ab = b - a;
    const float abLen2 = ab.lengthSq();
    const float t = abLen2 > 1e-10f
        ? std::clamp((p - a).dot(ab) / abLen2, 0.0f, 1.0f)
        : 0.0f;
    const Vec3 closest = a + ab * t;
    return (p - closest).lengthSq();
}

void addHierarchyNeighbors(const Skeleton& skeleton, const std::vector<uint16_t>& pathBones,
                           std::vector<uint16_t>& out) {
    for (uint16_t bone : pathBones) {
        if (bone >= BoneCount) continue;
        const int16_t parent = skeleton.bones[bone].parent;
        if (parent >= 0) out.push_back(static_cast<uint16_t>(parent));
        for (uint16_t child = 0; child < BoneCount; ++child) {
            if (skeleton.bones[child].parent == static_cast<int16_t>(bone)) out.push_back(child);
        }
    }
}

} // namespace

Vec3 boneColor(uint16_t bone) {
    const float r = static_cast<float>(bone % 5) / 4.0f;
    const float g = static_cast<float>((bone / 5) % 5) / 4.0f;
    const float b = static_cast<float>((bone / 25) % 3) / 2.0f;
    return {0.25f + 0.75f * r, 0.25f + 0.75f * g, 0.25f + 0.75f * b};
}

void assignWeights(PartSpec& part, const Skeleton& skeleton, bool heatmap) {
    if (!part.mesh || !part.path || !part.pathBones) return;
    if (part.path->empty() || part.path->size() != part.pathBones->size()) return;

    // Candidate bones: the part's own path bones plus hierarchy neighbours
    // (parents/children) so vertices near a part boundary blend continuously.
    std::vector<uint16_t> candidates(*part.pathBones);
    addHierarchyNeighbors(skeleton, *part.pathBones, candidates);
    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
    candidates.erase(std::remove_if(candidates.begin(), candidates.end(),
                                    [](uint16_t b) { return b >= BoneCount; }),
                     candidates.end());
    if (candidates.empty()) return;

    // Influence segments per candidate bone: path segments that reference the
    // bone; hierarchy-only bones use their skeleton head->tail segment.
    std::vector<std::vector<std::pair<Vec3, Vec3>>> segments(candidates.size());
    for (size_t i = 0; i + 1 < part.path->size(); ++i) {
        const uint16_t b0 = (*part.pathBones)[i];
        const uint16_t b1 = (*part.pathBones)[i + 1];
        const std::pair<Vec3, Vec3> segment{(*part.path)[i], (*part.path)[i + 1]};
        for (size_t c = 0; c < candidates.size(); ++c) {
            if (candidates[c] == b0 || candidates[c] == b1) segments[c].push_back(segment);
        }
    }
    for (size_t c = 0; c < candidates.size(); ++c) {
        if (segments[c].empty()) {
            const BoneDefinition& def = skeleton.bones[candidates[c]];
            segments[c].push_back({def.head, def.tail});
        }
    }

    for (Vertex& vertex : part.mesh->vertices) {
        float best[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        uint16_t bones[4] = {0, 0, 0, 0};
        for (size_t c = 0; c < candidates.size(); ++c) {
            float distSq = std::numeric_limits<float>::max();
            for (const auto& segment : segments[c]) {
                distSq = std::min(distSq, pointSegmentDistanceSq(vertex.position,
                                                                 segment.first, segment.second));
            }
            const bool isPathCandidate = std::find(part.pathBones->begin(),
                                                   part.pathBones->end(),
                                                   candidates[c]) != part.pathBones->end();
            if (!isPathCandidate && distSq > kNeighborReachSq) continue; // hard reach
            const float weight = 1.0f / (distSq + kDistanceEpsilonSq);
            // Insert into the top-4 list.
            for (int slot = 0; slot < 4; ++slot) {
                if (weight > best[slot]) {
                    for (int shift = 3; shift > slot; --shift) {
                        best[shift] = best[shift - 1];
                        bones[shift] = bones[shift - 1];
                    }
                    best[slot] = weight;
                    bones[slot] = candidates[c];
                    break;
                }
            }
        }
        // Path-dominance cap: hierarchy neighbours blend into the region but
        // never dominate it (keeps rigid parts like the head head-driven).
        bool isPathBone[4] = {false, false, false, false};
        float pathSum = 0.0f, nonPathSum = 0.0f;
        for (int slot = 0; slot < 4; ++slot) {
            for (uint16_t b : *part.pathBones) {
                if (bones[slot] == b) {
                    isPathBone[slot] = true;
                    break;
                }
            }
            if (isPathBone[slot]) pathSum += best[slot];
            else nonPathSum += best[slot];
        }
        const float total = pathSum + nonPathSum;
        if (total > 1e-10f && nonPathSum > 0.30f * total) {
            const float scale = (0.30f * total) / nonPathSum;
            nonPathSum *= scale;
            pathSum = total - nonPathSum;
            for (int slot = 0; slot < 4; ++slot) {
                if (!isPathBone[slot]) best[slot] *= scale;
            }
        }

        const float sum = best[0] + best[1] + best[2] + best[3];
        const float inv = sum > 1e-10f ? 1.0f / sum : 0.0f;
        const float w0 = best[0] * inv;
        const float w1 = best[1] * inv;
        const float w2 = best[2] * inv;
        const float w3 = best[3] * inv;
        vertex.joints = {bones[0], bones[1], bones[2], bones[3]};
        vertex.weights = {w0, w1, w2, w3};

        if (heatmap) {
            const Vec3 skin = Vec3{0.95f, 0.75f, 0.62f};
            Vec3 blend{};
            for (int slot = 0; slot < 4; ++slot) {
                const Vec3 c0 = boneColor(bones[slot]);
                blend = blend + c0 * (slot == 0 ? w0 : slot == 1 ? w1 : slot == 2 ? w2 : w3);
            }
            vertex.color = Vec4{blend.x * 0.8f + skin.x * 0.2f,
                                blend.y * 0.8f + skin.y * 0.2f,
                                blend.z * 0.8f + skin.z * 0.2f,
                                static_cast<float>(part.materialId)};
        } else {
            Vec3 tint;
            switch (part.materialId) {
                case 0: tint = {0.95f, 0.75f, 0.62f}; break;
                case 1: tint = {0.22f, 0.13f, 0.10f}; break;
                case 2: tint = {0.25f, 0.42f, 0.72f}; break;
                case 3: tint = {0.97f, 0.97f, 0.92f}; break;
                case 4: tint = {0.05f, 0.04f, 0.06f}; break;
                default: tint = {0.8f, 0.8f, 0.8f}; break;
            }
            const float shade = 0.85f + 0.15f * std::clamp(vertex.normal.y * 0.5f + 0.5f, 0.0f, 1.0f);
            vertex.color = {tint.x * shade, tint.y * shade, tint.z * shade,
                            static_cast<float>(part.materialId)};
        }
        // Keep weights numerically clean for the validator (sum == 1).
        const float weightSum = vertex.weights.x + vertex.weights.y +
                                vertex.weights.z + vertex.weights.w;
        if (std::fabs(weightSum - 1.0f) > kUnitTolerance) {
            const float fix = weightSum > 1e-10f ? 1.0f / weightSum : 0.0f;
            vertex.weights = {vertex.weights.x * fix, vertex.weights.y * fix,
                              vertex.weights.z * fix, vertex.weights.w * fix};
        }
    }
}

} // namespace pae::weights
