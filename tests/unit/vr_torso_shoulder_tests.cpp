#include "mini_test.hpp"

#include "procedural/MeshTopology.h"
#include "procedural/character/AnatomicalLandmarks.h"
#include "procedural/humanoid/canonical/CanonicalSkinGenerator.h"
#include "procedural/humanoid/canonical/CanonicalTopology.h"
#include "procedural/character/BodyPatchGenerator.h"
#include "procedural/character/GeometryBuildRequest.h"
#include "procedural/character/LocalShapeBasis.h"
#include "procedural/character/LodCorrespondence.h"
#include "procedural/character/PatchAssembler.h"
#include "procedural/character/RegionWeightGenerator.h"
#include "procedural/character/ResolvedCharacterState.h"
#include "procedural/character/TorsoShoulderPatchGenerator.h"
#include "procedural/character/VariableProfileSweep.h"
#include "procedural/shape/v2/CharacterDefinition.h"
#include "motion/providers/SyntheticPoseProvider.h"
#include "skeleton/Skinning.h"

#include <algorithm>
#include <array>
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
    GeometryBuildRequest request = makeRequest(definition);
    request.generatorVersion = kR1GeneratorVersion;
    return TorsoShoulderPatchGenerator::generate(definition, request);
}

TorsoShoulderResult generateR2(const shape::v2::CharacterDefinition& definition,
                               uint32_t detailLevel = 1) {
    GeometryBuildRequest request = makeGeometryBuildRequest(definition, detailLevel, 2);
    request.generatorVersion = kR2GeneratorVersion;
    return TorsoShoulderPatchGenerator::generate(definition, request);
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

struct R2Diagnostics {
    size_t globalSamples = 0;
    size_t seamSamples = 0;
    size_t curvatureSamples = 0;
    size_t slivers = 0;
    float globalMaxDeg = 0.0f;
    float globalP95Deg = 0.0f;
    float seamMaxDeg = 0.0f;
    float seamP95Deg = 0.0f;
    float curvatureMax = 0.0f;
    float curvatureP95 = 0.0f;
    float minTriangleAngleDeg = 180.0f;
    float maxAspect = 1.0f;
};

float percentile95(std::vector<float> values) {
    if (values.empty()) return 0.0f;
    std::sort(values.begin(), values.end());
    return values[std::min(values.size() - 1,
                           static_cast<size_t>(std::ceil(values.size() * 0.95)) - 1)];
}

R2Diagnostics measureR2Diagnostics(const TorsoShoulderResult& result) {
    R2Diagnostics diagnostics;
    struct EdgeFaces {
        uint32_t a = UINT32_MAX;
        uint32_t b = UINT32_MAX;
    };
    std::map<std::pair<uint32_t, uint32_t>, EdgeFaces> edges;
    std::vector<Vec3> faceNormals;
    std::vector<std::vector<uint32_t>> vertexNeighbours(result.mesh.vertices.size());
    for (size_t i = 0; i + 2 < result.mesh.indices.size(); i += 3) {
        const uint32_t ids[3] = {result.mesh.indices[i], result.mesh.indices[i + 1],
                                 result.mesh.indices[i + 2]};
        if (ids[0] >= result.mesh.vertices.size() || ids[1] >= result.mesh.vertices.size() ||
            ids[2] >= result.mesh.vertices.size()) {
            return diagnostics;
        }
        const Vec3& a = result.mesh.vertices[ids[0]].position;
        const Vec3& b = result.mesh.vertices[ids[1]].position;
        const Vec3& c = result.mesh.vertices[ids[2]].position;
        faceNormals.push_back((b - a).cross(c - a).normalized());
        const float lengths[3] = {(b - a).length(), (c - b).length(), (a - c).length()};
        const float minLength = std::fmin(lengths[0], std::fmin(lengths[1], lengths[2]));
        const float maxLength = std::fmax(lengths[0], std::fmax(lengths[1], lengths[2]));
        diagnostics.maxAspect = std::fmax(diagnostics.maxAspect, maxLength / minLength);
        if (minLength <= 1e-7f || maxLength / minLength > 10.0f) ++diagnostics.slivers;
        for (size_t corner = 0; corner < 3; ++corner) {
            const uint32_t v0 = ids[corner];
            const uint32_t v1 = ids[(corner + 1) % 3];
            const uint32_t v2 = ids[(corner + 2) % 3];
            const Vec3 e0 = result.mesh.vertices[v1].position - result.mesh.vertices[v0].position;
            const Vec3 e1 = result.mesh.vertices[v2].position - result.mesh.vertices[v0].position;
            const float angle = std::acos(std::clamp(e0.normalized().dot(e1.normalized()), -1.0f, 1.0f));
            diagnostics.minTriangleAngleDeg = std::fmin(diagnostics.minTriangleAngleDeg,
                                                         angle * 57.2957795f);
            const uint32_t lo = std::min(v0, v1);
            const uint32_t hi = std::max(v0, v1);
            EdgeFaces& faces = edges[{lo, hi}];
            if (faces.a == UINT32_MAX) faces.a = static_cast<uint32_t>(i / 3);
            else faces.b = static_cast<uint32_t>(i / 3);
            vertexNeighbours[v0].push_back(v1);
            vertexNeighbours[v1].push_back(v0);
        }
    }
    std::vector<float> globalAngles;
    for (const auto& entry : edges) {
        const EdgeFaces& faces = entry.second;
        if (faces.b == UINT32_MAX) continue;
        const float dot = std::clamp(faceNormals[faces.a].dot(faceNormals[faces.b]), -1.0f, 1.0f);
        const float angle = std::acos(dot) * 57.2957795f;
        globalAngles.push_back(angle);
        diagnostics.globalMaxDeg = std::fmax(diagnostics.globalMaxDeg, angle);
    }
    diagnostics.globalSamples = globalAngles.size();
    diagnostics.globalP95Deg = percentile95(globalAngles);

    std::set<std::string> seenBoundaries;
    std::vector<float> seamAngles;
    for (const PatchBoundary& boundary : result.boundaryLoops) {
        if (boundary.crop || !seenBoundaries.insert(boundary.id).second) continue;
        const size_t edgeCount = boundary.closed ? boundary.vertexIds.size()
                                                  : boundary.vertexIds.size() - 1;
        for (size_t i = 0; i < edgeCount; ++i) {
            const uint32_t a = boundary.vertexIds[i];
            const uint32_t b = boundary.vertexIds[(i + 1) % boundary.vertexIds.size()];
            const auto found = edges.find({std::min(a, b), std::max(a, b)});
            if (found == edges.end() || found->second.b == UINT32_MAX) continue;
            const float dot = std::clamp(
                faceNormals[found->second.a].dot(faceNormals[found->second.b]), -1.0f, 1.0f);
            const float angle = std::acos(dot) * 57.2957795f;
            seamAngles.push_back(angle);
            diagnostics.seamMaxDeg = std::fmax(diagnostics.seamMaxDeg, angle);
        }
    }
    diagnostics.seamSamples = seamAngles.size();
    diagnostics.seamP95Deg = percentile95(seamAngles);

    std::vector<float> curvature;
    for (size_t vertex = 0; vertex < result.mesh.vertices.size(); ++vertex) {
        for (uint32_t neighbour : vertexNeighbours[vertex]) {
            if (neighbour <= vertex) continue;
            const float distance = (result.mesh.vertices[vertex].position -
                                    result.mesh.vertices[neighbour].position).length();
            if (distance <= 1e-7f) continue;
            const float dot = std::clamp(
                result.mesh.vertices[vertex].normal.dot(result.mesh.vertices[neighbour].normal),
                -1.0f, 1.0f);
            const float value = std::acos(dot) / distance;
            curvature.push_back(value);
            diagnostics.curvatureMax = std::fmax(diagnostics.curvatureMax, value);
        }
    }
    diagnostics.curvatureSamples = curvature.size();
    diagnostics.curvatureP95 = percentile95(curvature);
    return diagnostics;
}

} // namespace

PAE_TEST_CASE(vr_data_layer_resolves_single_authority) {
    shape::v2::CharacterDefinition definition = makeDefinition();
    definition.landmarks.placed.push_back(
        {"chest-center", {0.0f, 2.0f, 0.0f}, shape::v2::ValueConfidence::Assumed, "unit-test"});
    const ResolvedCharacterState state = resolveCharacterState(definition);
    EXPECT_TRUE(state.valid);
    EXPECT_TRUE(state.skeleton.bones.size() == BoneCount);
    EXPECT_TRUE(state.landmarks.points.size() == static_cast<size_t>(character::LandmarkId::Count));
    const character::LandmarkPoint* chest = state.landmarks.find(character::LandmarkId::ChestCenter);
    EXPECT_TRUE(chest != nullptr);
    EXPECT_NEAR(chest->position.y, 2.0f, 1e-6f);
    EXPECT_TRUE(chest->confidence == shape::v2::ValueConfidence::Assumed);
    EXPECT_TRUE(chest->provenance == "unit-test");
    EXPECT_TRUE(state.landmarkReport.usedLegacyJoints == false);

    shape::v2::CharacterDefinition measured = makeDefinition();
    measured.landmarks.placed.push_back(
        {"chest-center", {0.0f, 1.3f, 0.14f}, shape::v2::ValueConfidence::Measured, "unit-measured"});
    const ResolvedCharacterState measuredState = resolveCharacterState(measured);
    const character::LandmarkPoint* measuredChest = measuredState.landmarks.find(character::LandmarkId::ChestCenter);
    EXPECT_TRUE(measuredChest != nullptr);
    EXPECT_TRUE(measuredChest->confidence == shape::v2::ValueConfidence::Measured);
    EXPECT_TRUE(measuredChest->provenance == "unit-measured");
}

PAE_TEST_CASE(vr_r2_contract_has_stations_and_complete_hash) {
    const shape::v2::CharacterDefinition definition = makeDefinition();
    EXPECT_TRUE(definition.schemaVersion == 3u);
    EXPECT_TRUE(definition.torso.controlStations.size() == 12u);
    for (size_t i = 1; i < definition.torso.controlStations.size(); ++i) {
        EXPECT_TRUE(definition.torso.controlStations[i].height >
                    definition.torso.controlStations[i - 1].height);
    }
    const GeometryBuildRequest request = makeRequest(definition);
    EXPECT_TRUE(request.generatorVersion == kR2GeneratorVersion);

    shape::v2::CharacterDefinition clavicle = definition;
    clavicle.torso.clavicleRelief = 0.03f;
    EXPECT_TRUE(hashCharacterDefinition(clavicle) != hashCharacterDefinition(definition));
    shape::v2::CharacterDefinition acromion = definition;
    acromion.landmarks.placed.push_back({"left-acromion", {0.30f, 1.40f, 0.0f}});
    EXPECT_TRUE(hashCharacterDefinition(acromion) != hashCharacterDefinition(definition));
    shape::v2::CharacterDefinition rightDelta = definition;
    rightDelta.asymmetry.rightUpperArmRadiusDelta = 0.40f;
    EXPECT_TRUE(hashCharacterDefinition(rightDelta) != hashCharacterDefinition(definition));

    const shape::v2::CharacterDefinition r1 = shape::v2::defaultR1CharacterDefinition();
    const GeometryBuildRequest r1Request = makeGeometryBuildRequest(r1, 1, 2);
    EXPECT_TRUE(r1.schemaVersion == 2u);
    EXPECT_TRUE(r1.torso.controlStations.empty());
    EXPECT_TRUE(r1Request.generatorVersion == kR1GeneratorVersion);
    EXPECT_TRUE(hashCharacterDefinition(r1) == 8849779482416134771ull);
}

PAE_TEST_CASE(vr_landmark_override_controls_resolved_station) {
    const shape::v2::CharacterDefinition base = makeDefinition();
    const ResolvedCharacterState baseState = resolveCharacterState(base);
    const shape::v2::TorsoControlStation* baseCostal = nullptr;
    for (const auto& station : baseState.torsoStations) {
        if (station.id == "costal-margin") baseCostal = &station;
    }
    EXPECT_TRUE(baseCostal != nullptr);

    shape::v2::CharacterDefinition changed = base;
    changed.landmarks.placed.push_back(
        {"costal-margin", {0.0f, 1.30f, 0.130f}, shape::v2::ValueConfidence::Assumed, "unit-test"});
    const ResolvedCharacterState changedState = resolveCharacterState(changed);
    const shape::v2::TorsoControlStation* changedCostal = nullptr;
    for (const auto& station : changedState.torsoStations) {
        if (station.id == "costal-margin") changedCostal = &station;
    }
    EXPECT_TRUE(changedCostal != nullptr);
    EXPECT_TRUE(changedCostal->height > baseCostal->height);
    EXPECT_TRUE(changedCostal->controls.front.z > baseCostal->controls.front.z);
}

PAE_TEST_CASE(vr_patch_assembler_shared_boundary_contract) {
    const auto makeVertex = [](const Vec3& position) {
        Vertex vertex;
        vertex.position = position;
        vertex.normal = {0.0f, 1.0f, 0.0f};
        return vertex;
    };
    SurfacePatch a;
    a.id = "a";
    a.valid = true;
    a.mesh.vertices = {makeVertex({0, 0, 0}), makeVertex({1, 0, 0}),
                       makeVertex({2, 0, 0}), makeVertex({1, 1, 0})};
    a.mesh.indices = {3, 0, 1, 3, 1, 2};
    a.stableIds = {1, 2, 3, 4};
    a.vertexRegions = {RegionId::Thorax, RegionId::Thorax, RegionId::Thorax, RegionId::Thorax};
    a.boundaries.push_back({"seam", RegionId::Thorax, {0, 1, 2}, false, false});

    SurfacePatch b = a;
    b.id = "b";
    b.mesh.vertices[3] = makeVertex({1, -1, 0});
    b.mesh.indices = {3, 2, 1, 3, 1, 0};
    b.stableIds = {1, 2, 3, 5};
    b.boundaries.front().vertexIds = {2, 1, 0};

    const PatchAssemblyResult assembled = assemblePatches({a, b});
    std::fprintf(stderr, "VR1A-ASSEMBLY valid=%d error=%s\n", assembled.valid, assembled.error.c_str());
    EXPECT_TRUE(assembled.valid);
    EXPECT_TRUE(assembled.mesh.vertices.size() == 5u);
    EXPECT_TRUE(assembled.mesh.indices.size() == 12u);
    EXPECT_TRUE(assembled.boundaries.size() == 2u);

    SurfacePatch badPosition = b;
    badPosition.mesh.vertices[2].position.z = 0.01f;
    EXPECT_TRUE(!assemblePatches({a, badPosition}).valid);
    SurfacePatch badId = b;
    badId.stableIds[2] = 99;
    EXPECT_TRUE(!assemblePatches({a, badId}).valid);
}

PAE_TEST_CASE(vr_r2_torso_stations_shape_and_landmarks) {
    const shape::v2::CharacterDefinition definition = makeDefinition();
    const TorsoShoulderResult result = generateR2(definition);
    std::fprintf(stderr, "VR1A-R2 valid=%d error=%s verts=%zu tris=%zu boundary=%d weights=%d frame=%d comps=%d nonmanifold=%d degen=%d badnorm=%d\n",
                 result.valid, result.error.c_str(), result.mesh.vertices.size(),
                 result.mesh.indices.size() / 3, result.topology.boundaryEdges,
                 result.weights.valid, result.frameDiagnostics.valid,
                 result.topology.components, result.topology.nonManifoldEdges,
                 result.topology.degenerateTriangles, result.topology.invalidNormals);
    float rightMinX = 1e9f, rightMaxX = -1e9f, rightMinY = 1e9f, rightMaxY = -1e9f;
    for (size_t i = 0; i < result.mesh.vertices.size(); ++i) {
        if (result.vertexRegions[i] != RegionId::UpperArmRootRight) continue;
        const Vec3& p = result.mesh.vertices[i].position;
        rightMinX = std::fmin(rightMinX, p.x); rightMaxX = std::fmax(rightMaxX, p.x);
        rightMinY = std::fmin(rightMinY, p.y); rightMaxY = std::fmax(rightMaxY, p.y);
    }
    std::fprintf(stderr, "VR1A-R2-RIGHT-ARM x=[%.4f,%.4f] y=[%.4f,%.4f]\n",
                 rightMinX, rightMaxX, rightMinY, rightMaxY);
    const character::LandmarkPoint* dbgLeft = result.state.landmarks.find(character::LandmarkId::LeftAcromion);
    const character::LandmarkPoint* dbgRight = result.state.landmarks.find(character::LandmarkId::RightAcromion);
    std::fprintf(stderr, "VR1A-R2-ACROMION left=(%.4f,%.4f) right=(%.4f,%.4f)\n",
                 dbgLeft->position.x, dbgLeft->position.y, dbgRight->position.x, dbgRight->position.y);
    const R2Diagnostics r2Diagnostics = measureR2Diagnostics(result);
    std::fprintf(stderr,
                 "VR1A-R2-DIAG global_samples=%zu global_max=%.3f global_p95=%.3f "
                 "seam_samples=%zu seam_max=%.3f seam_p95=%.3f curvature_samples=%zu "
                 "curvature_max=%.3f curvature_p95=%.3f slivers=%zu min_angle=%.3f max_aspect=%.3f\n",
                 r2Diagnostics.globalSamples, r2Diagnostics.globalMaxDeg, r2Diagnostics.globalP95Deg,
                 r2Diagnostics.seamSamples, r2Diagnostics.seamMaxDeg, r2Diagnostics.seamP95Deg,
                 r2Diagnostics.curvatureSamples, r2Diagnostics.curvatureMax,
                 r2Diagnostics.curvatureP95, r2Diagnostics.slivers,
                 r2Diagnostics.minTriangleAngleDeg, r2Diagnostics.maxAspect);
    EXPECT_TRUE(r2Diagnostics.seamSamples > 0);
    const canonical::SelfIntersectionReport r2Intersections =
        canonical::detectSelfIntersections(result.mesh);
    std::fprintf(stderr, "VR1A-R2-INTERSECTIONS count=%zu\n", r2Intersections.intersectingPairs);
    for (size_t sample = 0; sample < r2Intersections.samples.size() && sample < 12; ++sample) {
        const auto pair = r2Intersections.samples[sample];
        Vec3 ca{};
        Vec3 cb{};
        for (size_t corner = 0; corner < 3; ++corner) {
            ca = ca + result.mesh.vertices[result.mesh.indices[pair[0] * 3 + corner]].position;
            cb = cb + result.mesh.vertices[result.mesh.indices[pair[1] * 3 + corner]].position;
        }
        ca = ca * (1.0f / 3.0f);
        cb = cb * (1.0f / 3.0f);
        std::fprintf(stderr, "VR1A-R2-INTERSECTION tri=(%u,%u) a=(%.4f,%.4f,%.4f) b=(%.4f,%.4f,%.4f)\n",
                     pair[0], pair[1], ca.x, ca.y, ca.z, cb.x, cb.y, cb.z);
    }
    EXPECT_TRUE(r2Intersections.intersectingPairs == 0);
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.mesh.vertices.size() > 1400u);
    EXPECT_TRUE(result.topology.components == 1);
    EXPECT_TRUE(result.topology.boundaryEdges == 32);
    EXPECT_TRUE(result.topology.nonManifoldEdges == 0);
    EXPECT_TRUE(result.report.boundaries.size() == 3u);
    bool hasNeckBoundary = false;
    for (const BoundaryReport& boundary : result.report.boundaries) {
        if (boundary.id == "neck-base-interface") hasNeckBoundary = true;
    }
    EXPECT_TRUE(hasNeckBoundary);

    float waistWidth = 0.0f;
    float chestWidth = 0.0f;
    float waistMinZ = 1e9f;
    float waistMaxZ = -1e9f;
    float chestMinZ = 1e9f;
    float chestMaxZ = -1e9f;
    for (size_t i = 0; i < result.mesh.vertices.size(); ++i) {
        const Vertex& vertex = result.mesh.vertices[i];
        const RegionId region = result.vertexRegions[i];
        if (region == RegionId::UpperArmRootLeft || region == RegionId::UpperArmRootRight ||
            region == RegionId::BridgeLeft || region == RegionId::BridgeRight ||
            region == RegionId::ShoulderGirdleLeft || region == RegionId::ShoulderGirdleRight) {
            continue;
        }
        const float y = vertex.position.y;
        if (y > 1.15f && y < 1.20f) {
            waistWidth = std::fmax(waistWidth, std::fabs(vertex.position.x));
            waistMinZ = std::fmin(waistMinZ, vertex.position.z);
            waistMaxZ = std::fmax(waistMaxZ, vertex.position.z);
        }
        if (y > 1.34f && y < 1.39f) {
            chestWidth = std::fmax(chestWidth, std::fabs(vertex.position.x));
            chestMinZ = std::fmin(chestMinZ, vertex.position.z);
            chestMaxZ = std::fmax(chestMaxZ, vertex.position.z);
        }
    }
    std::fprintf(stderr, "VR1A-R2-SHAPE waist_w=%.4f chest_w=%.4f waist_d=%.4f chest_d=%.4f\n",
                 waistWidth, chestWidth, waistMaxZ - waistMinZ, chestMaxZ - chestMinZ);
    EXPECT_TRUE(waistWidth < chestWidth - 0.03f);
    EXPECT_TRUE(waistMaxZ - waistMinZ < chestMaxZ - chestMinZ - 0.03f);

    shape::v2::CharacterDefinition moved = definition;
    moved.landmarks.placed.push_back(
        {"costal-margin", {0.0f, 1.30f, 0.130f}, shape::v2::ValueConfidence::Assumed, "unit-test"});
    const TorsoShoulderResult movedResult = generateR2(moved);
    float maxMove = 0.0f;
    for (size_t i = 0; i < result.mesh.vertices.size() && i < movedResult.mesh.vertices.size(); ++i) {
        maxMove = std::fmax(maxMove,
            (result.mesh.vertices[i].position - movedResult.mesh.vertices[i].position).length());
    }
    EXPECT_TRUE(maxMove > 0.001f);

    shape::v2::CharacterDefinition wider = definition;
    wider.shoulders.leftWidthScale = 1.15f;
    const TorsoShoulderResult widerResult = generateR2(wider);
    float maxLeft = 0.0f;
    float maxRight = 0.0f;
    for (size_t i = 0; i < result.mesh.vertices.size() && i < widerResult.mesh.vertices.size(); ++i) {
        const float delta = (result.mesh.vertices[i].position - widerResult.mesh.vertices[i].position).length();
        if (result.mesh.vertices[i].position.x > 0.0f) maxLeft = std::fmax(maxLeft, delta);
        if (result.mesh.vertices[i].position.x < 0.0f) maxRight = std::fmax(maxRight, delta);
    }
    EXPECT_TRUE(maxLeft > 0.001f);
    EXPECT_TRUE(maxRight < 1e-6f);
}

PAE_TEST_CASE(vr_r2_detail_levels_change_topology) {
    const shape::v2::CharacterDefinition definition = makeDefinition();
    const TorsoShoulderResult low = generateR2(definition, 0);
    const TorsoShoulderResult medium = generateR2(definition, 1);
    const TorsoShoulderResult high = generateR2(definition, 2);
    EXPECT_TRUE(low.valid && medium.valid && high.valid);
    const canonical::SelfIntersectionReport highIntersections =
        canonical::detectSelfIntersections(high.mesh);
    std::fprintf(stderr, "VR1A-R2-HIGH-INTERSECTIONS count=%zu\n", highIntersections.intersectingPairs);
    for (size_t i = 0; i < highIntersections.samples.size() && i < 6; ++i) {
        const auto pair = highIntersections.samples[i];
        Vec3 ca{};
        Vec3 cb{};
        for (size_t corner = 0; corner < 3; ++corner) {
            ca = ca + high.mesh.vertices[high.mesh.indices[pair[0] * 3 + corner]].position;
            cb = cb + high.mesh.vertices[high.mesh.indices[pair[1] * 3 + corner]].position;
        }
        ca = ca * (1.0f / 3.0f);
        cb = cb * (1.0f / 3.0f);
        std::fprintf(stderr, "VR1A-R2-HIGH-INTERSECTION tri=(%u,%u) a=(%.4f,%.4f,%.4f) b=(%.4f,%.4f,%.4f)\n",
                     pair[0], pair[1], ca.x, ca.y, ca.z, cb.x, cb.y, cb.z);
    }
    EXPECT_TRUE(low.mesh.vertices.size() < medium.mesh.vertices.size());
    EXPECT_TRUE(medium.mesh.vertices.size() < high.mesh.vertices.size());
    EXPECT_TRUE(low.mesh.indices.size() < medium.mesh.indices.size());
    EXPECT_TRUE(medium.mesh.indices.size() < high.mesh.indices.size());
    EXPECT_TRUE(canonical::detectSelfIntersections(low.mesh).intersectingPairs == 0);
    EXPECT_TRUE(highIntersections.intersectingPairs == 0);
    const LodMap lineage = buildLodMap(low.mesh, high.mesh, 0, 2);
    EXPECT_TRUE(lineage.valid);
    EXPECT_TRUE(lineage.vertices.size() == high.mesh.vertices.size());
    for (const RefinementVertex& vertex : lineage.vertices) {
        EXPECT_TRUE(vertex.parentFaceStableId != 0);
        EXPECT_TRUE(vertex.barycentric.x >= 0.0f && vertex.barycentric.y >= 0.0f);
        EXPECT_TRUE(vertex.barycentric.x + vertex.barycentric.y <= 1.0001f);
    }
    std::fprintf(stderr, "VR1A-R2-LOD verts=%zu/%zu/%zu tris=%zu/%zu/%zu\n",
                 low.mesh.vertices.size(), medium.mesh.vertices.size(), high.mesh.vertices.size(),
                 low.mesh.indices.size() / 3, medium.mesh.indices.size() / 3,
                 high.mesh.indices.size() / 3);
}

PAE_TEST_CASE(vr_r2_shoulder_points_and_actions_are_compatible) {
    const shape::v2::CharacterDefinition definition = makeDefinition();
    const TorsoShoulderResult result = generateR2(definition);
    EXPECT_TRUE(result.valid);
    const character::LandmarkPoint* acromion = result.state.landmarks.find(character::LandmarkId::LeftAcromion);
    const character::LandmarkPoint* shoulder = result.state.landmarks.find(character::LandmarkId::LeftShoulderJoint);
    const character::LandmarkPoint* root = result.state.landmarks.find(character::LandmarkId::LeftUpperArmRoot);
    const character::LandmarkPoint* deltoid = result.state.landmarks.find(character::LandmarkId::LeftDeltoidSurface);
    EXPECT_TRUE(acromion && shoulder && root && deltoid);
    EXPECT_TRUE((acromion->position - shoulder->position).length() > 0.10f);
    EXPECT_TRUE((root->position - acromion->position).length() > 0.05f);
    EXPECT_TRUE(deltoid->position.x > acromion->position.x);
    const character::LandmarkPoint* rightAcromion = result.state.landmarks.find(character::LandmarkId::RightAcromion);
    const character::LandmarkPoint* rightRoot = result.state.landmarks.find(character::LandmarkId::RightUpperArmRoot);
    EXPECT_TRUE(rightAcromion && rightRoot);
    EXPECT_TRUE(rightAcromion->position.x < 0.0f);
    EXPECT_TRUE(rightRoot->position.x < 0.0f);

    shape::v2::CharacterDefinition moved = definition;
    moved.landmarks.placed.push_back(
        {"left-acromion", {0.30f, 1.40f, 0.0f}, shape::v2::ValueConfidence::Assumed, "unit-test"});
    const TorsoShoulderResult movedResult = generateR2(moved);
    float maxLeft = 0.0f;
    float maxRight = 0.0f;
    for (size_t i = 0; i < result.mesh.vertices.size() && i < movedResult.mesh.vertices.size(); ++i) {
        const float delta = (result.mesh.vertices[i].position - movedResult.mesh.vertices[i].position).length();
        if (result.mesh.vertices[i].position.x > 0.0f) maxLeft = std::fmax(maxLeft, delta);
        if (result.mesh.vertices[i].position.x < 0.0f) maxRight = std::fmax(maxRight, delta);
    }
    EXPECT_TRUE(maxLeft > 0.001f);
    EXPECT_TRUE(maxRight < 1e-6f);

    for (int action : {1, 3}) {
        std::array<Quat, BoneCount> rotations{};
        SyntheticPoseProvider::rotationsForAction(action, rotations);
        const PoseResult pose = computePose(result.state.skeleton, rotations);
        const std::vector<Vec3> skinned = cpuSkinPositions(result.mesh, pose.palette);
        EXPECT_TRUE(skinned.size() == result.mesh.vertices.size());
        float maxDisplacement = 0.0f;
        for (size_t i = 0; i < skinned.size(); ++i) {
            EXPECT_TRUE(std::isfinite(skinned[i].x) && std::isfinite(skinned[i].y) &&
                        std::isfinite(skinned[i].z));
            maxDisplacement = std::fmax(maxDisplacement,
                (skinned[i] - result.mesh.vertices[i].position).length());
        }
        EXPECT_TRUE(maxDisplacement > 0.05f);
        EXPECT_TRUE(maxDisplacement < 0.45f);
    }
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