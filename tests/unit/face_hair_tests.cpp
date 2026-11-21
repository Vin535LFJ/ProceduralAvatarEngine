#include "mini_test.hpp"

#include "procedural/MeshValidator.h"
#include "procedural/humanoid/FaceGenerator.h"
#include "procedural/humanoid/HairGenerator.h"
#include "procedural/humanoid/Humanoid.h"

#include <cmath>
#include <cstring>

using namespace pae;

namespace {
face::FaceParams defaultFace() { return face::FaceParams{}; }
} // namespace

// M11-E: the shaped head passes validation and its jaw is narrower than the
// cheek band (the profile modulation actually works).
PAE_TEST_CASE(head_mesh_shaped_and_valid) {
    const face::FaceParams fp{};
    const MeshData head = face::generateHeadMesh(fp, {0.0f, 1.68f, 0.0f}, 0.224f, 0.28f, 20, 16);
    const ValidationReport report = validateMesh(head);
    EXPECT_TRUE(report.valid);
    if (!report.valid) {
        for (const std::string& error : report.errors) std::fprintf(stderr, "%s\n", error.c_str());
    }
    // Cheek band (y around head center) vs jaw band (near the bottom):
    // horizontal extent must be wider at the cheeks.
    float cheekMax = 0.0f, jawMax = 0.0f;
    const float centerY = 1.68f;
    for (const Vertex& v : head.vertices) {
        const float radial = std::sqrt(v.position.x * v.position.x + v.position.z * v.position.z);
        if (std::fabs(v.position.y - centerY) < 0.02f) cheekMax = std::fmax(cheekMax, radial);
        if (v.position.y < centerY - 0.11f && v.position.y > centerY - 0.14f)
            jawMax = std::fmax(jawMax, radial);
    }
    EXPECT_TRUE(cheekMax > jawMax);

    const MeshData again = face::generateHeadMesh(fp, {0.0f, 1.68f, 0.0f}, 0.224f, 0.28f, 20, 16);
    EXPECT_TRUE(std::memcmp(head.vertices.data(), again.vertices.data(),
                            head.vertices.size() * sizeof(Vertex)) == 0);
}

// M11-E: face features exist at expected anchor points and respond to params.
PAE_TEST_CASE(face_features_present_and_parameterized) {
    const face::FaceParams fp{};
    auto features = face::generateFaceFeatures(fp, {0.0f, 1.68f, 0.0f}, 0.224f, 0.28f, 14);
    EXPECT_TRUE(features.size() >= 10); // 2 eyes + 2 irises + 2 glints + 2 brows
                                       // + nose sweep/tip/2 wings + 2 lips + 2 ears
    bool eyeFound = false;
    const float expectedEyeX = 0.224f * 0.5f * 0.42f * fp.eyeSpacing;
    const Vec3 expectedEyeCenter{expectedEyeX, 1.68f + 0.28f * 0.5f * 0.04f,
                                 0.224f * 0.42f * 0.86f};
    for (const auto& piece : features) {
        if (piece.materialId != 3) continue;
        for (const Vertex& v : piece.mesh.vertices) {
            if ((v.position - expectedEyeCenter).lengthSq() < 9e-4f) eyeFound = true;
        }
    }
    EXPECT_TRUE(eyeFound);

    // eyeSpacing moves the eye anchors apart measurably.
    face::FaceParams wide = fp;
    wide.eyeSpacing = 1.6f;
    face::FaceParams narrow = fp;
    narrow.eyeSpacing = 0.5f;
    auto wideFeatures = face::generateFaceFeatures(wide, {0.0f, 1.68f, 0.0f}, 0.224f, 0.28f, 14);
    auto narrowFeatures = face::generateFaceFeatures(narrow, {0.0f, 1.68f, 0.0f}, 0.224f, 0.28f, 14);
    // Eye sclera pieces are materialId 3 — measure their x spread.
    auto eyeSpread = [](const std::vector<face::FacePiece>& pieces) {
        float minX = 1e9f, maxX = -1e9f;
        for (const auto& piece : pieces) {
            if (piece.materialId != 3 || piece.mesh.vertices.empty()) continue;
            for (const Vertex& v : piece.mesh.vertices) {
                minX = std::fmin(minX, v.position.x);
                maxX = std::fmax(maxX, v.position.x);
            }
        }
        return maxX - minX;
    };
    EXPECT_TRUE(eyeSpread(wideFeatures) > eyeSpread(narrowFeatures) + 0.01f);
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
