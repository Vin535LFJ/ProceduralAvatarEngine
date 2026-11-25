#include "mini_test.hpp"

#include "procedural/MeshTopology.h"
#include "procedural/character/AnatomicalLandmarks.h"
#include "procedural/humanoid/canonical/CanonicalSkinGenerator.h"
#include "procedural/humanoid/canonical/CanonicalTopology.h"
#include "procedural/character/BodyPatchGenerator.h"
#include "procedural/character/GeometryBuildRequest.h"
#include "procedural/character/LocalShapeBasis.h"
#include "procedural/character/LodCorrespondence.h"
#include "procedural/character/RegionWeightGenerator.h"
#include "procedural/character/ResolvedCharacterState.h"
#include "procedural/character/TorsoShoulderPatchGenerator.h"
#include "procedural/character/VariableProfileSweep.h"
#include "procedural/shape/v2/CharacterDefinition.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <map>
#include <set>
#include <tuple>
#include <unordered_set>

using namespace pae;
using namespace pae::character;

namespace {

shape::v2::CharacterDefinition makeDefinition() {
    return shape::v2::defaultCharacterDefinition();
}

GeometryBuildRequest makeRequest(const shape::v2::CharacterDefinition& definition) {
    return makeGeometryBuildRequest(definition, 1, 2);
}

TorsoShoulderResult generate(const shape::v2::CharacterDefinition& definition) {
    return TorsoShoulderPatchGenerator::generate(definition, makeRequest(definition));
}

float weightForBone(const Vertex& vertex, uint16_t bone) {
    const uint16_t joints[4] = {vertex.joints.x, vertex.joints.y,
                                vertex.joints.z, vertex.joints.w};
    const float weights[4] = {vertex.weights.x, vertex.weights.y,
                              vertex.weights.z, vertex.weights.w};
    float result = 0.0f;
    for (size_t i = 0; i < 4; ++i) {
        if (joints[i] == bone) result += weights[i];
    }
    return result;
}

bool hasOnlyBones(const Vertex& vertex, bool left) {
    const uint16_t joints[4] = {vertex.joints.x, vertex.joints.y,
                                vertex.joints.z, vertex.joints.w};
    for (uint16_t bone : joints) {
        if (left && (bone == BoneRightShoulder || bone == BoneRightUpperArm ||
                     bone == BoneRightLowerArm || bone == BoneRightHand)) return false;
        if (!left && (bone == BoneLeftShoulder || bone == BoneLeftUpperArm ||
                      bone == BoneLeftLowerArm || bone == BoneLeftHand)) return false;
    }
    return true;
}

} // namespace

PAE_TEST_CASE(vr_data_layer_resolves_single_authority) {
    shape::v2::CharacterDefinition definition = makeDefinition();
    definition.landmarks.placed.push_back({"chest-center", {0.0f, 2.0f, 0.0f}});
    const ResolvedCharacterState state = resolveCharacterState(definition);
    EXPECT_TRUE(state.valid);
    EXPECT_TRUE(state.skeleton.bones.size() == BoneCount);
    EXPECT_TRUE(state.landmarks.points.size() == static_cast<size_t>(LandmarkId::Count));
    const LandmarkPoint* chest = state.landmarks.find(LandmarkId::ChestCenter);
    EXPECT_TRUE(chest != nullptr);
    EXPECT_NEAR(chest->position.y, 2.0f, 1e-6f);
    EXPECT_TRUE(chest->confidence == shape::v2::ValueConfidence::Measured);
    EXPECT_TRUE(state.landmarkReport.usedLegacyJoints == false);
}

PAE_TEST_CASE(vr_bilateral_symmetric_and_asymmetric) {
    shape::v2::CharacterDefinition symmetric = makeDefinition();
    const TorsoShoulderResult symmetricResult = generate(symmetric);
    EXPECT_TRUE(symmetricResult.valid);
    EXPECT_TRUE(symmetricResult.frameDiagnostics.valid);
    EXPECT_TRUE(std::fabs(symmetricResult.mesh.bounds.max.x +
                          symmetricResult.mesh.bounds.min.x) < 0.012f);

    shape::v2::CharacterDefinition asymmetric = makeDefinition();
    asymmetric.asymmetry.leftShoulderWidthDelta = 0.08f;
    asymmetric.asymmetry.rightShoulderWidthDelta = -0.05f;
    const TorsoShoulderResult asymmetricResult = generate(asymmetric);
    EXPECT_TRUE(asymmetricResult.valid);
    float leftShoulderMax = 0.0f;
    float rightShoulderMax = 0.0f;
    for (size_t i = 0; i < asymmetricResult.mesh.vertices.size(); ++i) {
        const Vertex& vertex = asymmetricResult.mesh.vertices[i];
        const RegionId region = asymmetricResult.vertexRegions[i];
        if (vertex.position.y < 1.34f || region == RegionId::UpperArmRootLeft ||
            region == RegionId::UpperArmRootRight || region == RegionId::BridgeLeft ||
            region == RegionId::BridgeRight) continue;
        if (vertex.position.x > 0.0f) leftShoulderMax = std::fmax(leftShoulderMax, vertex.position.x);
        if (vertex.position.x < 0.0f) rightShoulderMax = std::fmax(rightShoulderMax, -vertex.position.x);
    }
    std::fprintf(stderr, "VR1A-ASYM left=%.4f right=%.4f yTop=%.4f\n",
                 leftShoulderMax, rightShoulderMax, asymmetricResult.mesh.bounds.max.y);
    EXPECT_TRUE(leftShoulderMax > rightShoulderMax + 0.01f);
}

PAE_TEST_CASE(vr_parameter_effects_are_local_and_measurable) {
    shape::v2::CharacterDefinition base = makeDefinition();
    const TorsoShoulderResult baseline = generate(base);

    shape::v2::CharacterDefinition deep = makeDefinition();
    deep.torso.chestDepthScale = 1.18f;
    const TorsoShoulderResult deeper = generate(deep);
    EXPECT_TRUE(deeper.valid);
    EXPECT_TRUE(deeper.mesh.bounds.max.z - deeper.mesh.bounds.min.z >
                baseline.mesh.bounds.max.z - baseline.mesh.bounds.min.z + 0.005f);

    shape::v2::CharacterDefinition wide = makeDefinition();
    wide.torso.ribcageWidthScale = 1.15f;
    const TorsoShoulderResult wider = generate(wide);
    EXPECT_TRUE(wider.valid);
    float baselineTorsoWidth = 0.0f;
    float widerTorsoWidth = 0.0f;
    for (size_t i = 0; i < baseline.mesh.vertices.size(); ++i) {
        const Vertex& vertex = baseline.mesh.vertices[i];
        const RegionId region = baseline.vertexRegions[i];
        if (vertex.position.y <= 1.0f || vertex.position.y >= 1.30f ||
            region == RegionId::UpperArmRootLeft || region == RegionId::UpperArmRootRight ||
            region == RegionId::BridgeLeft || region == RegionId::BridgeRight) continue;
        baselineTorsoWidth = std::fmax(baselineTorsoWidth, std::fabs(vertex.position.x));
    }
    for (size_t i = 0; i < wider.mesh.vertices.size(); ++i) {
        const Vertex& vertex = wider.mesh.vertices[i];
        const RegionId region = wider.vertexRegions[i];
        if (vertex.position.y <= 1.0f || vertex.position.y >= 1.30f ||
            region == RegionId::UpperArmRootLeft || region == RegionId::UpperArmRootRight ||
            region == RegionId::BridgeLeft || region == RegionId::BridgeRight) continue;
        widerTorsoWidth = std::fmax(widerTorsoWidth, std::fabs(vertex.position.x));
    }
    std::fprintf(stderr, "VR1A-WIDTH base=%.4f wide=%.4f boundsY=[%.4f,%.4f]\n",
                 baselineTorsoWidth, widerTorsoWidth,
                 baseline.mesh.bounds.min.y, baseline.mesh.bounds.max.y);
    EXPECT_TRUE(widerTorsoWidth > baselineTorsoWidth + 0.005f);
}

PAE_TEST_CASE(vr_axilla_concavity_and_local_basis) {
    shape::v2::CharacterDefinition base = makeDefinition();
    base.torso.axillaDepth = 0.0f;
    const TorsoShoulderResult without = generate(base);
    shape::v2::CharacterDefinition carved = makeDefinition();
    carved.torso.axillaDepth = 0.050f;
    const TorsoShoulderResult with = generate(carved);
    EXPECT_TRUE(without.valid && with.valid);
    EXPECT_TRUE(without.mesh.vertices.size() == with.mesh.vertices.size());
    double sumWithout = 0.0;
    double sumWith = 0.0;
    size_t samples = 0;
    for (size_t i = 0; i < without.mesh.vertices.size(); ++i) {
        const Vec3& p = without.mesh.vertices[i].position;
        if (p.x > 0.10f && p.y > 1.20f && p.y < 1.36f) {
            sumWithout += std::fabs(p.x);
            sumWith += std::fabs(with.mesh.vertices[i].position.x);
            ++samples;
        }
    }
    EXPECT_TRUE(samples > 10u);
    EXPECT_TRUE(sumWith < sumWithout);

    LocalShapeBasis relief{RegionId::Thorax, BasisKind::Relief, {0, 0, 0}, 0.1f, 0.02f};
    LocalShapeBasis concavity{RegionId::Thorax, BasisKind::Concavity, {0, 0, 0}, 0.1f, 0.02f};
    EXPECT_NEAR(relief.evaluate({0, 0, 0}), 0.02f, 1e-6f);
    EXPECT_NEAR(concavity.evaluate({0, 0, 0}), -0.02f, 1e-6f);
    EXPECT_TRUE(relief.evaluate({0.2f, 0, 0}) == 0.0f);
}

PAE_TEST_CASE(vr_rmf_and_closed_profile_are_stable) {
    std::vector<Vec3> path = {
        {0.2f, 1.3f, 0.0f}, {0.28f, 1.2f, 0.01f}, {0.36f, 1.1f, 0.0f}
    };
    SweepDiagnostics diagnostics;
    const auto frames = computeRmfFrames(path, {0, 0, 1}, &diagnostics);
    EXPECT_TRUE(frames.size() == path.size());
    EXPECT_TRUE(diagnostics.valid);
    EXPECT_TRUE(diagnostics.tangentReversals == 0);
    for (const SweepFrame& frame : frames) {
        EXPECT_NEAR(frame.u.lengthSq(), 1.0f, 1e-4f);
        EXPECT_NEAR(frame.v.lengthSq(), 1.0f, 1e-4f);
        EXPECT_NEAR(frame.tangent.lengthSq(), 1.0f, 1e-4f);
        EXPECT_NEAR(frame.u.dot(frame.v), 0.0f, 1e-4f);
        EXPECT_TRUE(frame.u.cross(frame.v).dot(frame.tangent) > 0.99f);
    }
    const std::vector<float> controls = {1, 1.1f, 1, 0.9f, 1, 1.05f};
    EXPECT_NEAR(periodicCatmullRom(controls, 0.0f), 1.0f, 1e-5f);
    EXPECT_NEAR(periodicCatmullRom(controls, 1.0f), 1.0f, 1e-5f);
}

PAE_TEST_CASE(vr_topology_shared_boundaries_and_stable_ids) {
    const TorsoShoulderResult result = generate(makeDefinition());
    std::fprintf(stderr,
                 "VR1A-TOPO verts=%zu tris=%zu comps=%d boundary=%d nonmanifold=%d degen=%d badnorm=%d weights=%d err=%s\n",
                 result.mesh.vertices.size(), result.mesh.indices.size() / 3,
                 result.topology.components, result.topology.boundaryEdges,
                 result.topology.nonManifoldEdges, result.topology.degenerateTriangles,
                 result.topology.invalidNormals, result.weights.valid ? 1 : 0,
                 result.error.c_str());
    for (size_t i = 0; i < result.topology.boundarySamples.size() && i < 12; ++i) {
        const Vec3& p = result.topology.boundarySamples[i];
        std::fprintf(stderr, "VR1A-BOUNDARY at=(%.4f,%.4f,%.4f)\n", p.x, p.y, p.z);
    }
    const auto boundaryLoops = canonical::extractBoundaryLoops(result.mesh);
    std::fprintf(stderr, "VR1A-LOOPS count=%zu\n", boundaryLoops.size());
    for (size_t i = 0; i < boundaryLoops.size(); ++i) {
        std::fprintf(stderr, "VR1A-LOOP index=%zu verts=%zu\n", i,
                     boundaryLoops[i].vertexIds.size());
    }
    std::map<std::pair<uint32_t, uint32_t>, std::pair<uint32_t, uint32_t>> directedEdges;
    for (size_t i = 0; i + 2 < result.mesh.indices.size(); i += 3) {
        const uint32_t tri[3] = {result.mesh.indices[i], result.mesh.indices[i + 1],
                                 result.mesh.indices[i + 2]};
        for (uint32_t edge = 0; edge < 3; ++edge) {
            const uint32_t a = tri[edge];
            const uint32_t b = tri[(edge + 1) % 3];
            auto& counts = directedEdges[{std::fmin(a, b), std::fmax(a, b)}];
            if (a < b) ++counts.first;
            else ++counts.second;
        }
    }
    size_t sameDirectionEdges = 0;
    size_t oppositeDirectionEdges = 0;
    for (const auto& entry : directedEdges) {
        if (entry.second.first == 2 || entry.second.second == 2) ++sameDirectionEdges;
        if (entry.second.first == 1 && entry.second.second == 1) ++oppositeDirectionEdges;
    }
    std::fprintf(stderr, "VR1A-WINDING same_direction_edges=%zu opposite_direction_edges=%zu\n",
                 sameDirectionEdges, oppositeDirectionEdges);
    EXPECT_TRUE(sameDirectionEdges == 0);
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.topology.components == 1);
    EXPECT_TRUE(result.topology.boundaryEdges == 0);
    EXPECT_TRUE(result.topology.nonManifoldEdges == 0);
    EXPECT_TRUE(result.topology.degenerateTriangles == 0);
    EXPECT_TRUE(result.topology.invalidNormals == 0);
    EXPECT_TRUE(result.stableIds.size() == result.mesh.vertices.size());
    std::unordered_set<uint64_t> unique(result.stableIds.begin(), result.stableIds.end());
    EXPECT_TRUE(unique.size() == result.stableIds.size());
    EXPECT_TRUE(result.report.boundaries.size() == 4u);
    EXPECT_TRUE(result.oq03Applicable == false);

    const TorsoShoulderResult again = generate(makeDefinition());
    EXPECT_TRUE(again.stableIds == result.stableIds);
}

PAE_TEST_CASE(vr_lod_maps_and_cache_keys_are_explicit) {
    const TorsoShoulderResult result = generate(makeDefinition());
    const LodMap identity = buildLodMap(result.stableIds, result.stableIds, 0, 1);
    EXPECT_TRUE(identity.valid);
    EXPECT_TRUE(identity.identity);

    std::vector<uint64_t> changed = result.stableIds;
    changed.front() ^= 0xffffffffffffffffull;
    const LodMap mismatch = buildLodMap(result.stableIds, changed, 1, 2);
    EXPECT_TRUE(!mismatch.identity);

    const shape::v2::CharacterDefinition definition = makeDefinition();
    const GeometryBuildRequest a = makeGeometryBuildRequest(definition, 0, 2);
    const GeometryBuildRequest b = makeGeometryBuildRequest(definition, 2, 2);
    EXPECT_TRUE(a.cacheKey() != b.cacheKey());
    EXPECT_TRUE(makeGeometryBuildRequest(definition, 1, 2).cacheKey() ==
                makeGeometryBuildRequest(definition, 1, 2).cacheKey());
}

PAE_TEST_CASE(vr_weights_are_region_isolated_and_heatmap_is_real) {
    const TorsoShoulderResult result = generate(makeDefinition());
    EXPECT_TRUE(result.weights.valid);
    size_t leftShoulderUpperArmBlend = 0;
    for (size_t i = 0; i < result.mesh.vertices.size(); ++i) {
        const Vertex& vertex = result.mesh.vertices[i];
        const float sum = vertex.weights.x + vertex.weights.y +
                          vertex.weights.z + vertex.weights.w;
        EXPECT_TRUE(std::fabs(sum - 1.0f) < 1e-3f);
        const uint16_t joints[4] = {vertex.joints.x, vertex.joints.y,
                                    vertex.joints.z, vertex.joints.w};
        for (uint16_t bone : joints) EXPECT_TRUE(bone < BoneCount);
        const RegionId region = result.vertexRegions[i];
        if (region == RegionId::UpperArmRootLeft || region == RegionId::BridgeLeft) {
            EXPECT_TRUE(hasOnlyBones(vertex, true));
        }
        if (region == RegionId::UpperArmRootRight || region == RegionId::BridgeRight) {
            EXPECT_TRUE(hasOnlyBones(vertex, false));
        }
        if (weightForBone(vertex, BoneLeftShoulder) > 0.05f &&
            weightForBone(vertex, BoneLeftUpperArm) > 0.05f) {
            ++leftShoulderUpperArmBlend;
        }
    }
    EXPECT_TRUE(leftShoulderUpperArmBlend > 10u);

    MeshData heatmap = result.mesh;
    const RegionWeightReport heatReport = assignRegionWeights(
        heatmap, result.vertexRegions, result.state.skeleton, true);
    EXPECT_TRUE(heatReport.valid);
    size_t colored = 0;
    for (size_t i = 0; i < heatmap.vertices.size(); ++i) {
        EXPECT_TRUE(heatmap.vertices[i].joints.x == result.mesh.vertices[i].joints.x);
        if (heatmap.vertices[i].color.x != heatmap.vertices[i].color.y ||
            heatmap.vertices[i].color.y != heatmap.vertices[i].color.z) {
            ++colored;
        }
    }
    EXPECT_TRUE(colored > 100u);
}

PAE_TEST_CASE(vr_generation_is_measurable_and_old_patch_survives) {
    const auto started = std::chrono::steady_clock::now();
    const TorsoShoulderResult result = generate(makeDefinition());
    const double elapsed = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - started).count();
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(elapsed > 0.0);
    EXPECT_TRUE(result.mesh.vertices.size() > 500u);
    std::fprintf(stderr,
                 "VR1A-TIMING total=%.3f state=%.3f geometry=%.3f assembly=%.3f weight=%.3f\n",
                 result.timings.totalMs, result.timings.stateResolutionMs,
                 result.timings.geometryMs, result.timings.assemblyMs, result.timings.weightMs);

    const ResolvedCharacterState state = resolveCharacterState(makeDefinition());
    pae::CharacterDefinition legacy;
    MeshData legacyMesh;
    PatchGenerationReport legacyReport;
    EXPECT_TRUE(BodyPatchGenerator::generate(legacy, state.joints, state.skeleton,
                                             legacyMesh, legacyReport));
    EXPECT_TRUE(legacyReport.valid);
    EXPECT_TRUE(!legacyMesh.vertices.empty());
}

PAE_TEST_CASE(vr_geometry_diagnostics_explain_visual_continuity) {
    const TorsoShoulderResult result = generate(makeDefinition());
    struct EdgeRecord {
        uint32_t count = 0;
        uint32_t faceA = 0;
        uint32_t faceB = 0;
    };
    std::map<std::pair<uint32_t, uint32_t>, EdgeRecord> edges;
    std::map<std::array<uint32_t, 3>, uint32_t> duplicateFaces;
    std::vector<Vec3> faceNormals;
    size_t sliverTriangles = 0;
    for (uint32_t face = 0; face < result.mesh.indices.size() / 3; ++face) {
        const uint32_t ids[3] = {result.mesh.indices[face * 3], result.mesh.indices[face * 3 + 1],
                                 result.mesh.indices[face * 3 + 2]};
        std::array<uint32_t, 3> sorted = {ids[0], ids[1], ids[2]};
        std::sort(sorted.begin(), sorted.end());
        ++duplicateFaces[sorted];
        const Vec3& a = result.mesh.vertices[ids[0]].position;
        const Vec3& b = result.mesh.vertices[ids[1]].position;
        const Vec3& c = result.mesh.vertices[ids[2]].position;
        const Vec3 normal = (b - a).cross(c - a);
        faceNormals.push_back(normal.normalized());
        const float lengths[3] = {(b - a).length(), (c - b).length(), (a - c).length()};
        const float minLength = std::fmin(lengths[0], std::fmin(lengths[1], lengths[2]));
        const float maxLength = std::fmax(lengths[0], std::fmax(lengths[1], lengths[2]));
        if (minLength <= 1e-7f || maxLength / minLength > 10.0f) ++sliverTriangles;
        for (uint32_t edge = 0; edge < 3; ++edge) {
            const uint32_t x = ids[edge];
            const uint32_t y = ids[(edge + 1) % 3];
            EdgeRecord& record = edges[{std::fmin(x, y), std::fmax(x, y)}];
            if (record.count == 0) record.faceA = face;
            else record.faceB = face;
            ++record.count;
        }
    }
    size_t edgeIncidence[4] = {0, 0, 0, 0};
    std::vector<float> seamAngles;
    for (const auto& entry : edges) {
        const EdgeRecord& record = entry.second;
        edgeIncidence[record.count < 3 ? record.count : 3]++;
        if (record.count == 2) {
            const float dot = std::clamp(faceNormals[record.faceA].dot(faceNormals[record.faceB]),
                                         -1.0f, 1.0f);
            seamAngles.push_back(std::acos(dot) * 180.0f / 3.14159265f);
        }
    }
    std::sort(seamAngles.begin(), seamAngles.end());
    const float maxSeamAngle = seamAngles.empty() ? 0.0f : seamAngles.back();
    const float p95SeamAngle = seamAngles.empty() ? 0.0f :
        seamAngles[std::min(seamAngles.size() - 1,
                            static_cast<size_t>(std::ceil(seamAngles.size() * 0.95)) - 1)];
    size_t duplicateFaceCount = 0;
    for (const auto& entry : duplicateFaces) {
        if (entry.second > 1) duplicateFaceCount += entry.second - 1;
    }

    constexpr float weldTolerance = 1e-4f;
    std::map<std::tuple<int64_t, int64_t, int64_t>, uint32_t> cells;
    MeshData welded;
    std::vector<uint32_t> remap(result.mesh.vertices.size());
    for (size_t i = 0; i < result.mesh.vertices.size(); ++i) {
        const Vec3& p = result.mesh.vertices[i].position;
        const auto key = std::make_tuple(
            static_cast<int64_t>(std::llround(p.x / weldTolerance)),
            static_cast<int64_t>(std::llround(p.y / weldTolerance)),
            static_cast<int64_t>(std::llround(p.z / weldTolerance)));
        const auto found = cells.find(key);
        if (found == cells.end()) {
            const uint32_t id = static_cast<uint32_t>(welded.vertices.size());
            cells[key] = id;
            welded.vertices.push_back(result.mesh.vertices[i]);
            remap[i] = id;
        } else {
            remap[i] = found->second;
        }
    }
    for (size_t i = 0; i + 2 < result.mesh.indices.size(); i += 3) {
        const uint32_t a = remap[result.mesh.indices[i]];
        const uint32_t b = remap[result.mesh.indices[i + 1]];
        const uint32_t c = remap[result.mesh.indices[i + 2]];
        if (a != b && b != c && c != a) welded.indices.insert(welded.indices.end(), {a, b, c});
    }
    const TopologyReport weldedTopology = analyzeTopology(welded);
    const canonical::SelfIntersectionReport intersections =
        canonical::detectSelfIntersections(result.mesh);

    size_t leftVertices = 0;
    size_t rightVertices = 0;
    for (RegionId region : result.vertexRegions) {
        if (region == RegionId::ShoulderGirdleLeft || region == RegionId::AxillaLeft ||
            region == RegionId::UpperArmRootLeft || region == RegionId::BridgeLeft) ++leftVertices;
        if (region == RegionId::ShoulderGirdleRight || region == RegionId::AxillaRight ||
            region == RegionId::UpperArmRootRight || region == RegionId::BridgeRight) ++rightVertices;
    }
    std::fprintf(stderr,
                 "VR1A-DIAG indexed comps=%d boundary=%d nonmanifold=%d degen=%d "
                 "edge1=%zu edge2=%zu edge3+=%zu welded_verts=%zu welded_comps=%d "
                 "welded_boundary=%zu welded_nonmanifold=%zu duplicate_faces=%zu slivers=%zu "
                 "self_intersections=%zu seam_max_deg=%.3f seam_p95_deg=%.3f "
                 "left_verts=%zu right_verts=%zu registered_boundaries=%zu oq03=%s\n",
                 result.topology.components, result.topology.boundaryEdges,
                 result.topology.nonManifoldEdges, result.topology.degenerateTriangles,
                 edgeIncidence[1], edgeIncidence[2], edgeIncidence[3],
                 welded.vertices.size(), weldedTopology.components,
                 static_cast<size_t>(weldedTopology.boundaryEdges),
                 static_cast<size_t>(weldedTopology.nonManifoldEdges),
                 duplicateFaceCount, sliverTriangles,
                 intersections.intersectingPairs, maxSeamAngle, p95SeamAngle,
                 leftVertices, rightVertices, result.report.boundaries.size(),
                 result.oq03Applicable ? "APPLICABLE" : "NOT_APPLICABLE");
    for (size_t i = 0; i < intersections.samples.size() && i < 12; ++i) {
        const auto pair = intersections.samples[i];
        Vec3 ca{};
        Vec3 cb{};
        for (size_t corner = 0; corner < 3; ++corner) {
            ca = ca + result.mesh.vertices[result.mesh.indices[pair[0] * 3 + corner]].position;
            cb = cb + result.mesh.vertices[result.mesh.indices[pair[1] * 3 + corner]].position;
        }
        ca = ca * (1.0f / 3.0f);
        cb = cb * (1.0f / 3.0f);
        std::fprintf(stderr, "VR1A-INTERSECTION tri=(%u,%u) a=(%.4f,%.4f,%.4f) b=(%.4f,%.4f,%.4f) "
                            "ids=(%u,%u,%u)/(%u,%u,%u) regions=(%d,%d,%d)/(%d,%d,%d)\n",
                     pair[0], pair[1], ca.x, ca.y, ca.z, cb.x, cb.y, cb.z,
                     result.mesh.indices[pair[0] * 3], result.mesh.indices[pair[0] * 3 + 1],
                     result.mesh.indices[pair[0] * 3 + 2], result.mesh.indices[pair[1] * 3],
                     result.mesh.indices[pair[1] * 3 + 1], result.mesh.indices[pair[1] * 3 + 2],
                     static_cast<int>(result.vertexRegions[result.mesh.indices[pair[0] * 3]]),
                     static_cast<int>(result.vertexRegions[result.mesh.indices[pair[0] * 3 + 1]]),
                     static_cast<int>(result.vertexRegions[result.mesh.indices[pair[0] * 3 + 2]]),
                     static_cast<int>(result.vertexRegions[result.mesh.indices[pair[1] * 3]]),
                     static_cast<int>(result.vertexRegions[result.mesh.indices[pair[1] * 3 + 1]]),
                     static_cast<int>(result.vertexRegions[result.mesh.indices[pair[1] * 3 + 2]]));
    }
    EXPECT_TRUE(duplicateFaceCount == 0);
    EXPECT_TRUE(intersections.intersectingPairs == 0);
    EXPECT_TRUE(weldedTopology.components == 1);
    EXPECT_TRUE(weldedTopology.boundaryEdges == 0);
    EXPECT_TRUE(weldedTopology.nonManifoldEdges == 0);
}

int main() { return test::runAllTests(); }