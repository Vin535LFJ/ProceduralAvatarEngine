#include "mini_test.hpp"

#include "procedural/MeshTopology.h"
#include "procedural/MeshValidator.h"
#include "procedural/humanoid/FaceGenerator.h"
#include "procedural/humanoid/HairGenerator.h"
#include "procedural/humanoid/HeadAnatomy.h"
#include "procedural/humanoid/Humanoid.h"

#include <array>
#include <cmath>
#include <functional>
#include <map>
#include <vector>
#include <cstring>

using namespace pae;

namespace {

face::FaceParams defaultFace() { return face::FaceParams{}; }

head::HeadParams defaultHead() {
    head::HeadParams hp{};
    hp.width = 0.168f;
    hp.height = 0.235f;
    hp.depth = 0.235f * 0.80f;
    hp.neckRadius = 0.055f;
    return hp;
}

const Vec3 kHeadCenter{0.0f, 1.64f, 0.0f};
constexpr float kNeckBaseY = 1.452f;

// Nearest-vertex distance from a point to a mesh, over a subsample.
float nearestVertexDistance(const MeshData& mesh, const Vec3& point, size_t stride) {
    float best = 1e9f;
    for (size_t i = 0; i < mesh.vertices.size(); i += stride) {
        const float d = (mesh.vertices[i].position - point).length();
        if (d < best) best = d;
    }
    return best;
}

} // namespace

// R4: the head is a closed, continuous surface. The previous lat/long
// ellipsoid could not be — it converged to a pole at the chin and carried
// the face relief as a floating overlay.
PAE_TEST_CASE(head_surface_is_closed_and_continuous) {
    const head::HeadParams hp = defaultHead();
    head::HeadLandmarks landmarks;
    const MeshData head = head::generateHeadMesh(hp, kHeadCenter, kNeckBaseY, &landmarks, 0);
    EXPECT_TRUE(head.vertices.size() > 1000);
    EXPECT_TRUE(head.indices.size() > 3000);

    const TopologyReport topology = analyzeTopology(head);
    EXPECT_TRUE(topology.boundaryEdges == 0);
    EXPECT_TRUE(topology.nonManifoldEdges == 0);
    // Surface Nets emits a few zero-area faces where two surface patches meet
    // at a sharp angle. They are structurally required (removing them opens
    // the surface), so the gate is a ratio rather than an exact zero.
    EXPECT_TRUE(topology.degenerateTriangles * 2000 <
                static_cast<int>(topology.triangles));
    EXPECT_TRUE(topology.invalidNormals == 0);
}

// R4: the head keeps the depth it is asked for. The old implementation
// derived the depth from the head *width* (0.42 * width), which produced a
// flattened slab roughly 25% too shallow for an adult skull.
PAE_TEST_CASE(head_keeps_requested_depth_and_width) {
    const head::HeadParams hp = defaultHead();
    const MeshData head = head::generateHeadMesh(hp, kHeadCenter, kNeckBaseY, nullptr, 0);

    float minZ = 1e9f, maxZ = -1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
    for (const Vertex& v : head.vertices) {
        minZ = std::fmin(minZ, v.position.z);
        maxZ = std::fmax(maxZ, v.position.z);
        maxX = std::fmax(maxX, std::fabs(v.position.x));
        minY = std::fmin(minY, v.position.y);
        maxY = std::fmax(maxY, v.position.y);
    }
    const float depth = maxZ - minZ;
    // The nose projects past the glabella, so the measured depth is slightly
    // larger than the skull depth; it must never be smaller.
    EXPECT_TRUE(depth >= hp.depth * 0.95f);
    EXPECT_TRUE(depth <= hp.depth * 1.45f);
    // The skull is wider than the requested half width by at most the ear.
    EXPECT_TRUE(maxX >= hp.width * 0.45f);
    EXPECT_TRUE(maxX <= hp.width * 0.70f);
    // The surface spans the neck down to the shoulder ring.
    EXPECT_TRUE(minY <= kNeckBaseY + 0.005f);
    EXPECT_TRUE(maxY >= kHeadCenter.y + hp.height * 0.45f);
}

// R4: the jaw is a jaw, not a cone. A latitude/longitude head tapers to a
// point at the bottom pole; a real head still has a measurable width where
// the chin sits.
PAE_TEST_CASE(chin_is_not_a_cone) {
    const head::HeadParams hp = defaultHead();
    const MeshData head = head::generateHeadMesh(hp, kHeadCenter, kNeckBaseY, nullptr, 0);

    float widest = 0.0f;
    for (const Vertex& v : head.vertices) widest = std::fmax(widest, std::fabs(v.position.x));

    // Width in the band that contains the chin (0.80-0.95 of the head height
    // below the crown).
    const float chinBandTop = kHeadCenter.y - hp.height * 0.42f;
    const float chinBandBottom = kHeadCenter.y - hp.height * 0.52f;
    float chinWidth = 0.0f;
    int chinSamples = 0;
    for (const Vertex& v : head.vertices) {
        if (v.position.y < chinBandBottom || v.position.y > chinBandTop) continue;
        chinWidth = std::fmax(chinWidth, std::fabs(v.position.x));
        ++chinSamples;
    }
    EXPECT_TRUE(chinSamples > 0);
    EXPECT_TRUE(chinWidth > widest * 0.18f);
}

// R4: every facial feature is anchored on the extracted surface. The old
// code placed the eyes at rz * 1.12 regardless of the actual surface depth,
// which floated them up to 15 mm in front of the face.
PAE_TEST_CASE(face_features_sit_on_the_head_surface) {
    const head::HeadParams hp = defaultHead();
    const face::FaceParams fp{};
    head::HeadLandmarks landmarks;
    const MeshData head = head::generateHeadMesh(hp, kHeadCenter, kNeckBaseY, &landmarks, 0);

    for (int i = 0; i < 2; ++i) {
        const float distance = nearestVertexDistance(head, landmarks.eyeCenter[i], 1);
        EXPECT_TRUE(distance < 0.0065f);
    }
    EXPECT_TRUE(nearestVertexDistance(head, landmarks.noseTip, 1) < 0.0065f);
    EXPECT_TRUE(nearestVertexDistance(head, landmarks.mouthCenter, 1) < 0.0065f);

    // The sclera must not bulge out of the socket: the front of the eye
    // piece stays behind the projected surface point.
    const auto pieces = face::generateFaceFeatures(fp, landmarks, 14);
    int scleraPieces = 0;
    for (const auto& piece : pieces) {
        if (piece.materialId != 3) continue;
        ++scleraPieces;
        float maxZ = -1e9f;
        for (const Vertex& v : piece.mesh.vertices) maxZ = std::fmax(maxZ, v.position.z);
        EXPECT_TRUE(maxZ <= landmarks.eyeCenter[0].z + 0.0015f);
    }
    EXPECT_TRUE(scleraPieces == 2);
}

// R4: the hair shell is derived from the skull itself, so it can neither
// float above the head nor leave the brim ring that made the old hair read
// as a helmet.
PAE_TEST_CASE(hair_shell_hugs_the_skull) {
    const head::HeadParams hp = defaultHead();
    const MeshData head = head::generateHeadMesh(hp, kHeadCenter, kNeckBaseY, nullptr, 0);
    const float thickness = 0.010f;
    const MeshData shell = head::generateHairShell(hp, kHeadCenter, kNeckBaseY, thickness, 0);
    EXPECT_TRUE(shell.vertices.size() > 200);

    const TopologyReport topology = analyzeTopology(shell);
    EXPECT_TRUE(topology.boundaryEdges == 0);

    // Every hair vertex is within the shell thickness (plus the voxel
    // tolerance) of the skull surface.
    float worst = 0.0f;
    Vec3 worstAt{};
    for (size_t i = 0; i < shell.vertices.size(); i += 7) {
        const float d = nearestVertexDistance(head, shell.vertices[i].position, 1);
        if (d > worst) { worst = d; worstAt = shell.vertices[i].position; }
    }
    EXPECT_TRUE(worst <= thickness + 0.006f);

    // The shell must not sit on the face: no hair vertex in front of the
    // nose tip at eye height.
    float frontMost = -1e9f;
    for (const Vertex& v : shell.vertices) {
        if (std::fabs(v.position.y - (kHeadCenter.y - 0.02f * hp.height * 0.5f)) > 0.02f) continue;
        frontMost = std::fmax(frontMost, v.position.z);
    }
    EXPECT_TRUE(frontMost < kHeadCenter.z + hp.depth * 0.30f);
}

// M11-E: hair presets differ and all pieces validate.
PAE_TEST_CASE(hair_styles_differ_and_validate) {
    float counts[3];
    for (int style = 0; style < 3; ++style) {
        hair::HairParams hp{};
        hp.style = style;
        hp.backLength = 0.10f;
        auto pieces = hair::generateHair(hp, {0.0f, 1.68f, 0.0f}, 0.224f, 0.28f, 0.19f, 14);
        size_t total = 0;
        for (const auto& piece : pieces) {
            const ValidationReport report = validateMesh(piece.mesh);
            EXPECT_TRUE(report.valid);
            total += piece.mesh.vertices.size();
            for (const Vertex& v : piece.mesh.vertices) {
                // Strands stay near the head (no runaway geometry).
                EXPECT_TRUE((v.position - Vec3{0.0f, 1.68f, 0.0f}).length() < 0.6f);
            }
        }
        counts[style] = static_cast<float>(total);
    }
    EXPECT_TRUE(counts[0] != counts[1] || counts[1] != counts[2]);
    EXPECT_TRUE(counts[2] > counts[0]); // long hair has more geometry than short
}

// M11-F: clothing shells exist, cover the body, and respond to params.
PAE_TEST_CASE(clothing_shells_cover_body_and_respond_to_params) {
    const HumanoidResult withClothes = generateHumanoid(BodyParameters{});
    int shirtSubMeshes = 0, pantsSubMeshes = 0;
    size_t pantsVerts = 0;
    for (const SubMesh& sub : withClothes.mesh.subMeshes) {
        if (sub.materialId == 2) ++shirtSubMeshes;
        if (sub.materialId == 5) {
            ++pantsSubMeshes;
            pantsVerts += sub.indexCount / 3;
        }
    }
    EXPECT_TRUE(shirtSubMeshes >= 3); // body + collar + 2 sleeves
    EXPECT_TRUE(pantsSubMeshes >= 3); // pelvis + 2 leg tubes
    EXPECT_TRUE(pantsVerts > 150);

    // shirtHem below the pelvis adds shirt vertices under the hip line.
    BodyParameters longShirt{};
    longShirt.clothing.shirtHem = -0.15f;
    BodyParameters shortShirt{};
    shortShirt.clothing.shirtHem = 0.45f;
    auto shirtVertsBelowHips = [](const HumanoidResult& r) {
        int count = 0;
        for (size_t s = 0; s < r.mesh.subMeshes.size(); ++s) {
            if (r.mesh.subMeshes[s].materialId != 2) continue;
            for (uint32_t i = r.mesh.subMeshes[s].firstIndex;
                 i < r.mesh.subMeshes[s].firstIndex + r.mesh.subMeshes[s].indexCount; ++i) {
                if (r.mesh.vertices[r.mesh.indices[i]].position.y < r.joints.hips.y) ++count;
            }
        }
        return count;
    };
    const int longCount = shirtVertsBelowHips(generateHumanoid(longShirt));
    const int shortCount = shirtVertsBelowHips(generateHumanoid(shortShirt));
    std::fprintf(stderr, "[DBG] below-hips shirt verts: long=%d short=%d\n",
                 longCount, shortCount);
    EXPECT_TRUE(longCount > shortCount + 20);
}

int main() {
    return test::runAllTests();
}
