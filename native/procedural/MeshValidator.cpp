#include "MeshValidator.h"

#include <cmath>
#include <string>

namespace pae {

ValidationReport validateMesh(const MeshData& mesh, float minTriangleArea,
                              bool requireSkinWeights) {
    ValidationReport report;
    if (mesh.vertices.empty()) report.fail("mesh has no vertices");
    if (mesh.indices.empty()) report.fail("mesh has no indices");
    if (mesh.indices.size() % 3 != 0) report.fail("index count is not a multiple of 3");
    if (report.errors.size() >= 3) return report;

    Aabb computed;
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        const Vertex& v = mesh.vertices[i];
        if (!v.position.finite()) report.fail("vertex position is not finite: " + std::to_string(i));
        if (!v.normal.finite()) report.fail("vertex normal is not finite: " + std::to_string(i));
        const float n2 = v.normal.lengthSq();
        if (!(std::fabs(n2 - 1.0f) < 1e-3f)) {
            report.fail("vertex normal not unit length: " + std::to_string(i));
        }
        if (requireSkinWeights) {
            const float sum = v.weights.x + v.weights.y + v.weights.z + v.weights.w;
            if (!(sum > 0.99f && sum < 1.01f)) {
                report.fail("skin weights do not sum to 1: " + std::to_string(i));
            }
            const uint16_t joints[4] = {v.joints.x, v.joints.y, v.joints.z, v.joints.w};
            const float weights[4] = {v.weights.x, v.weights.y, v.weights.z, v.weights.w};
            for (int k = 0; k < 4; ++k) {
                if (weights[k] < 0.0f) report.fail("negative skin weight");
                if (weights[k] > 1e-6f && joints[k] >= 22) {
                    report.fail("skin weight references invalid bone id");
                }
            }
        }
        computed.expand(v.position);
        if (report.errors.size() > 16) return report;
    }

    bool indicesValid = true;
    for (size_t i = 0; i < mesh.indices.size(); ++i) {
        if (mesh.indices[i] >= mesh.vertices.size()) {
            report.fail("index out of range: " + std::to_string(i));
            indicesValid = false;
            break;
        }
    }

    size_t degenerate = 0;
    if (indicesValid) {
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            const Vertex& a = mesh.vertices[mesh.indices[i]];
            const Vertex& b = mesh.vertices[mesh.indices[i + 1]];
            const Vertex& c = mesh.vertices[mesh.indices[i + 2]];
            const float area = (b.position - a.position).cross(c.position - a.position).length() * 0.5f;
            if (!std::isfinite(area) || area < minTriangleArea) ++degenerate;
        }
    }
    // Poles of sphere-like primitives intentionally create degenerate rings.
    if (degenerate > mesh.indices.size() / 3 / 8) {
        report.fail("too many degenerate triangles: " + std::to_string(degenerate));
    }

    if (!computed.valid) {
        report.fail("computed AABB is invalid");
    } else if (mesh.bounds.valid) {
        const Vec3 expected = computed.size();
        const Vec3 actual = mesh.bounds.size();
        if (std::fabs(expected.x - actual.x) > 1e-3f ||
            std::fabs(expected.y - actual.y) > 1e-3f ||
            std::fabs(expected.z - actual.z) > 1e-3f) {
            report.fail("stored AABB size differs from computed bounds");
        }
    }

    for (const SubMesh& sub : mesh.subMeshes) {
        if (sub.firstIndex > mesh.indices.size() ||
            static_cast<size_t>(sub.firstIndex) + sub.indexCount > mesh.indices.size()) {
            report.fail("submesh index range out of bounds");
        }
    }
    return report;
}

} // namespace pae
