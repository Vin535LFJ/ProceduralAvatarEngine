#include "procedural/human_template/BaseHumanRenderMeshBuilder.h"

#include <cmath>

namespace pae::human_template {

namespace {

double minAngleOfTriangle(const D3& a, const D3& b, const D3& c) {
    // Smallest interior angle via edge lengths (law of cosines), robust for
    // slivers. Returns 0 for degenerate triangles.
    const double ab = (b - a).length(), bc = (c - b).length(), ca = (a - c).length();
    if (ab < 1e-12 || bc < 1e-12 || ca < 1e-12) return 0.0;
    const double edges[3] = {ab, bc, ca};
    double minAngle = 3.14159265358979323846;
    for (int i = 0; i < 3; ++i) {
        const double opp = edges[i];
        const double s1 = edges[(i + 1) % 3], s2 = edges[(i + 2) % 3];
        double cosAngle = (s1 * s1 + s2 * s2 - opp * opp) / (2.0 * s1 * s2);
        if (cosAngle > 1.0) cosAngle = 1.0;
        if (cosAngle < -1.0) cosAngle = -1.0;
        const double angle = std::acos(cosAngle);
        if (angle < minAngle) minAngle = angle;
    }
    return minAngle;
}

double splitQuality(const D3& p0, const D3& p1, const D3& p2, const D3& p3, bool diagonal02) {
    if (diagonal02) {
        return std::min(minAngleOfTriangle(p0, p1, p2), minAngleOfTriangle(p0, p2, p3));
    }
    return std::min(minAngleOfTriangle(p1, p2, p3), minAngleOfTriangle(p1, p3, p0));
}

} // namespace

bool buildStaticRenderMesh(const HumanTemplate& t, MeshData& out, std::string& error) {
    StaticRenderMeshReport report;
    return buildStaticRenderMesh(t, out, report, error);
}

bool buildStaticRenderMesh(const HumanTemplate& t, MeshData& out, StaticRenderMeshReport& report,
                           std::string& error) {
    out = MeshData{};
    report = StaticRenderMeshReport{};
    const PolyMesh& topo = t.topology;
    if (topo.pos.empty() || topo.faces.empty()) {
        error = "template topology is empty";
        return false;
    }
    const size_t vertexCount = topo.pos.size();
    out.vertices.resize(vertexCount);
    for (size_t i = 0; i < vertexCount; ++i) {
        Vertex v;
        v.position = Vec3(static_cast<float>(topo.pos[i].x),
                          static_cast<float>(topo.pos[i].y),
                          static_cast<float>(topo.pos[i].z));
        v.normal = Vec3(0.0f, 0.0f, 0.0f);
        v.uv = Vec2(0.0f, 0.0f);
        v.joints = U16Vec4(0, 0, 0, 0);
        v.weights = Vec4(1.0f, 0.0f, 0.0f, 0.0f);
        v.color = Vec4(1.0f, 1.0f, 1.0f, 0.0f);  // materialId 0 (skin); clay view ignores it
        out.vertices[i] = v;
    }

    // Accumulate area-weighted face normals in double precision.
    std::vector<D3> normalAccum(vertexCount, D3{});
    auto emitTriangle = [&](int ia, int ib, int ic, int ta, int tb, int tc) {
        const D3& a = topo.pos[static_cast<size_t>(ia)];
        const D3& b = topo.pos[static_cast<size_t>(ib)];
        const D3& c = topo.pos[static_cast<size_t>(ic)];
        const D3 faceNormal = (b - a).cross(c - a);  // 2x area, outward by winding
        normalAccum[static_cast<size_t>(ia)] += faceNormal;
        normalAccum[static_cast<size_t>(ib)] += faceNormal;
        normalAccum[static_cast<size_t>(ic)] += faceNormal;
        if (faceNormal.length() < 1e-14) ++report.degenerateTriangles;
        out.indices.push_back(static_cast<uint32_t>(ia));
        out.indices.push_back(static_cast<uint32_t>(ib));
        out.indices.push_back(static_cast<uint32_t>(ic));
        auto setUv = [&](int vertexId, int uvId) {
            if (uvId >= 0 && static_cast<size_t>(uvId) < topo.uv.size()) {
                const D2& uv = topo.uv[static_cast<size_t>(uvId)];
                out.vertices[static_cast<size_t>(vertexId)].uv =
                    Vec2(static_cast<float>(uv.u), static_cast<float>(uv.v));
            }
        };
        setUv(ia, ta);
        setUv(ib, tb);
        setUv(ic, tc);
    };

    for (const PolyFace& face : topo.faces) {
        const size_t n = face.v.size();
        if (n < 3) {
            error = "face with < 3 corners";
            return false;
        }
        for (size_t k = 0; k < n; ++k) {
            if (face.v[k] < 0 || static_cast<size_t>(face.v[k]) >= vertexCount) {
                error = "face vertex index out of range";
                return false;
            }
        }
        if (n == 3) {
            emitTriangle(face.v[0], face.v[1], face.v[2], face.vt[0], face.vt[1], face.vt[2]);
            ++report.otherFaceCount;
        } else if (n == 4) {
            const D3& p0 = topo.pos[static_cast<size_t>(face.v[0])];
            const D3& p1 = topo.pos[static_cast<size_t>(face.v[1])];
            const D3& p2 = topo.pos[static_cast<size_t>(face.v[2])];
            const D3& p3 = topo.pos[static_cast<size_t>(face.v[3])];
            const double q02 = splitQuality(p0, p1, p2, p3, true);
            const double q13 = splitQuality(p0, p1, p2, p3, false);
            if (q13 > q02) {
                emitTriangle(face.v[1], face.v[2], face.v[3], face.vt[1], face.vt[2], face.vt[3]);
                emitTriangle(face.v[1], face.v[3], face.v[0], face.vt[1], face.vt[3], face.vt[0]);
            } else {
                emitTriangle(face.v[0], face.v[1], face.v[2], face.vt[0], face.vt[1], face.vt[2]);
                emitTriangle(face.v[0], face.v[2], face.v[3], face.vt[0], face.vt[2], face.vt[3]);
            }
            ++report.quadCount;
        } else {
            for (size_t k = 1; k + 1 < n; ++k) {
                emitTriangle(face.v[0], face.v[k], face.v[k + 1],
                             face.vt[0], face.vt[k], face.vt[k + 1]);
            }
            ++report.otherFaceCount;
        }
    }

    for (size_t i = 0; i < vertexCount; ++i) {
        const D3 n = normalAccum[i].normalized();
        out.vertices[i].normal = (n.length() > 0.5)
            ? Vec3(static_cast<float>(n.x), static_cast<float>(n.y), static_cast<float>(n.z))
            : Vec3(0.0f, 1.0f, 0.0f);
        out.bounds.expand(out.vertices[i].position);
    }

    SubMesh sub;
    sub.firstIndex = 0;
    sub.indexCount = static_cast<uint32_t>(out.indices.size());
    sub.materialId = 0;
    out.subMeshes.push_back(sub);

    report.vertexCount = static_cast<uint32_t>(vertexCount);
    report.triangleCount = static_cast<uint32_t>(out.indices.size() / 3);
    return true;
}

} // namespace pae::human_template