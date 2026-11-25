#include "HumanAnatomyGenerator.h"

#include "core/diagnostics/Log.h"
#include "procedural/fusion/SdfFusion.h"
#include "procedural/MeshTopology.h"
#include "skeleton/Skeleton.h"
#include "skeleton/weights/WeightGenerator.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>

namespace pae::anatomy {

namespace {

struct RegionInfo {
    BodyRegion region;
    float field;
};

} // namespace

AnatomyResult generateAnatomyUpperBody(const HumanAnatomyParameters& params,
                                       const BodyParameters& bodyParams,
                                       const AnatomyResolution& resolution) {
    const auto genStart = std::chrono::steady_clock::now();
    AnatomyResult result;

    // Skeleton at the standard bind landmarks (M11 derivation, 1.7m figure).
    const float estimatedHeight = bodyParams.upperLegLength + bodyParams.lowerLegLength +
                                  0.08f + bodyParams.torsoLength + bodyParams.neckLength +
                                  bodyParams.headHeight;
    const float scale = std::clamp(bodyParams.height / std::fmax(estimatedHeight, 0.1f), 0.5f, 1.8f);
    result.joints = deriveJoints(bodyParams, scale);
    result.skeleton = buildHumanoidSkeleton(result.joints);
    const BodyJoints& j = result.joints;

    // Field over the body volume (covers hips -> head top, arms to wrists).
    // M13: the primitive set is resolved once and captured, so the hundreds of
    // thousands of samples do not each rebuild the joint-anchored primitives.
    auto field = makeUpperBodyField(params, j);

    fusion::FusionVolume volume;
    // Anisotropic volume: the torso is far wider than it is deep, so the box
    // and the cell counts are set per axis. X covers the arms out to the
    // wrists, Y the head-top-to-hips span, Z the torso depth plus margin.
    const float extentY = (j.head.y + 0.14f) - (j.hips.y - 0.10f);
    const float sizeX = (2.0f * (j.leftWrist.x + 0.10f));
    const float sizeZ = 0.40f;
    volume.center = Vec3{0.0f, (j.head.y + 0.14f + j.hips.y - 0.10f) * 0.5f, 0.0f};
    volume.size = Vec3{sizeX, extentY + 0.30f, sizeZ};
    // Cells per axis, floored so no axis degenerates; X/Y/Z are honoured
    // independently (a scalar resolution cannot express a body-shaped grid).
    volume.resolution = {std::max(16, resolution.x), std::max(24, resolution.y),
                         std::max(12, resolution.z)};

    MeshData mesh = fusion::extractSurfaceNets(volume, field, &result.surfaceQuads, true);
    if (mesh.vertices.empty() || mesh.indices.empty()) {
        PAE_LOGE("PAE_GEOMETRY", "anatomy extraction produced no geometry");
        return result;
    }

    // Semantic weights from the region fields: primary bone = the region that
    // owns the vertex (smallest field), secondary = the region's hierarchy
    // neighbour. The distance-based WeightGenerator then refines against the
    // bone segments for a smooth final distribution.
    for (Vertex& vertex : mesh.vertices) {
        auto fields = regionFields(params, j, vertex.position);
        // Find the owning region (smallest field) and the runner-up.
        size_t owner = 0;
        float ownerField = fields[0];
        for (size_t r = 1; r < fields.size(); ++r) {
            if (fields[r] < ownerField) {
                ownerField = fields[r];
                owner = r;
            }
        }
        const int side = vertex.position.x >= 0.0f ? 1 : -1;
        const BodyRegion region = static_cast<BodyRegion>(owner);
        const uint16_t primary = regionBone(region, side);

        // Secondary: the skeleton bone segment nearest to the vertex among the
        // upper-body set, excluding the primary.
        float bestDistSq = std::numeric_limits<float>::max();
        uint16_t secondBone = primary;
        for (uint16_t bone : {BoneHips, BoneSpine, BoneChest, BoneNeck, BoneHead,
                              BoneLeftShoulder, BoneRightShoulder,
                              BoneLeftUpperArm, BoneRightUpperArm,
                              BoneLeftLowerArm, BoneRightLowerArm}) {
            if (bone == primary) continue;
            const BoneDefinition& def = result.skeleton.bones[bone];
            const Vec3 ab = def.tail - def.head;
            const float l2 = ab.lengthSq();
            const float h = l2 > 1e-10f
                ? std::clamp((vertex.position - def.head).dot(ab) / l2, 0.0f, 1.0f) : 0.0f;
            const float distSq = (vertex.position - (def.head + ab * h)).lengthSq();
            if (distSq < bestDistSq) {
                bestDistSq = distSq;
                secondBone = bone;
            }
        }
        const Vec3 ab2 = result.skeleton.bones[secondBone].tail -
                         result.skeleton.bones[secondBone].head;
        const float l22 = ab2.lengthSq();
        const float h2 = l22 > 1e-10f
            ? std::clamp((vertex.position - result.skeleton.bones[secondBone].head).dot(ab2) / l22,
                         0.0f, 1.0f) : 0.0f;
        const float distSq2 = (vertex.position -
            (result.skeleton.bones[secondBone].head + ab2 * h2)).lengthSq();
        const float w0 = 1.0f / (ownerField * ownerField + 0.0004f);
        const float w1 = 1.0f / (distSq2 + 0.0004f);
        const float sum = w0 + w1;
        vertex.joints = {primary, secondBone, 0, 0};
        vertex.weights = {w0 / sum, w1 / sum, 0.0f, 0.0f};
        vertex.color = Vec4{1.0f, 1.0f, 1.0f, 0.0f}; // skin material
    }

    // Human Anatomy Report (M12 §9).
    const auto genEnd = std::chrono::steady_clock::now();
    const double genMs = std::chrono::duration<double, std::milli>(genEnd - genStart).count();
    const TopologyReport topology = analyzeTopology(mesh);
    result.mesh = std::move(mesh);
    // The acceptance criteria for the body are a single watertight, manifold
    // surface. The mesh is still returned when the topology is not clean (the
    // preview stays usable), but the failure is reported loudly and the
    // result carries the flag, so a pretty geometry report can no longer
    // hide a mesh that does not meet the acceptance criteria.
    result.topologyClean = topology.components == 1 &&
                            topology.boundaryEdges == 0 &&
                            topology.nonManifoldEdges == 0 &&
                            topology.invalidNormals == 0;
    result.valid = result.mesh.vertices.size() > 500 && topology.components == 1;
    if (!result.topologyClean) {
        PAE_LOGE("PAE_GEOMETRY",
                 "anatomy topology NOT clean: components=%d boundary=%d nonmanifold=%d "
                 "badnorm=%d (P1 blocker)",
                 topology.components, topology.boundaryEdges, topology.nonManifoldEdges,
                 topology.invalidNormals);
    }

    float chestWidthSample = 0.0f, waistWidthSample = 0.0f, shoulderSample = 0.0f, pelvisSample = 0.0f;
    for (const Vertex& v : result.mesh.vertices) {
        const float radial = std::sqrt(v.position.x * v.position.x + v.position.z * v.position.z);
        if (std::fabs(v.position.y - (j.chest.y + (j.spine.y - j.chest.y) * 0.55f)) < 0.03f)
            chestWidthSample = std::fmax(chestWidthSample, radial);
        if (std::fabs(v.position.y - j.hips.y - 0.03f) < 0.02f)
            waistWidthSample = std::fmax(waistWidthSample, radial);
        if (std::fabs(v.position.y - j.leftShoulder.y) < 0.03f)
            shoulderSample = std::fmax(shoulderSample, v.position.x);
        if (std::fabs(v.position.y - j.hips.y) < 0.02f)
            pelvisSample = std::fmax(pelvisSample, radial);
    }

    PAE_LOGI("PAE_GEOMETRY",
             "Human Anatomy Report: verts=%u tris=%ld components=%d boundary=%d "
             "nonmanifold=%d degen=%d dup=%d badnorm=%d gen_ms=%.0f res=%dx%dx%d | "
             "chest=%.3f waist=%.3f shoulder_x=%.3f pelvis=%.3f",
             static_cast<unsigned>(result.mesh.vertices.size()), topology.triangles,
             topology.components, topology.boundaryEdges, topology.nonManifoldEdges,
             topology.degenerateTriangles, topology.duplicateVertices,
             topology.invalidNormals, genMs, resolution.x, resolution.y, resolution.z,
             chestWidthSample * 2.0f, waistWidthSample * 2.0f, shoulderSample,
             pelvisSample * 2.0f);
    return result;
}

} // namespace pae::anatomy
