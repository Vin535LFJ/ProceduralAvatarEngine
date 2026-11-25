#include "TorsoShoulderPatchGenerator.h"

#include "LocalShapeBasis.h"
#include "TopologyTemplate.h"
#include "procedural/humanoid/canonical/CanonicalSkinGenerator.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <utility>
#include <unordered_map>

namespace pae::character {
namespace {

constexpr uint32_t kTorsoSegments = 32;
constexpr uint32_t kTorsoRings = 11;
constexpr uint32_t kArmSamples = 6;
constexpr uint32_t kBridgeSteps = 4;
constexpr float kPi = 3.14159265f;

float smooth01(float value) {
    const float t = std::clamp(value, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float mix(float a, float b, float t) { return a + (b - a) * t; }

struct TorsoSection {
    float width = 0.15f;
    float depth = 0.10f;
    float centerZ = 0.0f;
};

TorsoSection torsoSection(float t, const shape::v2::CharacterDefinition& definition,
                          const Anthropometry& anthropometry, const BodyJoints& joints) {
    const float hipW = anthropometry.hipWidthM * 0.5f * 0.92f *
                       definition.torso.upperPelvisWidthScale;
    const float waistW = anthropometry.waistWidthM * 0.5f *
                         (1.0f - 0.20f * definition.torso.waistPinch);
    const float chestW = anthropometry.chestWidthM * 0.5f * definition.torso.ribcageWidthScale;
    const float shoulderW = anthropometry.shoulderBreadthM * 0.5f *
                            definition.torso.shoulderGirdleMass;

    const float hipD = anthropometry.hipWidthM * 0.30f;
    const float waistD = anthropometry.waistDepthM * 0.5f;
    const float chestD = anthropometry.chestDepthM * 0.5f * definition.torso.chestDepthScale;
    const float shoulderD = chestD * 0.98f;

    float width = 0.0f;
    float depth = 0.0f;
    if (t < 0.28f) {
        const float u = smooth01(t / 0.28f);
        width = mix(hipW, waistW, u);
        depth = mix(hipD, waistD, u);
    } else if (t < 0.64f) {
        const float u = smooth01((t - 0.28f) / 0.36f);
        width = mix(waistW, chestW, u);
        depth = mix(waistD, chestD, u);
    } else {
        const float u = smooth01((t - 0.64f) / 0.36f);
        width = mix(chestW, shoulderW, u);
        depth = mix(chestD, shoulderD, u);
    }
    const float chestForward = 0.012f * smooth01((t - 0.35f) / 0.45f);
    const float spineCurve = 0.006f * std::sin(t * kPi);
    return {width, depth, chestForward + spineCurve};
}

std::vector<float> torsoProfile(float t, const shape::v2::CharacterDefinition& definition) {
    std::vector<float> controls = {
        0.965f, 0.985f, 1.025f, 1.085f, 1.030f, 0.995f,
        0.965f, 0.980f, 0.995f, 0.925f, 0.970f, 0.985f
    };
    const float chestBoost = 0.025f * smooth01((t - 0.38f) / 0.35f);
    controls[2] += chestBoost * 0.5f;
    controls[3] += chestBoost;
    controls[4] += chestBoost * 0.5f;
    const float clavicle = definition.torso.clavicleRelief * 0.20f *
                           smooth01((t - 0.78f) / 0.22f);
    controls[2] += clavicle;
    controls[4] += clavicle;
    return controls;
}

uint32_t addTrackedVertex(MeshData& mesh,
                          std::vector<RegionId>& regions,
                          std::vector<uint64_t>& stableIds,
                          const Vec3& position, RegionId region, uint8_t side,
                          uint32_t localId) {
    Vertex vertex;
    vertex.position = position;
    vertex.normal = Vec3{0.0f, 1.0f, 0.0f};
    vertex.color = Vec4{0.72f, 0.72f, 0.72f, 1.0f};
    const uint32_t index = static_cast<uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back(vertex);
    regions.push_back(region);
    stableIds.push_back(makeStableVertexId(region, side, localId));
    return index;
}

void addTriangleOriented(MeshData& mesh, uint32_t a, uint32_t b, uint32_t c,
                         const Vec3& outwardHint) {
    const Vec3& pa = mesh.vertices[a].position;
    const Vec3& pb = mesh.vertices[b].position;
    const Vec3& pc = mesh.vertices[c].position;
    const Vec3 normal = (pb - pa).cross(pc - pa);
    if (normal.dot(outwardHint) < 0.0f) std::swap(b, c);
    mesh.indices.insert(mesh.indices.end(), {a, b, c});
}

void addQuadOriented(MeshData& mesh, uint32_t a, uint32_t b, uint32_t c, uint32_t d,
                     const Vec3& outwardHint) {
    addTriangleOriented(mesh, a, b, d, outwardHint);
    addTriangleOriented(mesh, a, d, c, outwardHint);
}

void recomputeNormals(MeshData& mesh) {
    for (Vertex& vertex : mesh.vertices) vertex.normal = Vec3{};
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        Vertex& a = mesh.vertices[mesh.indices[i]];
        Vertex& b = mesh.vertices[mesh.indices[i + 1]];
        Vertex& c = mesh.vertices[mesh.indices[i + 2]];
        const Vec3 normal = (b.position - a.position).cross(c.position - a.position);
        a.normal = a.normal + normal;
        b.normal = b.normal + normal;
        c.normal = c.normal + normal;
    }
    for (Vertex& vertex : mesh.vertices) {
        vertex.normal = vertex.normal.normalized(Vec3{0.0f, 1.0f, 0.0f});
    }
}

struct BuiltArm {
    std::vector<std::vector<uint32_t>> rings;
    Vec3 rootCenter{};
};

BuiltArm buildArm(const std::vector<uint32_t>& rootRing, const Vec3& elbow, bool left,
                  const shape::v2::CharacterDefinition& definition,
                  const Anthropometry& anthropometry, MeshData& mesh,
                  std::vector<RegionId>& regions, std::vector<uint64_t>& stableIds,
                  uint32_t& localCounter, SweepDiagnostics* diagnostics) {
    BuiltArm arm;
    if (rootRing.size() < 3) return arm;
    const uint8_t side = left ? 1 : 2;
    const RegionId region = left ? RegionId::UpperArmRootLeft : RegionId::UpperArmRootRight;
    const RegionId bridgeRegion = left ? RegionId::BridgeLeft : RegionId::BridgeRight;
    const float sideScale = (left ? definition.shoulders.leftWidthScale *
                                      definition.upperArms.leftRadiusScale *
                                      (1.0f + definition.asymmetry.leftUpperArmRadiusDelta)
                                 : definition.shoulders.rightWidthScale *
                                      definition.upperArms.rightRadiusScale *
                                      (1.0f + definition.asymmetry.rightUpperArmRadiusDelta));
    Vec3 rootCenter{};
    for (uint32_t index : rootRing) rootCenter = rootCenter + mesh.vertices[index].position;
    arm.rootCenter = rootCenter * (1.0f / static_cast<float>(rootRing.size()));
    const Vec3 outward{left ? 1.0f : -1.0f, 0.0f, 0.0f};
    const Vec3 armDirection = (elbow - arm.rootCenter).normalized(outward);
    const Vec3 shoulderCenter = arm.rootCenter + armDirection * 0.050f + outward * 0.030f;
    std::vector<Vec3> path;
    path.reserve(kArmSamples);
    for (uint32_t i = 0; i < kArmSamples; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kArmSamples - 1);
        path.push_back(shoulderCenter + (elbow - shoulderCenter) * t);
    }
    const std::vector<SweepFrame> frames = computeRmfFrames(path, Vec3{0.0f, 0.0f, 1.0f}, diagnostics);
    if (frames.size() != path.size()) return arm;

    std::vector<uint32_t> orderedRoot = rootRing;
    double rootArea2 = 0.0;
    for (size_t i = 0; i < orderedRoot.size(); ++i) {
        const Vec3 a = mesh.vertices[orderedRoot[i]].position - arm.rootCenter;
        const Vec3 b = mesh.vertices[orderedRoot[(i + 1) % orderedRoot.size()]].position - arm.rootCenter;
        rootArea2 += static_cast<double>(a.dot(frames[0].u)) * b.dot(frames[0].v) -
                     static_cast<double>(b.dot(frames[0].u)) * a.dot(frames[0].v);
    }
    if (rootArea2 < 0.0) std::reverse(orderedRoot.begin(), orderedRoot.end());

    const float baseRadius = std::max(0.042f, anthropometry.chestWidthM * 0.155f) * sideScale;
    Vec3 phaseRadial = mesh.vertices[orderedRoot.front()].position - arm.rootCenter;
    phaseRadial = phaseRadial - frames[0].tangent * phaseRadial.dot(frames[0].tangent);
    const float profilePhase = std::atan2(phaseRadial.dot(frames[0].v),
                                          phaseRadial.dot(frames[0].u)) / 6.2831853f;
    const std::vector<float> profile = {
        0.98f, 1.00f, 1.02f, 1.035f, 1.015f, 0.99f,
        0.98f, 0.99f, 1.01f, 1.02f, 1.00f, 0.98f
    };
    const uint32_t ringSegments = static_cast<uint32_t>(orderedRoot.size());
    std::vector<uint32_t> firstRing;
    std::vector<uint32_t> bridgeRing;
    firstRing.reserve(ringSegments);
    bridgeRing.reserve(ringSegments);
    for (uint32_t s = 0; s < ringSegments; ++s) {
        Vec3 radial = mesh.vertices[orderedRoot[s]].position - arm.rootCenter;
        radial = radial - frames[0].tangent * radial.dot(frames[0].tangent);
        if (radial.lengthSq() < 1e-8f) radial = frames[0].u;
        radial = radial.normalized(frames[0].u) * baseRadius * 0.96f;
        const Vec3 armPosition = path[0] + radial;
        const Vec3 rootPosition = mesh.vertices[orderedRoot[s]].position;
        const Vec3 bridgePosition = (rootPosition + armPosition) * 0.5f +
                                    outward * 0.008f + Vec3{0.0f, 0.010f, 0.0f};
        firstRing.push_back(addTrackedVertex(mesh, regions, stableIds, armPosition, region, side,
                                            localCounter++));
        bridgeRing.push_back(addTrackedVertex(mesh, regions, stableIds, bridgePosition,
                                             bridgeRegion, side, localCounter++));
    }
    arm.rings.reserve(kArmSamples);
    arm.rings.push_back(std::move(firstRing));
    for (uint32_t row = 0; row < 2; ++row) {
        const std::vector<uint32_t>& from = row == 0 ? orderedRoot : bridgeRing;
        const std::vector<uint32_t>& to = row == 0 ? bridgeRing : arm.rings.front();
        for (uint32_t s = 0; s < ringSegments; ++s) {
            const uint32_t sn = (s + 1) % ringSegments;
            addQuadOriented(mesh, from[s], from[sn], to[s], to[sn], outward);
        }
    }
    for (uint32_t i = 1; i < kArmSamples; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kArmSamples - 1);
        const float deltoid = 0.16f * definition.shoulders.deltoidVolume *
                              compactFalloff(std::fabs(t - 0.10f), 0.34f);
        const float radius = baseRadius * (1.0f + deltoid) * (1.0f - 0.25f * t) *
                             mix(1.0f, definition.upperArms.elbowRadiusScale, smooth01(t));
        std::vector<uint32_t> ring;
        ring.reserve(ringSegments);
        for (uint32_t s = 0; s < ringSegments; ++s) {
            const float u = profilePhase + static_cast<float>(s) / static_cast<float>(ringSegments);
            const Vec2 section = sampleClosedProfile(profile, u, radius, radius * 0.88f);
            const Vec3 position = path[i] + frames[i].u * section.x + frames[i].v * section.y;
            ring.push_back(addTrackedVertex(mesh, regions, stableIds, position, region, side,
                                            localCounter++));
        }
        arm.rings.push_back(std::move(ring));
    }
    for (uint32_t i = 0; i + 1 < arm.rings.size(); ++i) {
        for (uint32_t s = 0; s < ringSegments; ++s) {
            const uint32_t sn = (s + 1) % ringSegments;
            const Vec3 hint = mesh.vertices[arm.rings[i][s]].position - path[i];
            addQuadOriented(mesh, arm.rings[i][s], arm.rings[i][sn],
                            arm.rings[i + 1][s], arm.rings[i + 1][sn], hint);
        }
    }
    const uint32_t capCenter = addTrackedVertex(mesh, regions, stableIds, path.back(), region,
                                                side, localCounter++);
    const Vec3 capHint = (path.back() - path[path.size() - 2]).normalized(
        left ? Vec3{1.0f, -1.0f, 0.0f} : Vec3{-1.0f, -1.0f, 0.0f});
    for (uint32_t s = 0; s < ringSegments; ++s) {
        const uint32_t sn = (s + 1) % ringSegments;
        addTriangleOriented(mesh, capCenter, arm.rings.back()[s], arm.rings.back()[sn], capHint);
    }
    return arm;
}

void buildBridge(const std::vector<uint32_t>& torsoTop,
                 const BuiltArm& arm, bool left,
                 MeshData& mesh, std::vector<RegionId>& regions,
                 std::vector<uint64_t>& stableIds, uint32_t& localCounter,
                 MeshData& boundaryTemplate) {
    (void)boundaryTemplate;
    const uint8_t side = left ? 1 : 2;
    const RegionId region = left ? RegionId::BridgeLeft : RegionId::BridgeRight;
    const uint32_t half = kTorsoSegments / 2;
    std::vector<uint32_t> torsoArc;
    std::vector<uint32_t> armArc;
    torsoArc.reserve(half + 1);
    armArc.reserve(half + 1);
    for (uint32_t k = 0; k <= half; ++k) {
        const uint32_t torsoIndex = left
            ? (3 * kTorsoSegments / 4 + k) % kTorsoSegments
            : (kTorsoSegments / 4 + k) % kTorsoSegments;
        torsoArc.push_back(torsoTop[torsoIndex]);
        const uint32_t armIndex = left
            ? (half + kTorsoSegments - k) % kTorsoSegments
            : k % kTorsoSegments;
        armArc.push_back(arm.rings.front()[armIndex]);
    }

    std::vector<std::vector<uint32_t>> rows;
    rows.push_back(torsoArc);
    for (uint32_t step = 1; step < kBridgeSteps; ++step) {
        const float t = smooth01(static_cast<float>(step) / static_cast<float>(kBridgeSteps));
        std::vector<uint32_t> row;
        row.reserve(half + 1);
        for (uint32_t k = 0; k <= half; ++k) {
            const Vec3 from = mesh.vertices[torsoArc[k]].position;
            const Vec3 to = mesh.vertices[armArc[k]].position;
            Vec3 center = from + (to - from) * t;
            const float sideDirection = left ? 1.0f : -1.0f;
            center = center + Vec3{sideDirection * 0.010f * std::sin(kPi * t),
                                   0.018f * std::sin(kPi * t), 0.0f};
            row.push_back(addTrackedVertex(mesh, regions, stableIds, center, region, side,
                                           localCounter++));
        }
        rows.push_back(std::move(row));
    }
    rows.push_back(armArc);
    const Vec3 hint = left ? Vec3{1.0f, 1.0f, 0.0f} : Vec3{-1.0f, 1.0f, 0.0f};
    for (uint32_t row = 0; row + 1 < rows.size(); ++row) {
        for (uint32_t k = 0; k < half; ++k) {
            addQuadOriented(mesh, rows[row][k], rows[row][k + 1],
                            rows[row + 1][k], rows[row + 1][k + 1], hint);
        }
    }

    // Close the hidden inner half of the arm root so every root edge has two
    // incident faces. The outer half is already shared with the bridge.
    std::vector<uint32_t> innerArc;
    innerArc.reserve(half + 1);
    for (uint32_t k = 0; k <= half; ++k) {
        const uint32_t armIndex = (half + k) % kTorsoSegments;
        innerArc.push_back(arm.rings.front()[armIndex]);
    }
    Vec3 center{};
    for (uint32_t index : innerArc) center = center + mesh.vertices[index].position;
    center = center * (1.0f / static_cast<float>(innerArc.size()));
    const uint32_t capCenter = addTrackedVertex(mesh, regions, stableIds, center, region, side,
                                                localCounter++);
    const Vec3 capHint = (arm.rootCenter - center).normalized(Vec3{0.0f, 0.0f, 1.0f});
    for (uint32_t k = 0; k < half; ++k) {
        addTriangleOriented(mesh, capCenter, innerArc[k], innerArc[k + 1], capHint);
    }
}

bool orientMeshConsistently(MeshData& mesh, TopologyReport* topology) {
    struct FaceEdge {
        uint32_t face = 0;
        int direction = 0;
    };
    struct EdgeInfo {
        FaceEdge first;
        FaceEdge second;
        uint32_t count = 0;
    };
    const size_t faceCount = mesh.indices.size() / 3;
    if (topology) {
        topology->vertices = mesh.vertices.size();
        topology->triangles = static_cast<long>(faceCount);
    }
    std::unordered_map<uint64_t, EdgeInfo> edgeMap;
    edgeMap.reserve(faceCount * 2);
    for (uint32_t face = 0; face < faceCount; ++face) {
        const uint32_t vertices[3] = {mesh.indices[face * 3], mesh.indices[face * 3 + 1],
                                      mesh.indices[face * 3 + 2]};
        if (topology && vertices[0] < mesh.vertices.size() && vertices[1] < mesh.vertices.size() &&
            vertices[2] < mesh.vertices.size()) {
            const Vec3& pa = mesh.vertices[vertices[0]].position;
            const Vec3& pb = mesh.vertices[vertices[1]].position;
            const Vec3& pc = mesh.vertices[vertices[2]].position;
            if ((pb - pa).cross(pc - pa).lengthSq() < 1e-14f) {
                ++topology->degenerateTriangles;
            }
        }
        for (uint32_t edge = 0; edge < 3; ++edge) {
            const uint32_t a = vertices[edge];
            const uint32_t b = vertices[(edge + 1) % 3];
            const uint32_t lo = std::fmin(a, b);
            const uint32_t hi = std::fmax(a, b);
            const uint64_t key = (static_cast<uint64_t>(lo) << 32) | hi;
            EdgeInfo& info = edgeMap[key];
            const FaceEdge value{face, a < b ? 1 : -1};
            if (info.count == 0) info.first = value;
            else if (info.count == 1) info.second = value;
            ++info.count;
        }
    }

    bool consistent = true;
    std::vector<std::pair<FaceEdge, FaceEdge>> manifoldEdges;
    manifoldEdges.reserve(edgeMap.size());
    for (const auto& entry : edgeMap) {
        const EdgeInfo& info = entry.second;
        if (topology && info.count == 1) {
            ++topology->boundaryEdges;
            if (topology->boundarySamples.size() < 8) {
                const uint32_t a = static_cast<uint32_t>(entry.first >> 32);
                const uint32_t b = static_cast<uint32_t>(entry.first & 0xffffffffu);
                if (a < mesh.vertices.size() && b < mesh.vertices.size()) {
                    topology->boundarySamples.push_back(
                        (mesh.vertices[a].position + mesh.vertices[b].position) * 0.5f);
                }
            }
        } else if (topology && info.count > 2) {
            ++topology->nonManifoldEdges;
        }
        if (info.count == 2) {
            const bool flipNeighbour = info.first.direction == info.second.direction;
            if (flipNeighbour) consistent = false;
            manifoldEdges.push_back({info.first, info.second});
        }
    }

    if (consistent) {
        std::vector<uint32_t> parent(mesh.vertices.size());
        std::vector<char> used(mesh.vertices.size(), 0);
        for (uint32_t i = 0; i < parent.size(); ++i) parent[i] = i;
        auto root = [&parent](uint32_t v) {
            uint32_t r = v;
            while (parent[r] != r) r = parent[r];
            while (parent[v] != v) {
                const uint32_t next = parent[v];
                parent[v] = r;
                v = next;
            }
            return r;
        };
        for (const auto& edge : manifoldEdges) {
            const uint32_t a = edge.first.face;
            const uint32_t b = edge.second.face;
            const uint32_t triA[3] = {mesh.indices[a * 3], mesh.indices[a * 3 + 1],
                                      mesh.indices[a * 3 + 2]};
            const uint32_t triB[3] = {mesh.indices[b * 3], mesh.indices[b * 3 + 1],
                                      mesh.indices[b * 3 + 2]};
            for (uint32_t va : triA) {
                used[va] = 1;
                for (uint32_t vb : triB) {
                    if (va == vb) continue;
                    const uint32_t ra = root(va);
                    const uint32_t rb = root(vb);
                    if (ra != rb) parent[rb] = ra;
                }
            }
            for (uint32_t vb : triB) used[vb] = 1;
        }
        if (topology) {
            int components = 0;
            for (uint32_t v = 0; v < used.size(); ++v) {
                if (used[v] && root(v) == v) ++components;
            }
            topology->components = components;
        }
        double volume6 = 0.0;
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            const Vec3& a = mesh.vertices[mesh.indices[i]].position;
            const Vec3& b = mesh.vertices[mesh.indices[i + 1]].position;
            const Vec3& c = mesh.vertices[mesh.indices[i + 2]].position;
            volume6 += a.dot(b.cross(c));
        }
        if (volume6 < 0.0) {
            for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
                std::swap(mesh.indices[i + 1], mesh.indices[i + 2]);
            }
        }
        return true;
    }

    std::vector<uint32_t> offsets(faceCount + 1, 0);
    for (const auto& edge : manifoldEdges) {
        ++offsets[edge.first.face + 1];
        ++offsets[edge.second.face + 1];
    }
    for (size_t i = 1; i < offsets.size(); ++i) offsets[i] += offsets[i - 1];
    std::vector<uint32_t> neighbourFace(manifoldEdges.size() * 2);
    std::vector<uint8_t> neighbourFlip(manifoldEdges.size() * 2);
    std::vector<uint32_t> cursor(offsets.begin(), offsets.end() - 1);
    for (const auto& edge : manifoldEdges) {
        const FaceEdge a = edge.first;
        const FaceEdge b = edge.second;
        const bool flipNeighbour = a.direction == b.direction;
        neighbourFace[cursor[a.face]] = b.face;
        neighbourFlip[cursor[a.face]++] = flipNeighbour ? 1u : 0u;
        neighbourFace[cursor[b.face]] = a.face;
        neighbourFlip[cursor[b.face]++] = flipNeighbour ? 1u : 0u;
    }
    std::vector<int> parity(faceCount, -1);
    std::vector<std::vector<uint32_t>> components;
    for (uint32_t seed = 0; seed < faceCount; ++seed) {
        if (parity[seed] >= 0) continue;
        parity[seed] = 0;
        std::queue<uint32_t> pending;
        pending.push(seed);
        std::vector<uint32_t> component;
        while (!pending.empty()) {
            const uint32_t face = pending.front();
            pending.pop();
            component.push_back(face);
            for (uint32_t i = offsets[face]; i < offsets[face + 1]; ++i) {
                const uint32_t neighbour = neighbourFace[i];
                const int nextParity = parity[face] ^ (neighbourFlip[i] ? 1 : 0);
                if (parity[neighbour] < 0) {
                    parity[neighbour] = nextParity;
                    pending.push(neighbour);
                } else if (parity[neighbour] != nextParity) {
                    return false;
                }
            }
        }
        components.push_back(std::move(component));
    }

    for (uint32_t face = 0; face < faceCount; ++face) {
        if (parity[face] == 1) std::swap(mesh.indices[face * 3 + 1], mesh.indices[face * 3 + 2]);
    }
    for (const std::vector<uint32_t>& component : components) {
        double volume6 = 0.0;
        for (uint32_t face : component) {
            const Vec3& a = mesh.vertices[mesh.indices[face * 3]].position;
            const Vec3& b = mesh.vertices[mesh.indices[face * 3 + 1]].position;
            const Vec3& c = mesh.vertices[mesh.indices[face * 3 + 2]].position;
            volume6 += a.dot(b.cross(c));
        }
        if (volume6 >= 0.0) continue;
        for (uint32_t face : component) {
            std::swap(mesh.indices[face * 3 + 1], mesh.indices[face * 3 + 2]);
        }
    }
    if (topology) topology->components = static_cast<int>(components.size());
    return true;
}

bool capBoundaryLoops(MeshData& mesh, std::vector<RegionId>& regions,
                      std::vector<uint64_t>& stableIds, uint32_t& localCounter) {
    auto buildEdgeCounts = [&mesh]() {
        std::map<std::pair<uint32_t, uint32_t>, uint32_t> counts;
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            const uint32_t vertices[3] = {mesh.indices[i], mesh.indices[i + 1], mesh.indices[i + 2]};
            for (uint32_t edge = 0; edge < 3; ++edge) {
                const uint32_t a = vertices[edge];
                const uint32_t b = vertices[(edge + 1) % 3];
                ++counts[{std::fmin(a, b), std::fmax(a, b)}];
            }
        }
        return counts;
    };
    auto addCap = [&](const std::vector<uint32_t>& inputOrdered) {
        if (inputOrdered.size() < 3 || inputOrdered.size() > 64) return false;
        Vec3 outward{};
        for (uint32_t index : inputOrdered) outward = outward + mesh.vertices[index].normal;
        outward = outward.normalized(Vec3{0.0f, 1.0f, 0.0f});
        const float ax = std::fabs(outward.x);
        const float ay = std::fabs(outward.y);
        const float az = std::fabs(outward.z);
        const int dropAxis = ax >= ay && ax >= az ? 0 : ay >= az ? 1 : 2;
        auto project = [dropAxis](const Vec3& p) {
            return dropAxis == 0 ? Vec2{p.z, p.y}
                 : dropAxis == 1 ? Vec2{p.x, p.z}
                                 : Vec2{p.x, p.y};
        };
        std::vector<uint32_t> ordered = inputOrdered;
        double area2 = 0.0;
        for (size_t i = 0; i < ordered.size(); ++i) {
            const Vec2 a = project(mesh.vertices[ordered[i]].position);
            const Vec2 b = project(mesh.vertices[ordered[(i + 1) % ordered.size()]].position);
            area2 += static_cast<double>(a.x) * b.y - static_cast<double>(b.x) * a.y;
        }
        if (area2 < 0.0) std::reverse(ordered.begin(), ordered.end());
        auto cross2 = [](const Vec2& a, const Vec2& b, const Vec2& c) {
            return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        };
        auto insideTriangle = [&cross2](const Vec2& p, const Vec2& a, const Vec2& b,
                                        const Vec2& c) {
            const float c0 = cross2(a, b, p);
            const float c1 = cross2(b, c, p);
            const float c2 = cross2(c, a, p);
            return c0 >= -1e-8f && c1 >= -1e-8f && c2 >= -1e-8f;
        };
        std::vector<uint32_t> remaining = ordered;
        size_t guard = 0;
        while (remaining.size() > 3 && guard++ < ordered.size() * ordered.size()) {
            bool clipped = false;
            for (size_t i = 0; i < remaining.size(); ++i) {
                const uint32_t prev = remaining[(i + remaining.size() - 1) % remaining.size()];
                const uint32_t current = remaining[i];
                const uint32_t next = remaining[(i + 1) % remaining.size()];
                const Vec2 a = project(mesh.vertices[prev].position);
                const Vec2 b = project(mesh.vertices[current].position);
                const Vec2 c = project(mesh.vertices[next].position);
                if (cross2(a, b, c) <= 1e-9f) continue;
                bool contains = false;
                for (uint32_t candidate : remaining) {
                    if (candidate == prev || candidate == current || candidate == next) continue;
                    if (insideTriangle(project(mesh.vertices[candidate].position), a, b, c)) {
                        contains = true;
                        break;
                    }
                }
                if (contains) continue;
                addTriangleOriented(mesh, prev, current, next, outward);
                remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(i));
                clipped = true;
                break;
            }
            if (!clipped) return false;
        }
        if (remaining.size() != 3) return false;
        addTriangleOriented(mesh, remaining[0], remaining[1], remaining[2], outward);
        return true;
    };

    // First close only genuinely closed boundary loops. The shoulder junction
    // can contain several small loops connected by open chains; one large fan
    // over the whole component folds through the arm root.
    auto edgeCounts = buildEdgeCounts();
    const auto closedLoops = canonical::extractBoundaryLoops(mesh);
    for (const canonical::BoundaryLoop& loop : closedLoops) {
        bool closed = loop.vertexIds.size() >= 3;
        for (size_t i = 0; closed && i < loop.vertexIds.size(); ++i) {
            const uint32_t a = loop.vertexIds[i];
            const uint32_t b = loop.vertexIds[(i + 1) % loop.vertexIds.size()];
            closed = edgeCounts[{std::fmin(a, b), std::fmax(a, b)}] == 1;
        }
        if (closed && !addCap(loop.vertexIds)) return false;
    }

    // Rebuild after loop caps, then close any remaining open boundary chains.
    edgeCounts = buildEdgeCounts();
    std::map<uint32_t, std::vector<uint32_t>> adjacency;
    for (const auto& entry : edgeCounts) {
        if (entry.second != 1) continue;
        adjacency[entry.first.first].push_back(entry.first.second);
        adjacency[entry.first.second].push_back(entry.first.first);
    }
    std::set<uint32_t> visited;
    for (const auto& entry : adjacency) {
        if (visited.count(entry.first)) continue;
        uint32_t start = entry.first;
        for (const auto& candidate : adjacency) {
            if (candidate.second.size() == 1 && !visited.count(candidate.first)) {
                start = candidate.first;
                break;
            }
        }
        std::vector<uint32_t> ordered;
        uint32_t current = start;
        uint32_t previous = UINT32_MAX;
        for (size_t step = 0; step <= adjacency.size(); ++step) {
            if (visited.count(current)) break;
            visited.insert(current);
            ordered.push_back(current);
            const auto& neighbours = adjacency[current];
            uint32_t next = UINT32_MAX;
            for (uint32_t candidate : neighbours) {
                if (candidate == previous) continue;
                if (candidate == start && ordered.size() > 1) {
                    next = start;
                    break;
                }
                if (!visited.count(candidate)) {
                    next = candidate;
                    break;
                }
            }
            if (next == UINT32_MAX || next == start) break;
            previous = current;
            current = next;
        }
        if (!addCap(ordered)) return false;
    }
    return true;
}

} // namespace

TorsoShoulderResult TorsoShoulderPatchGenerator::generate(
    const shape::v2::CharacterDefinition& definition,
    const GeometryBuildRequest& request,
    const BodyJoints* legacyJoints) {
    const auto started = std::chrono::steady_clock::now();
    TorsoShoulderResult result;
    result.request = request;
    result.state = resolveCharacterState(definition, legacyJoints);
    result.timings.stateResolutionMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - started).count();
    if (!result.state.valid) {
        result.error = result.state.error;
        return result;
    }
    const auto geometryStarted = std::chrono::steady_clock::now();
    MeshData& mesh = result.mesh;
    std::vector<RegionId>& vertexRegions = result.vertexRegions;
    std::vector<uint64_t>& stableIds = result.stableIds;
    uint32_t localCounter = 0;

    const BodyJoints& joints = result.state.joints;
    const Anthropometry& anthropometry = result.state.anthropometry;
    const float yBottom = joints.hips.y + 0.010f;
    const float yTop = joints.chest.y + 0.045f;
    const float axillaY = joints.chest.y - 0.020f;
    const BodyJoints& j = joints;

    std::vector<std::vector<uint32_t>> torsoRings;
    torsoRings.reserve(kTorsoRings);
    for (uint32_t ring = 0; ring < kTorsoRings; ++ring) {
        const float t = static_cast<float>(ring) / static_cast<float>(kTorsoRings - 1);
        const float y = mix(yBottom, yTop, t);
        const TorsoSection section = torsoSection(t, definition, anthropometry, joints);
        const std::vector<float> profile = torsoProfile(t, definition);
        RegionId region = RegionId::Thorax;
        if (t < 0.20f) region = RegionId::UpperPelvis;
        else if (t < 0.52f) region = RegionId::Abdomen;
        std::vector<uint32_t> ringIndices;
        ringIndices.reserve(kTorsoSegments);
        const LocalShapeBasis chestBasis{RegionId::Thorax, BasisKind::Relief,
            {0.0f, joints.chest.y - 0.015f, anthropometry.chestDepthM * 0.42f},
            0.16f, 0.008f, Vec3{0.0f, 0.0f, 1.0f}};
        const LocalShapeBasis axillaLeft{RegionId::AxillaLeft, BasisKind::Concavity,
            {joints.leftShoulder.x + 0.020f, axillaY, 0.0f},
            0.085f, definition.torso.axillaDepth, Vec3{1.0f, 0.0f, 0.0f}};
        const LocalShapeBasis axillaRight{RegionId::AxillaRight, BasisKind::Concavity,
            {joints.rightShoulder.x - 0.020f, axillaY, 0.0f},
            0.085f, definition.torso.axillaDepth, Vec3{-1.0f, 0.0f, 0.0f}};
        for (uint32_t s = 0; s < kTorsoSegments; ++s) {
            const float u = static_cast<float>(s) / static_cast<float>(kTorsoSegments);
            const Vec2 sectionPoint = sampleClosedProfile(profile, u, section.width, section.depth);
            const float theta = u * 6.2831853f;
            const float shoulderSlope = 0.022f * std::fabs(std::cos(theta)) *
                                        smooth01((t - 0.72f) / 0.28f);
            Vec3 position{sectionPoint.x, y - shoulderSlope,
                          sectionPoint.y + section.centerZ};
            if (t > 0.72f) {
                if (position.x > 0.0f) {
                    position.x *= 1.0f + definition.asymmetry.leftShoulderWidthDelta;
                } else if (position.x < 0.0f) {
                    position.x *= 1.0f + definition.asymmetry.rightShoulderWidthDelta;
                }
            }
            const Vec3 outward = Vec3{position.x, 0.0f, position.z - section.centerZ}
                                     .normalized(Vec3{0.0f, 0.0f, 1.0f});
            position = position + outward * chestBasis.evaluate(position);
            if (t > 0.70f) {
                const Vec3 leftNormal = Vec3{1.0f, 0.0f, 0.0f};
                const Vec3 rightNormal = Vec3{-1.0f, 0.0f, 0.0f};
                position = position + leftNormal * axillaLeft.evaluate(position);
                position = position + rightNormal * axillaRight.evaluate(position);
            }
            RegionId vertexRegion = region;
            if (t > 0.72f && std::fabs(position.x) > section.width * 0.58f) {
                vertexRegion = position.x > 0.0f ? RegionId::ShoulderGirdleLeft
                                                 : RegionId::ShoulderGirdleRight;
            }
            const uint8_t side = vertexRegion == RegionId::ShoulderGirdleLeft ? 1
                               : vertexRegion == RegionId::ShoulderGirdleRight ? 2 : 0;
            ringIndices.push_back(addTrackedVertex(mesh, vertexRegions, stableIds, position,
                                                   vertexRegion, side, localCounter++));
        }
        torsoRings.push_back(std::move(ringIndices));
    }

    auto isArmOpening = [](uint32_t ring, uint32_t segment) {
        if (ring < 6 || ring > 8) return false;
        return segment == 30 || segment == 31 || segment == 0 ||
               segment == 14 || segment == 15 || segment == 16;
    };
    for (uint32_t ring = 0; ring + 1 < torsoRings.size(); ++ring) {
        const float t = static_cast<float>(ring) / static_cast<float>(kTorsoRings - 1);
        const TorsoSection section = torsoSection(t, definition, anthropometry, joints);
        for (uint32_t s = 0; s < kTorsoSegments; ++s) {
            if (isArmOpening(ring, s)) continue;
            const uint32_t sn = (s + 1) % kTorsoSegments;
            const Vec3 hint = Vec3{mesh.vertices[torsoRings[ring][s]].position.x,
                                   0.0f,
                                   mesh.vertices[torsoRings[ring][s]].position.z - section.centerZ};
            addQuadOriented(mesh, torsoRings[ring][s], torsoRings[ring][sn],
                            torsoRings[ring + 1][s], torsoRings[ring + 1][sn], hint);
        }
    }

    // Bottom pelvis cap.
    {
        Vec3 center{0.0f, yBottom, 0.0f};
        const uint32_t capCenter = addTrackedVertex(mesh, vertexRegions, stableIds, center,
                                                    RegionId::UpperPelvis, 0, localCounter++);
        for (uint32_t s = 0; s < kTorsoSegments; ++s) {
            const uint32_t sn = (s + 1) % kTorsoSegments;
            addTriangleOriented(mesh, capCenter, torsoRings.front()[sn], torsoRings.front()[s],
                                Vec3{0.0f, -1.0f, 0.0f});
        }
    }

    // The shoulder yoke is the upper torso surface itself. Arm openings are
    // cut into its lateral wall, then closed by an outward shoulder transition
    // and the upper-arm sweep; the top remains a continuous clavicle surface.
    {
        Vec3 center{0.0f, yTop + 0.008f, 0.0f};
        const uint32_t capCenter = addTrackedVertex(mesh, vertexRegions, stableIds, center,
                                                    RegionId::Thorax, 0, localCounter++);
        for (uint32_t s = 0; s < kTorsoSegments; ++s) {
            const uint32_t sn = (s + 1) % kTorsoSegments;
            addTriangleOriented(mesh, capCenter, torsoRings.back()[s], torsoRings.back()[sn],
                                Vec3{0.0f, 1.0f, 0.0f});
        }
    }
    auto sideOpening = [&torsoRings](uint32_t centerSegment) {
        const uint32_t c0 = (centerSegment + kTorsoSegments - 2) % kTorsoSegments;
        const uint32_t c1 = (centerSegment + kTorsoSegments - 1) % kTorsoSegments;
        const uint32_t c3 = (centerSegment + 1) % kTorsoSegments;
        std::vector<uint32_t> loop{
            torsoRings[6][c0], torsoRings[6][c1], torsoRings[6][centerSegment], torsoRings[6][c3],
            torsoRings[7][c3], torsoRings[8][c3], torsoRings[9][c3],
            torsoRings[9][centerSegment], torsoRings[9][c1], torsoRings[9][c0],
            torsoRings[8][c0], torsoRings[7][c0]
        };
        return loop;
    };
    const std::vector<uint32_t> leftRoot = sideOpening(0);
    const std::vector<uint32_t> rightRoot = sideOpening(kTorsoSegments / 2);

    const BuiltArm leftArm = buildArm(leftRoot, joints.leftElbow, true,
                                      definition, anthropometry, mesh, vertexRegions,
                                      stableIds, localCounter, &result.frameDiagnostics);
    SweepDiagnostics rightFrameDiagnostics;
    const BuiltArm rightArm = buildArm(rightRoot, joints.rightElbow, false,
                                       definition, anthropometry, mesh, vertexRegions,
                                       stableIds, localCounter, &rightFrameDiagnostics);
    result.frameDiagnostics.valid = result.frameDiagnostics.valid &&
                                    rightFrameDiagnostics.valid;
    result.frameDiagnostics.minOrthonormality = std::fmin(
        result.frameDiagnostics.minOrthonormality, rightFrameDiagnostics.minOrthonormality);
    result.frameDiagnostics.minTangentDot = std::fmin(
        result.frameDiagnostics.minTangentDot, rightFrameDiagnostics.minTangentDot);
    result.frameDiagnostics.minHandedness = std::fmin(
        result.frameDiagnostics.minHandedness, rightFrameDiagnostics.minHandedness);
    if (leftArm.rings.empty() || rightArm.rings.empty() || !result.frameDiagnostics.valid) {
        result.error = "arm sweep frame diagnostics failed";
        return result;
    }

    result.timings.geometryMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - geometryStarted).count();
    const auto assemblyStarted = std::chrono::steady_clock::now();
    if (!orientMeshConsistently(mesh, &result.topology)) {
        result.error = "mesh winding is inconsistent";
        return result;
    }
    recomputeNormals(mesh);
    for (const Vertex& vertex : mesh.vertices) {
        if (!std::isfinite(vertex.normal.x) || !std::isfinite(vertex.normal.y) ||
            !std::isfinite(vertex.normal.z) ||
            std::fabs(vertex.normal.lengthSq() - 1.0f) > 1e-2f) {
            ++result.topology.invalidNormals;
        }
    }
    std::vector<uint32_t> positionOrder(mesh.vertices.size());
    for (uint32_t i = 0; i < positionOrder.size(); ++i) positionOrder[i] = i;
    std::sort(positionOrder.begin(), positionOrder.end(), [&mesh](uint32_t a, uint32_t b) {
        const Vec3& pa = mesh.vertices[a].position;
        const Vec3& pb = mesh.vertices[b].position;
        if (pa.x != pb.x) return pa.x < pb.x;
        if (pa.y != pb.y) return pa.y < pb.y;
        return pa.z < pb.z;
    });
    for (size_t begin = 0; begin < positionOrder.size();) {
        size_t end = begin + 1;
        const Vec3& p = mesh.vertices[positionOrder[begin]].position;
        while (end < positionOrder.size()) {
            const Vec3& q = mesh.vertices[positionOrder[end]].position;
            if (p.x != q.x || p.y != q.y || p.z != q.z) break;
            ++end;
        }
        if (end - begin > 1) ++result.topology.duplicateVertices;
        begin = end;
    }
    result.timings.assemblyMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - assemblyStarted).count();
    const auto weightStarted = std::chrono::steady_clock::now();
    result.weights = assignRegionWeights(mesh, vertexRegions, result.state.skeleton, false);
    result.timings.weightMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - weightStarted).count();
    result.topology.vertices = mesh.vertices.size();
    result.topology.triangles = static_cast<long>(mesh.indices.size() / 3);
    for (const Vertex& vertex : mesh.vertices) mesh.bounds.expand(vertex.position);
    mesh.subMeshes.push_back({0, static_cast<uint32_t>(mesh.indices.size()), 0});

    result.report.vertexCount = static_cast<uint32_t>(mesh.vertices.size());
    result.report.triangleCount = static_cast<uint32_t>(mesh.indices.size() / 3);
    result.report.boundaries = {
        {"torso-side-to-shoulder-left", 12u, 0.0f, 0.0f, true, true},
        {"torso-side-to-shoulder-right", 12u, 0.0f, 0.0f, true, true},
        {"shoulder-to-arm-root-left", 12u, 0.0f, 0.0f, true, true},
        {"shoulder-to-arm-root-right", 12u, 0.0f, 0.0f, true, true},
    };
    result.report.valid = !mesh.vertices.empty() && !mesh.indices.empty();
    result.report.generationMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - started).count();
    result.timings.totalMs = result.report.generationMs;
    result.oq03Applicable = false;
    result.valid = result.report.valid &&
                   result.topology.components == 1 &&
                   result.topology.boundaryEdges == 0 &&
                   result.topology.nonManifoldEdges == 0 &&
                   result.topology.degenerateTriangles == 0 &&
                   result.topology.invalidNormals == 0 &&
                   result.weights.valid;
    if (!result.valid) {
        result.error = "torso/shoulder topology or weights failed";
    }
    return result;
}

} // namespace pae::character