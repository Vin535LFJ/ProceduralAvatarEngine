#include "mini_test.hpp"

#include "procedural/humanoid/canonical/CanonicalSkinGenerator.h"
#include "procedural/humanoid/canonical/ContinuousSkinGenerator.h"
#include "procedural/fusion/SdfFusion.h"
#include "procedural/MeshTopology.h"
#include "skeleton/Skeleton.h"
#include "skeleton/Skinning.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace pae;
using namespace pae::canonical;

namespace {

MeshData openCylinder(uint32_t segments, uint32_t rows, float y0, float y1,
                      uint16_t bone, float radius = 0.1f) {
    MeshData mesh;
    for (uint32_t row = 0; row < rows; ++row) {
        const float y = y0 + (y1 - y0) * row / (rows - 1);
        for (uint32_t column = 0; column < segments; ++column) {
            const float angle = 6.28318530718f * column / segments;
            Vertex vertex;
            vertex.position = {radius * std::cos(angle), y, radius * std::sin(angle)};
            vertex.normal = {std::cos(angle), 0.0f, std::sin(angle)};
            vertex.joints = {bone, 0, 0, 0};
            vertex.weights = {1.0f, 0.0f, 0.0f, 0.0f};
            mesh.vertices.push_back(vertex);
        }
    }
    for (uint32_t row = 0; row + 1 < rows; ++row) {
        for (uint32_t column = 0; column < segments; ++column) {
            const uint32_t next = (column + 1) % segments;
            const uint32_t a = row * segments + column;
            const uint32_t b = row * segments + next;
            const uint32_t c = (row + 1) * segments + column;
            const uint32_t d = (row + 1) * segments + next;
            mesh.indices.insert(mesh.indices.end(), {a, c, b, b, c, d});
        }
    }
    return mesh;
}

void appendMesh(MeshData& destination, const MeshData& source) {
    const uint32_t offset = static_cast<uint32_t>(destination.vertices.size());
    destination.vertices.insert(destination.vertices.end(), source.vertices.begin(), source.vertices.end());
    for (uint32_t index : source.indices) destination.indices.push_back(offset + index);
}

BoundaryLoop loopAtHeight(const MeshData& mesh, float height, size_t occurrence = 0) {
    const auto loops = extractBoundaryLoops(mesh);
    std::vector<const BoundaryLoop*> matches;
    float bestError = 1e9f;
    for (const auto& loop : loops) {
        float y = 0.0f;
        for (uint32_t id : loop.vertexIds) y += mesh.vertices[id].position.y;
        y /= loop.vertexIds.size();
        const float error = std::fabs(y - height);
        if (error < bestError - 1e-4f) {
            bestError = error;
            matches.clear();
        }
        if (std::fabs(error - bestError) < 1e-4f) matches.push_back(&loop);
    }
    return occurrence < matches.size() ? *matches[occurrence] : BoundaryLoop{};
}

} // namespace

PAE_TEST_CASE(p1a1_different_density_boundaries_resample_by_arc_length) {
    MeshData a = openCylinder(7, 3, 0.0f, 0.2f, BoneChest);
    MeshData b = openCylinder(13, 3, 0.2f, 0.4f, BoneLeftUpperArm);
    const auto loopA = loopAtHeight(a, 0.2f);
    const auto loopB = loopAtHeight(b, 0.2f);
    EXPECT_TRUE(loopA.vertexIds.size() == 7u);
    EXPECT_TRUE(loopB.vertexIds.size() == 13u);
    const auto points = resampleBoundary(a, loopA, 13);
    EXPECT_TRUE(points.size() == 13u);
    for (size_t i = 0; i < points.size(); ++i) {
        const float chord = (points[i] - points[(i + 1) % points.size()]).length();
        EXPECT_TRUE(chord > 0.005f && chord < 0.1f);
        EXPECT_NEAR(std::sqrt(points[i].x * points[i].x + points[i].z * points[i].z), 0.1f, 0.011f);
    }
    MeshData stitched;
    std::vector<uint32_t> shared(13);
    for (uint32_t i = 0; i < 13; ++i) {
        Vertex vertex;
        vertex.position = points[i];
        vertex.normal = {vertex.position.x / 0.1f, 0.0f, vertex.position.z / 0.1f};
        vertex.joints = {BoneChest, BoneLeftUpperArm, 0, 0};
        vertex.weights = {0.5f, 0.5f, 0.0f, 0.0f};
        shared[i] = static_cast<uint32_t>(stitched.vertices.size());
        stitched.vertices.push_back(vertex);
    }
    for (uint32_t i = 0; i < 13; ++i) {
        const uint32_t next = (i + 1) % 13;
        stitched.indices.insert(stitched.indices.end(),
                                {shared[i], shared[next], static_cast<uint32_t>(stitched.vertices.size())});
        Vertex lower;
        lower.position = {points[i].x, 0.0f, points[i].z};
        lower.normal = {lower.position.x / 0.1f, 0.0f, lower.position.z / 0.1f};
        lower.joints = {BoneChest, 0, 0, 0};
        lower.weights = {1.0f, 0.0f, 0.0f, 0.0f};
        stitched.vertices.push_back(lower);
        Vertex upper;
        upper.position = {points[i].x, 0.4f, points[i].z};
        upper.normal = {upper.position.x / 0.1f, 0.0f, upper.position.z / 0.1f};
        upper.joints = {BoneLeftUpperArm, 0, 0, 0};
        upper.weights = {1.0f, 0.0f, 0.0f, 0.0f};
        stitched.vertices.push_back(upper);
        stitched.indices.insert(stitched.indices.end(),
                                {shared[next], static_cast<uint32_t>(stitched.vertices.size() - 1), shared[i]});
    }
    const TopologyReport topology = analyzeTopology(stitched);
    EXPECT_TRUE(topology.components == 1);
    EXPECT_TRUE(topology.nonManifoldEdges == 0);
    EXPECT_TRUE(validateWeights(stitched).invalidVertices == 0u);
}

PAE_TEST_CASE(p1a1_intersecting_regions_weld_to_one_component) {
    MeshData mesh;
    appendMesh(mesh, openCylinder(8, 3, 0.0f, 0.3f, BoneHips));
    appendMesh(mesh, openCylinder(8, 3, 0.3f, 0.6f, BoneSpine));
    const BoundaryLoop reference = loopAtHeight(mesh, 0.3f, 0);
    const BoundaryLoop candidate = loopAtHeight(mesh, 0.3f, 1);
    const WeldResult weld = weldBoundaryLoops(mesh, reference, candidate);
    EXPECT_TRUE(weld.valid);
    EXPECT_TRUE(weld.weldedPairs == 8u);
    const TopologyReport topology = analyzeTopology(mesh);
    EXPECT_TRUE(topology.components == 1);
    EXPECT_TRUE(topology.boundaryEdges == 16);
    EXPECT_TRUE(topology.nonManifoldEdges == 0);
    EXPECT_TRUE(topology.duplicateVertices == 0);
}

PAE_TEST_CASE(p1a1_seam_weights_merge_by_bone_and_top4) {
    MeshData mesh;
    appendMesh(mesh, openCylinder(8, 2, 0.0f, 0.2f, BoneChest));
    appendMesh(mesh, openCylinder(8, 2, 0.2f, 0.4f, BoneLeftUpperArm));
    const BoundaryLoop reference = loopAtHeight(mesh, 0.2f, 0);
    const BoundaryLoop candidate = loopAtHeight(mesh, 0.2f, 1);
    weldBoundaryLoops(mesh, reference, candidate);
    for (Vertex& vertex : mesh.vertices) {
        if (std::fabs(vertex.position.y - 0.2f) < 1e-5f) {
            EXPECT_TRUE(vertex.joints.x == BoneChest);
            EXPECT_TRUE(vertex.joints.y == BoneLeftUpperArm);
            EXPECT_NEAR(vertex.weights.x, 0.5f, 1e-6f);
            EXPECT_NEAR(vertex.weights.y, 0.5f, 1e-6f);
        }
    }
    Vertex destination;
    destination.joints = {BoneHips, BoneSpine, BoneChest, BoneNeck};
    destination.weights = {0.4f, 0.3f, 0.2f, 0.1f};
    Vertex source;
    source.joints = {BoneHead, BoneChest, BoneSpine, BoneLeftHand};
    source.weights = {0.4f, 0.3f, 0.2f, 0.1f};
    mergeVertexWeights(destination, source);
    EXPECT_NEAR(destination.weights.x + destination.weights.y + destination.weights.z +
                destination.weights.w, 1.0f, 1e-6f);
    EXPECT_TRUE(destination.joints.x == BoneChest || destination.joints.x == BoneSpine);
    EXPECT_TRUE(validateWeights(mesh).invalidVertices == 0u);
}

PAE_TEST_CASE(p1a1_uv_seam_render_mapping_preserves_canonical_id) {
    CanonicalSkinResult canonical;
    canonical.mesh = openCylinder(12, 4, 0.0f, 0.4f, BoneSpine);
    canonical.mesh.subMeshes.push_back({0, static_cast<uint32_t>(canonical.mesh.indices.size()), 0});
    const RenderResult render = buildRenderMesh(canonical);
    EXPECT_TRUE(render.valid);
    EXPECT_TRUE(render.mesh.indices.size() == canonical.mesh.indices.size());
    EXPECT_TRUE(render.renderToCanonical.size() == render.mesh.vertices.size());
    EXPECT_TRUE(render.uv.charts == canonical.mesh.indices.size() / 3);
    EXPECT_TRUE(render.uv.renderVertices == render.mesh.vertices.size());
    EXPECT_TRUE(render.uv.invalidUvVertices == 0u);
    EXPECT_TRUE(render.uv.outOfRangeUvVertices == 0u);
    EXPECT_TRUE(render.uv.zeroAreaUvTriangles == 0u);
    for (uint32_t id : render.renderToCanonical) EXPECT_TRUE(id < canonical.mesh.vertices.size());
    for (const Vertex& vertex : render.mesh.vertices) {
        EXPECT_TRUE(std::isfinite(vertex.uv.x) && std::isfinite(vertex.uv.y));
        EXPECT_TRUE(vertex.uv.x >= 0.0f && vertex.uv.x <= 1.0f);
        EXPECT_TRUE(vertex.uv.y >= 0.0f && vertex.uv.y <= 1.0f);
    }
}

PAE_TEST_CASE(p1a1_curved_surface_has_ordered_boundaries_and_valid_weights) {
    MeshData mesh = openCylinder(10, 5, 0.0f, 0.5f, BoneSpine);
    for (Vertex& vertex : mesh.vertices) {
        vertex.position.x += 0.3f * vertex.position.y * vertex.position.y;
        vertex.position.z += 0.1f * std::sin(vertex.position.y * 5.0f);
        vertex.normal = vertex.normal.normalized();
    }
    const auto loops = extractBoundaryLoops(mesh);
    EXPECT_TRUE(loops.size() == 2u);
    for (const auto& loop : loops) {
        EXPECT_TRUE(loop.totalLength > 0.0f);
        EXPECT_TRUE(loop.distance.size() == loop.vertexIds.size());
        EXPECT_TRUE(resampleBoundary(mesh, loop, 10).size() == 10u);
    }
    EXPECT_TRUE(analyzeTopology(mesh).nonManifoldEdges == 0);
}

PAE_TEST_CASE(p1b_cpu_normal_matrix_handles_nonuniform_scale) {
    MeshData mesh;
    Vertex vertex;
    vertex.position = {1.0f, 2.0f, 3.0f};
    vertex.normal = {1.0f, 1.0f, 1.0f};
    vertex.joints = {0, 0, 0, 0};
    vertex.weights = {1.0f, 0.0f, 0.0f, 0.0f};
    mesh.vertices.push_back(vertex);
    std::array<Mat4, BoneCount> identity{};
    const Mat4 model = Mat4::translation({0.25f, -0.5f, 0.75f}) *
                       Mat4::scaling({2.0f, 0.5f, 3.0f});
    const auto positions = cpuSkinPositions(mesh, identity, model);
    const auto normals = cpuSkinNormals(mesh, identity, model);
    const Vec3 expectedNormal = Vec3{0.5f, 2.0f, 1.0f / 3.0f}.normalized();
    EXPECT_NEAR(positions[0].x, 2.25f, 1e-5f);
    EXPECT_NEAR(positions[0].y, 0.5f, 1e-5f);
    EXPECT_NEAR(positions[0].z, 9.75f, 1e-5f);
    EXPECT_NEAR(normals[0].x, expectedNormal.x, 1e-5f);
    EXPECT_NEAR(normals[0].y, expectedNormal.y, 1e-5f);
    EXPECT_NEAR(normals[0].z, expectedNormal.z, 1e-5f);
}

PAE_TEST_CASE(p1b_render_mesh_uv_atlas_preserves_canonical_seams) {
    CanonicalSkinResult canonical;
    canonical.mesh.vertices = {
        {{0.0f, 0.0f, 0.0f}}, {{1.0f, 0.0f, 0.0f}},
        {{0.0f, 1.0f, 0.0f}}, {{1.0f, 1.0f, 0.0f}},
    };
    canonical.mesh.indices = {0, 1, 2, 1, 3, 2};
    const RenderResult render = buildRenderMesh(canonical);
    EXPECT_TRUE(render.valid);
    EXPECT_TRUE(render.mesh.vertices.size() == 6u);
    EXPECT_TRUE(render.splitVertices == 2u);
    EXPECT_TRUE(render.uv.charts == 2u);
    EXPECT_TRUE(render.renderToCanonical[0] == 0u);
    EXPECT_TRUE(render.renderToCanonical[3] == 1u);
    EXPECT_TRUE(render.renderToCanonical[2] == 2u);
    EXPECT_TRUE(render.renderToCanonical[4] == 3u);
    EXPECT_TRUE(render.renderToCanonical[5] == 2u);
    const Vec2& a = render.mesh.vertices[0].uv;
    const Vec2& b = render.mesh.vertices[3].uv;
    EXPECT_TRUE(a.x != b.x || a.y != b.y);
    EXPECT_TRUE(render.uv.minUvArea > 0.0f);
}

PAE_TEST_CASE(p1a2_self_intersection_disjoint_triangles) {
    MeshData mesh;
    mesh.vertices = {
        {{-1.0f, -1.0f, 0.0f}}, {{1.0f, -1.0f, 0.0f}}, {{0.0f, 1.0f, 0.0f}},
        {{-1.0f, -1.0f, 2.0f}}, {{1.0f, -1.0f, 2.0f}}, {{0.0f, 1.0f, 2.0f}},
    };
    mesh.indices = {0, 1, 2, 3, 4, 5};
    const SelfIntersectionReport report = detectSelfIntersections(mesh);
    EXPECT_TRUE(report.candidatePairs == 0u);
    EXPECT_TRUE(report.intersectingPairs == 0u);
}

PAE_TEST_CASE(p1a2_self_intersection_crossing_triangles) {
    MeshData mesh;
    mesh.vertices = {
        {{-1.0f, -1.0f, 0.0f}}, {{1.0f, -1.0f, 0.0f}}, {{0.0f, 1.0f, 0.0f}},
        {{0.0f, -0.5f, -1.0f}}, {{0.0f, 0.5f, -1.0f}}, {{0.0f, 0.0f, 1.0f}},
    };
    mesh.indices = {0, 1, 2, 3, 4, 5};
    const SelfIntersectionReport report = detectSelfIntersections(mesh);
    EXPECT_TRUE(report.candidatePairs == 1u);
    EXPECT_TRUE(report.intersectingPairs == 1u);
    EXPECT_TRUE(report.samples.size() == 1u);
}

PAE_TEST_CASE(p1a2_self_intersection_coplanar_overlap) {
    MeshData mesh;
    mesh.vertices = {
        {{-1.0f, -1.0f, 0.0f}}, {{1.0f, -1.0f, 0.0f}}, {{0.0f, 1.0f, 0.0f}},
        {{-0.5f, -0.5f, 0.0f}}, {{0.5f, -0.5f, 0.0f}}, {{0.0f, 0.5f, 0.0f}},
    };
    mesh.indices = {0, 1, 2, 3, 4, 5};
    const SelfIntersectionReport report = detectSelfIntersections(mesh);
    EXPECT_TRUE(report.candidatePairs == 1u);
    EXPECT_TRUE(report.intersectingPairs == 1u);
}

PAE_TEST_CASE(p1a2_self_intersection_coplanar_disjoint) {
    MeshData mesh;
    mesh.vertices = {
        {{-1.0f, -1.0f, 0.0f}}, {{1.0f, -1.0f, 0.0f}}, {{0.0f, 1.0f, 0.0f}},
        {{2.0f, -1.0f, 0.0f}}, {{4.0f, -1.0f, 0.0f}}, {{3.0f, 1.0f, 0.0f}},
    };
    mesh.indices = {0, 1, 2, 3, 4, 5};
    const SelfIntersectionReport report = detectSelfIntersections(mesh);
    EXPECT_TRUE(report.candidatePairs == 0u);
    EXPECT_TRUE(report.intersectingPairs == 0u);
}

PAE_TEST_CASE(p1a2_self_intersection_small_parallel_triangles_use_metric_distance) {
    MeshData mesh;
    const Vec3 a{-0.0001f, 0.0f, 0.0001f};
    const Vec3 b{0.0001f, 0.0f, -0.0001f};
    const Vec3 c{0.0f, 0.0001f, -0.0001f};
    const Vec3 normal = (b - a).cross(c - a).normalized();
    const Vec3 offset = normal * 0.00001f;
    mesh.vertices = {Vertex{a}, Vertex{b}, Vertex{c},
                     Vertex{a + offset}, Vertex{b + offset}, Vertex{c + offset}};
    mesh.indices = {0, 1, 2, 3, 4, 5};
    const SelfIntersectionReport report = detectSelfIntersections(mesh);
    EXPECT_TRUE(report.candidatePairs == 1u);
    EXPECT_TRUE(report.intersectingPairs == 0u);
}

PAE_TEST_CASE(p1a2_self_intersection_excludes_shared_vertices) {
    MeshData mesh;
    mesh.vertices = {
        {{0.0f, 0.0f, 0.0f}}, {{1.0f, 0.0f, 0.0f}},
        {{0.0f, 1.0f, 0.0f}}, {{0.0f, -1.0f, 0.0f}},
    };
    mesh.indices = {0, 1, 2, 1, 0, 3};
    const SelfIntersectionReport report = detectSelfIntersections(mesh);
    EXPECT_TRUE(report.candidatePairs == 1u);
    EXPECT_TRUE(report.intersectingPairs == 0u);
}

PAE_TEST_CASE(p1a2_self_intersection_detects_shared_edge_coplanar_overlap) {
    for (float scale : {1e-4f, 1.0f, 1e4f}) {
        MeshData mesh;
        mesh.vertices = {
            {{0.0f, 0.0f, 0.0f}}, {{1.0f, 0.0f, 0.0f}},
            {{0.0f, 1.0f, 0.0f}}, {{0.5f, 0.5f, 0.0f}},
        };
        for (Vertex& vertex : mesh.vertices) vertex.position = vertex.position * scale;
        mesh.indices = {0, 1, 2, 0, 1, 3};
        const SelfIntersectionReport report = detectSelfIntersections(mesh);
        EXPECT_TRUE(report.candidatePairs == 1u);
        EXPECT_TRUE(report.intersectingPairs == 1u);
    }
}

PAE_TEST_CASE(p1a2_self_intersection_coplanar_overlap_is_scale_and_translation_invariant) {
    for (float scale : {1e-7f, 1e-4f, 1.0f, 1e4f}) {
        for (Vec3 offset : {Vec3{}, scale >= 1e-4f ? Vec3{1000.0f, 1000.0f, 0.0f} : Vec3{}}) {
            MeshData mesh;
            mesh.vertices = {
                {{-1.0f, -1.0f, 0.0f}}, {{1.0f, -1.0f, 0.0f}}, {{0.0f, 1.0f, 0.0f}},
                {{-0.5f, -0.5f, 0.0f}}, {{0.5f, -0.5f, 0.0f}}, {{0.0f, 0.5f, 0.0f}},
            };
            for (Vertex& vertex : mesh.vertices) vertex.position = vertex.position * scale + offset;
            mesh.indices = {0, 1, 2, 3, 4, 5};
            const SelfIntersectionReport report = detectSelfIntersections(mesh);
            EXPECT_TRUE(report.candidatePairs == 1u);
            EXPECT_TRUE(report.intersectingPairs == 1u);
        }
    }
}

PAE_TEST_CASE(p1a2_self_intersection_crossing_is_scale_invariant) {
    for (float scale : {1e-7f, 1e-4f, 1.0f, 1e4f}) {
        MeshData mesh;
        mesh.vertices = {
            {{-1.0f, -1.0f, 0.0f}}, {{1.0f, -1.0f, 0.0f}}, {{0.0f, 1.0f, 0.0f}},
            {{0.0f, -0.5f, -1.0f}}, {{0.0f, 0.5f, -1.0f}}, {{0.0f, 0.0f, 1.0f}},
        };
        for (Vertex& vertex : mesh.vertices) vertex.position = vertex.position * scale;
        mesh.indices = {0, 1, 2, 3, 4, 5};
        const SelfIntersectionReport report = detectSelfIntersections(mesh);
        EXPECT_TRUE(report.candidatePairs == 1u);
        EXPECT_TRUE(report.intersectingPairs == 1u);
    }
}

PAE_TEST_CASE(p1_surface_nets_quad_area_threshold_is_scale_invariant) {
    size_t referenceIndices = 0;
    for (float scale : {1e-4f, 1.0f, 1e4f}) {
        fusion::FusionVolume volume;
        volume.size = Vec3{2.4f, 2.4f, 2.4f} * scale;
        volume.resolution = {20, 20, 20};
        const float radius = 0.5f * scale;
        const fusion::SdfField field = [radius](Vec3 point) {
            return fusion::sdSphere(point, Vec3{}, radius);
        };
        const MeshData mesh = fusion::extractSurfaceNets(volume, field, nullptr, true);
        EXPECT_TRUE(!mesh.indices.empty());
        if (scale == 1e-4f) referenceIndices = mesh.indices.size();
        EXPECT_TRUE(mesh.indices.size() == referenceIndices);
    }
    EXPECT_TRUE(referenceIndices > 0u);
}

PAE_TEST_CASE(p1_surface_nets_quad_area_threshold_ignores_unused_volume_margin) {
    for (Vec3 size : {Vec3{10000.0f, 2.4f, 2.4f}, Vec3{2.4f, 2.4f, 10000.0f}}) {
        fusion::FusionVolume volume;
        volume.size = size;
        volume.resolution = {20, 20, 20};
        const fusion::SdfField field = [](Vec3 point) {
            return fusion::sdSphere(point, Vec3{}, 0.5f);
        };
        const MeshData mesh = fusion::extractSurfaceNets(volume, field, nullptr, true);
        EXPECT_TRUE(!mesh.indices.empty());
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            const Vec3 a = mesh.vertices[mesh.indices[i]].position;
            const Vec3 b = mesh.vertices[mesh.indices[i + 1]].position;
            const Vec3 c = mesh.vertices[mesh.indices[i + 2]].position;
            EXPECT_TRUE((b - a).cross(c - a).lengthSq() > 0.0f);
        }
    }
}

PAE_TEST_CASE(p1a2_adult_landmarks_keep_shoulders_and_hands_clear) {
    const BodyParameters params = clampBodyParameters(BodyParameters{});
    const float estimatedHeight = params.upperLegLength + params.lowerLegLength + 0.08f +
                                  params.torsoLength + params.neckLength + params.headHeight;
    const float scale = params.height / estimatedHeight;
    const BodyJoints joints = deriveJoints(params, scale);
    EXPECT_TRUE(joints.leftShoulder.y < joints.neck.y - 0.04f * scale);
    EXPECT_TRUE(params.headHeight / params.height > 0.12f);
    EXPECT_TRUE(params.headHeight / params.height < 0.15f);
    EXPECT_TRUE(joints.leftWrist.x > joints.leftHip.x + 0.15f * scale);
    const float handInner = joints.leftHand.x - 0.044f * scale;
    const float thighOuter = joints.leftHip.x +
        params.legRadius * scale * params.legShape.thighVolume;
    EXPECT_TRUE(handInner - thighOuter > 0.05f * scale);
}

PAE_TEST_CASE(p1a2_full_body_is_one_closed_canonical_shell) {
    BodyParameters params;
    params.detailLevel = 0;
    const ContinuousSkinResult result = generateContinuousSkin(params, false);
    const TopologyReport& topology = result.canonical.topology;
    std::fprintf(stderr,
                 "P1A2-TOPO verts=%zu tris=%ld components=%d boundary=%d nonmanifold=%d "
                 "degen=%d dup=%d badnorm=%d weights_bad=%zu intersections=%zu total_ms=%.1f\n",
                 topology.vertices, topology.triangles, topology.components,
                 topology.boundaryEdges, topology.nonManifoldEdges, topology.degenerateTriangles,
                 topology.duplicateVertices, topology.invalidNormals,
                 result.canonical.weights.invalidVertices,
                 result.canonical.selfIntersections.intersectingPairs, result.generationMs);
    EXPECT_TRUE(topology.vertices > 1000u);
    EXPECT_TRUE(topology.components == 1);
    EXPECT_TRUE(topology.boundaryEdges == 0);
    EXPECT_TRUE(topology.nonManifoldEdges == 0);
    EXPECT_TRUE(topology.degenerateTriangles == 0);
    EXPECT_TRUE(topology.duplicateVertices == 0);
    EXPECT_TRUE(topology.invalidNormals == 0);
    EXPECT_TRUE(result.canonical.weights.invalidVertices == 0u);
    EXPECT_TRUE(result.canonical.selfIntersections.intersectingPairs == 0u);
    EXPECT_TRUE(result.canonical.valid);
    EXPECT_TRUE(result.canonical.boundaryLoops.empty());
    EXPECT_TRUE(result.canonical.mesh.bounds.max.y > result.joints.head.y);
    EXPECT_TRUE(result.canonical.mesh.bounds.min.y < 0.06f);

    std::array<Quat, BoneCount> identity{};
    const PoseResult bindPose = computePose(result.skeleton, identity);
    const std::vector<Vec3> bindPositions = cpuSkinPositions(result.canonical.mesh, bindPose.palette);
    float maxBindError = 0.0f;
    for (size_t i = 0; i < bindPositions.size(); ++i) {
        maxBindError = std::fmax(maxBindError,
                                 (bindPositions[i] - result.canonical.mesh.vertices[i].position).length());
    }
    EXPECT_TRUE(maxBindError < 1e-4f);

    size_t conflictingInfluences = 0;
    for (const Vertex& vertex : result.canonical.mesh.vertices) {
        bool leftLimb = false;
        bool rightLimb = false;
        bool arm = false;
        bool leg = false;
        const uint16_t joints[4] = {vertex.joints.x, vertex.joints.y,
                                   vertex.joints.z, vertex.joints.w};
        for (uint16_t joint : joints) {
            if (joint == BoneLeftShoulder || joint == BoneLeftUpperArm ||
                joint == BoneLeftLowerArm || joint == BoneLeftHand ||
                joint == BoneLeftUpperLeg || joint == BoneLeftLowerLeg ||
                joint == BoneLeftFoot || joint == BoneLeftToe) leftLimb = true;
            if (joint == BoneRightShoulder || joint == BoneRightUpperArm ||
                joint == BoneRightLowerArm || joint == BoneRightHand ||
                joint == BoneRightUpperLeg || joint == BoneRightLowerLeg ||
                joint == BoneRightFoot || joint == BoneRightToe) rightLimb = true;
            if (joint == BoneLeftShoulder || joint == BoneLeftUpperArm ||
                joint == BoneLeftLowerArm || joint == BoneLeftHand ||
                joint == BoneRightShoulder || joint == BoneRightUpperArm ||
                joint == BoneRightLowerArm || joint == BoneRightHand) arm = true;
            if (joint == BoneLeftUpperLeg || joint == BoneLeftLowerLeg ||
                joint == BoneLeftFoot || joint == BoneLeftToe ||
                joint == BoneRightUpperLeg || joint == BoneRightLowerLeg ||
                joint == BoneRightFoot || joint == BoneRightToe) leg = true;
        }
        if ((leftLimb && rightLimb) || (arm && leg)) ++conflictingInfluences;
    }
    std::fprintf(stderr, "P1A2-WEIGHTS conflicting_influences=%zu\n", conflictingInfluences);
    EXPECT_TRUE(conflictingInfluences == 0u);

    std::array<Quat, BoneCount> rotations{};
    rotations[BoneLeftUpperArm] = Quat{0.0f, 0.0f, std::sin(0.5f), std::cos(0.5f)};
    rotations[BoneLeftLowerLeg] = Quat{0.0f, 0.0f, std::sin(-0.3f), std::cos(-0.3f)};
    const PoseResult posed = computePose(result.skeleton, rotations);
    const std::vector<Vec3> posedPositions = cpuSkinPositions(result.canonical.mesh, posed.palette);
    const std::vector<Vec3> posedNormals = cpuSkinNormals(result.canonical.mesh, posed.palette);
    size_t moved = 0;
    float maxDisplacement = 0.0f;
    for (size_t i = 0; i < posedPositions.size(); ++i) {
        const Vec3& position = posedPositions[i];
        EXPECT_TRUE(std::isfinite(position.x) && std::isfinite(position.y) && std::isfinite(position.z));
        EXPECT_NEAR(posedNormals[i].lengthSq(), 1.0f, 1e-3f);
        const float displacement = (position - result.canonical.mesh.vertices[i].position).length();
        maxDisplacement = std::fmax(maxDisplacement, displacement);
        if (displacement > 0.01f) ++moved;
    }
    std::fprintf(stderr, "P1A2-POSE moved=%zu max_displacement=%.5f\n", moved, maxDisplacement);
    EXPECT_TRUE(moved > 100u);
    EXPECT_TRUE(maxDisplacement < 0.75f);

    const auto boneWeight = [](const Vertex& vertex, uint16_t bone) {
        const uint16_t joints[4] = {vertex.joints.x, vertex.joints.y,
                                   vertex.joints.z, vertex.joints.w};
        const float weights[4] = {vertex.weights.x, vertex.weights.y,
                                  vertex.weights.z, vertex.weights.w};
        float value = 0.0f;
        for (size_t slot = 0; slot < 4; ++slot) {
            if (joints[slot] == bone) value += weights[slot];
        }
        return value;
    };
    std::array<Quat, BoneCount> legRotation{};
    legRotation[BoneLeftLowerLeg] = rotations[BoneLeftLowerLeg];
    const PoseResult legPose = computePose(result.skeleton, legRotation);
    const std::vector<Vec3> legPositions = cpuSkinPositions(result.canonical.mesh, legPose.palette);
    float handMovementFromLeg = 0.0f;
    size_t handRegionVertices = 0;
    for (size_t i = 0; i < legPositions.size(); ++i) {
        const Vertex& vertex = result.canonical.mesh.vertices[i];
        const Vec3& point = vertex.position;
        if (point.x > result.joints.leftWrist.x - 0.025f && point.y < result.joints.leftWrist.y &&
            boneWeight(vertex, BoneLeftHand) > 0.50f) {
            ++handRegionVertices;
            handMovementFromLeg = std::fmax(handMovementFromLeg,
                (legPositions[i] - point).length());
        }
    }
    std::array<Quat, BoneCount> armRotation{};
    armRotation[BoneLeftUpperArm] = rotations[BoneLeftUpperArm];
    const PoseResult armPose = computePose(result.skeleton, armRotation);
    const std::vector<Vec3> armPositions = cpuSkinPositions(result.canonical.mesh, armPose.palette);
    float thighMovementFromArm = 0.0f;
    float oppositeArmMovement = 0.0f;
    float headMovement = 0.0f;
    size_t thighRegionVertices = 0;
    size_t oppositeArmVertices = 0;
    size_t posedHeadVertices = 0;
    for (size_t i = 0; i < armPositions.size(); ++i) {
        const Vertex& vertex = result.canonical.mesh.vertices[i];
        const Vec3& point = vertex.position;
        if (point.y > result.joints.leftKnee.y && point.y < result.joints.leftHip.y + 0.04f &&
            std::fabs(point.x - result.joints.leftHip.x) < 0.13f &&
            boneWeight(vertex, BoneLeftUpperLeg) > 0.50f) {
            ++thighRegionVertices;
            thighMovementFromArm = std::fmax(thighMovementFromArm,
                (armPositions[i] - point).length());
        }
        if (point.x < result.joints.rightWrist.x - 0.02f && point.y > result.joints.rightWrist.y &&
            (boneWeight(vertex, BoneRightShoulder) + boneWeight(vertex, BoneRightUpperArm) +
             boneWeight(vertex, BoneRightLowerArm) + boneWeight(vertex, BoneRightHand)) > 0.50f) {
            ++oppositeArmVertices;
            oppositeArmMovement = std::fmax(oppositeArmMovement,
                (armPositions[i] - point).length());
        }
        if (point.y > result.joints.head.y + 0.08f && boneWeight(vertex, BoneHead) > 0.50f) {
            ++posedHeadVertices;
            headMovement = std::fmax(headMovement, (armPositions[i] - point).length());
        }
    }
    std::fprintf(stderr,
                 "P1A2-ISOLATION hand=%zu thigh=%zu opposite=%zu head=%zu "
                 "hand_from_leg=%.7f thigh_from_arm=%.7f opposite_arm=%.7f head=%.7f\n",
                 handRegionVertices, thighRegionVertices, oppositeArmVertices, posedHeadVertices,
                 handMovementFromLeg, thighMovementFromArm, oppositeArmMovement, headMovement);
    EXPECT_TRUE(handRegionVertices > 20u);
    EXPECT_TRUE(thighRegionVertices > 100u);
    EXPECT_TRUE(oppositeArmVertices > 50u);
    EXPECT_TRUE(posedHeadVertices > 100u);
    EXPECT_TRUE(handMovementFromLeg < 1e-4f);
    EXPECT_TRUE(thighMovementFromArm < 1e-4f);
    EXPECT_TRUE(oppositeArmMovement < 1e-4f);
    EXPECT_TRUE(headMovement < 1e-4f);

    size_t headVertices = 0;
    size_t headFailures = 0;
    float minHeadWeight = 1.0f;
    for (const Vertex& vertex : result.canonical.mesh.vertices) {
        if (vertex.position.y <= result.joints.head.y + 0.08f) continue;
        ++headVertices;
        const uint16_t joints[4] = {vertex.joints.x, vertex.joints.y, vertex.joints.z, vertex.joints.w};
        const float weights[4] = {vertex.weights.x, vertex.weights.y, vertex.weights.z, vertex.weights.w};
        float headWeight = 0.0f;
        for (size_t slot = 0; slot < 4; ++slot) {
            if (joints[slot] == BoneHead) headWeight += weights[slot];
        }
        minHeadWeight = std::fmin(minHeadWeight, headWeight);
        if (headWeight <= 0.50f) ++headFailures;
    }
    std::fprintf(stderr, "P1A2-HEAD vertices=%zu failures=%zu min_weight=%.4f\n",
                 headVertices, headFailures, minHeadWeight);
    EXPECT_TRUE(headVertices > 100u);
    EXPECT_TRUE(headFailures == 0u);
}

PAE_TEST_CASE(p1a2_quality_levels_are_closed) {
    for (uint32_t detail = 0; detail <= 2; ++detail) {
        BodyParameters params;
        params.detailLevel = detail;
        const ContinuousSkinResult result = generateContinuousSkin(params, false);
        const TopologyReport& topology = result.canonical.topology;
        std::fprintf(stderr,
                     "P1A2-QUALITY detail=%u verts=%zu tris=%ld components=%d boundary=%d "
                     "nonmanifold=%d degen=%d intersections=%zu total_ms=%.1f holes=%zu\n",
                     detail, topology.vertices, topology.triangles, topology.components,
                     topology.boundaryEdges, topology.nonManifoldEdges,
                     topology.degenerateTriangles, result.canonical.selfIntersections.intersectingPairs,
                     result.generationMs, result.repairedBoundaryHoles);
        for (const Vec3& point : topology.boundarySamples) {
            std::fprintf(stderr, "P1A2-BOUNDARY detail=%u at=(%.6f,%.6f,%.6f)\n",
                         detail, point.x, point.y, point.z);
        }
        const SelfIntersectionReport& intersections = result.canonical.selfIntersections;
        std::fprintf(stderr,
                     "P1A2-INTERSECTIONS detail=%u candidates=%zu intersecting=%zu samples=%zu\n",
                     detail, intersections.candidatePairs, intersections.intersectingPairs,
                     intersections.samples.size());
        for (const auto& sample : intersections.samples) {
            std::fprintf(stderr, "P1A2-INTERSECTION detail=%u triangles=(%u,%u)\n",
                         detail, sample[0], sample[1]);
        }
        const RenderResult render = buildRenderMesh(result.canonical);
        std::fprintf(stderr,
                     "P1B-UV detail=%u charts=%zu atlas=%zux%zu render_verts=%zu split=%zu "
                     "invalid=%zu out_of_range=%zu zero_area=%zu min_area=%.9g max_area=%.9g\n",
                     detail, render.uv.charts, render.uv.atlasColumns, render.uv.atlasRows,
                     render.uv.renderVertices, render.splitVertices, render.uv.invalidUvVertices,
                     render.uv.outOfRangeUvVertices, render.uv.zeroAreaUvTriangles,
                     render.uv.minUvArea, render.uv.maxUvArea);
        EXPECT_TRUE(result.canonical.mesh.subMeshes.size() == 1u);
        EXPECT_TRUE(result.canonical.mesh.subMeshes.front().indexCount ==
                    result.canonical.mesh.indices.size());
        EXPECT_TRUE(topology.components == 1);
        EXPECT_TRUE(topology.boundaryEdges == 0);
        EXPECT_TRUE(topology.nonManifoldEdges == 0);
        EXPECT_TRUE(topology.degenerateTriangles == 0);
        EXPECT_TRUE(topology.invalidNormals == 0);
        EXPECT_TRUE(result.canonical.weights.invalidVertices == 0u);
        EXPECT_TRUE(intersections.intersectingPairs == 0u);
        EXPECT_TRUE(result.canonical.valid);
        EXPECT_TRUE(render.valid);
        EXPECT_TRUE(render.renderToCanonical.size() == render.mesh.vertices.size());
        EXPECT_TRUE(render.uv.charts == static_cast<size_t>(topology.triangles));
        EXPECT_TRUE(render.uv.invalidUvVertices == 0u);
        EXPECT_TRUE(render.uv.outOfRangeUvVertices == 0u);
        EXPECT_TRUE(render.uv.zeroAreaUvTriangles == 0u);
        if (detail > 0) {
            size_t fingerVertices = 0;
            size_t toeVertices = 0;
            float faceFront = -1.0f;
            for (const Vertex& vertex : result.canonical.mesh.vertices) {
                if (vertex.position.x > result.joints.leftWrist.x - 0.03f &&
                    vertex.position.x < result.joints.leftHand.x + 0.08f &&
                    vertex.position.y < result.joints.leftHand.y - 0.070f) ++fingerVertices;
                if (vertex.position.z > result.joints.leftToe.z + 0.010f &&
                    vertex.position.y < result.joints.leftAnkle.y) ++toeVertices;
                if (vertex.position.y > result.joints.head.y - 0.08f &&
                    std::fabs(vertex.position.x) < 0.09f) {
                    faceFront = std::fmax(faceFront, vertex.position.z);
                }
            }
            std::fprintf(stderr, "P1A2-DETAIL detail=%u fingers=%zu toes=%zu face_front=%.5f\n",
                         detail, fingerVertices, toeVertices, faceFront);
            EXPECT_TRUE(fingerVertices > 20u);
            EXPECT_TRUE(toeVertices > 20u);
            EXPECT_TRUE(faceFront > result.joints.head.z + 0.070f);
        }
    }
}

int main() { return test::runAllTests(); }
