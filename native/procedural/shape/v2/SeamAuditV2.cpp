#include "procedural/shape/v2/SeamAuditV2.h"

#include "procedural/shape/SurfaceMetrics.h"   // Distribution

#include <algorithm>
#include <cmath>
#include <map>

namespace pae {
namespace shape {
namespace v2 {

namespace {

constexpr double kPi = 3.14159265358979323846;

// Vertices whose height matches the crop ring within this tolerance belong to
// the ring. The template places every ring vertex at exactly the same height,
// so the tolerance only absorbs float error.
constexpr float kRingHeightEps = 1e-4f;

Distribution summarizeLocal(std::vector<double> values) {
    Distribution d;
    if (values.empty()) return d;
    std::sort(values.begin(), values.end());
    d.count = static_cast<int>(values.size());
    d.mean = 0.0;
    for (double v : values) d.mean += v;
    d.mean /= static_cast<double>(values.size());
    d.max = values.back();
    d.p50 = values[values.size() / 2];
    // Nearest-rank p95.
    const size_t idx = static_cast<size_t>(std::ceil(0.95 * static_cast<double>(values.size())));
    d.p95 = values[std::min(values.size() - 1, idx > 0 ? idx - 1 : 0)];
    return d;
}

} // namespace

const char* seamApplicabilityName(SeamApplicability a) {
    switch (a) {
        case SeamApplicability::Applicable: return "APPLICABLE";
        case SeamApplicability::NotApplicable: return "NOT_APPLICABLE";
    }
    return "UNKNOWN";
}

Vec3 seamQuadNormal(const MeshData& mesh, const std::array<uint32_t, 4>& quad) {
    // The normal must follow the TRIANGLE WINDING, because the definition
    // says "outward normals, determined by the face winding".
    //
    // It is NOT computed from the diagonal cross product of the quad's vertex
    // ORDER: in the template layout {lower, lower, upper, upper} the vertex
    // order runs clockwise when seen from OUTSIDE, so the diagonal product
    // points INWARD while the triangles point OUTWARD. The G2 code uses the
    // diagonal product; that is self-consistent for its angle statistics (the
    // sign cancels), but it cannot be used for a winding check, which must
    // compare against the outward radial direction.
    //
    // So the normal is taken from the mesh's own triangles: every triangle
    // whose three vertices belong to this quad contributes its face normal.
    Vec3 accumulated{0.0f, 0.0f, 0.0f};
    bool found = false;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const uint32_t a = mesh.indices[i];
        const uint32_t b = mesh.indices[i + 1];
        const uint32_t c = mesh.indices[i + 2];
        const bool inQuad = (a == quad[0] || a == quad[1] || a == quad[2] || a == quad[3]) &&
                            (b == quad[0] || b == quad[1] || b == quad[2] || b == quad[3]) &&
                            (c == quad[0] || c == quad[1] || c == quad[2] || c == quad[3]);
        if (!inQuad) continue;
        const Vec3& pa = mesh.vertices[a].position;
        const Vec3& pb = mesh.vertices[b].position;
        const Vec3& pc = mesh.vertices[c].position;
        accumulated = accumulated + (pb - pa).cross(pc - pa);
        found = true;
    }
    if (!found) {
        // Fall back to the diagonal product when the quad carries no triangles.
        const Vec3& a = mesh.vertices[quad[0]].position;
        const Vec3& b = mesh.vertices[quad[1]].position;
        const Vec3& c = mesh.vertices[quad[2]].position;
        const Vec3& d = mesh.vertices[quad[3]].position;
        const Vec3 diag1 = c - a;
        const Vec3 diag2 = d - b;
        Vec3 n = diag1.cross(diag2);
        const float len = std::sqrt(n.lengthSq());
        if (len < 1e-12f) return Vec3{0.0f, 1.0f, 0.0f};
        return n * (1.0f / len);
    }
    const float len = std::sqrt(accumulated.lengthSq());
    if (len < 1e-12f) return Vec3{0.0f, 1.0f, 0.0f};
    return accumulated * (1.0f / len);
}

double seamAngleDeg(const Vec3& n0, const Vec3& n1) {
    float d = n0.dot(n1);
    d = std::clamp(d, -1.0f, 1.0f);
    // acos keeps the full [0, 180] domain: a fold above 90 deg stays visible.
    return std::acos(static_cast<double>(d)) * 180.0 / kPi;
}

SeamMeasurement measureShoulderRootSeam(const MeshData& mesh,
                                         const std::vector<std::array<uint32_t, 4>>& quads,
                                         const std::vector<CropBoundary>& crops,
                                         const std::string& cropLabel,
                                         double thresholdDeg) {
    SeamMeasurement m;
    m.thresholdDeg = thresholdDeg;

    // ---- Locate the crop ring ---------------------------------------------
    const CropBoundary* crop = nullptr;
    for (const CropBoundary& c : crops) {
        if (c.label == cropLabel) {
            crop = &c;
            break;
        }
    }
    if (crop == nullptr || crop->ringIndex < 0) {
        m.notes.push_back("no registered crop boundary with the requested label");
        m.notes.push_back("NOT APPLICABLE must never be recorded as PASS");
        return m;   // applicability stays NotApplicable, passes stays false
    }

    m.cropLabel = crop->label;
    m.ringIndex = crop->ringIndex;
    m.ringVertexCount = crop->vertexCount;

    // ---- The ring vertex set ----------------------------------------------
    //
    // Identified GEOMETRICALLY (the vertices that sit at the crop height),
    // not by index arithmetic, so the measurement does not depend on the
    // template layout convention.
    std::vector<uint32_t> ringVertices;
    ringVertices.reserve(static_cast<size_t>(std::max(0, crop->vertexCount)));
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        if (std::fabs(mesh.vertices[i].position.y - crop->y) <= kRingHeightEps) {
            ringVertices.push_back(static_cast<uint32_t>(i));
        }
    }
    if (ringVertices.size() < 3) {
        m.notes.push_back("crop ring has fewer than 3 vertices at the crop height");
        return m;
    }
    const std::map<uint32_t, bool> ringSet = [&] {
        std::map<uint32_t, bool> s;
        for (uint32_t v : ringVertices) s[v] = true;
        return s;
    }();

    // ---- Select the included quads ----------------------------------------
    //
    // A quad is included when its UPPER ROW (the two vertices with the
    // greater height) lies on the crop ring. Quads that share no edge with
    // the ring are excluded by construction.
    struct IncludedQuad {
        size_t index = 0;
        std::array<uint32_t, 4> quad{};
        int upperRow[2] = {0, 0};   // the two ring vertices
        int lowerRow[2] = {0, 0};   // the two vertices below the ring
    };
    std::vector<IncludedQuad> included;

    for (size_t q = 0; q < quads.size(); ++q) {
        const std::array<uint32_t, 4>& quad = quads[q];
        bool valid = true;
        for (uint32_t idx : quad) {
            if (idx >= mesh.vertices.size()) {
                valid = false;
                break;
            }
        }
        if (!valid) continue;

        // Split into the two rows by height. The upper row is the pair with
        // the greater y; ties (a horizontal ring) are broken by winding
        // order so the split stays deterministic.
        int order[4] = {0, 1, 2, 3};
        const float y0 = mesh.vertices[quad[0]].position.y;
        const float y1 = mesh.vertices[quad[1]].position.y;
        const float y2 = mesh.vertices[quad[2]].position.y;
        const float y3 = mesh.vertices[quad[3]].position.y;
        // Template layout: quad = { lower, lower, upper, upper }.
        const bool lowerFirst = (y0 + y1) <= (y2 + y3);
        const int l0 = lowerFirst ? 0 : 2;
        const int l1 = lowerFirst ? 1 : 3;
        const int u0 = lowerFirst ? 2 : 0;
        const int u1 = lowerFirst ? 3 : 1;
        (void)order;

        const uint32_t upperA = quad[static_cast<size_t>(u0)];
        const uint32_t upperB = quad[static_cast<size_t>(u1)];
        if (ringSet.find(upperA) == ringSet.end() || ringSet.find(upperB) == ringSet.end()) {
            continue;   // excluded: no shared edge with the ring
        }

        IncludedQuad iq;
        iq.index = q;
        iq.quad = quad;
        iq.upperRow[0] = u0;
        iq.upperRow[1] = u1;
        iq.lowerRow[0] = l0;
        iq.lowerRow[1] = l1;
        included.push_back(iq);
    }

    if (included.size() < 3) {
        m.notes.push_back("fewer than 3 quads share the crop ring");
        return m;
    }

    m.includedQuadCount = static_cast<int>(included.size());

    // ---- Winding verification ---------------------------------------------
    //
    // Every included quad must have an OUTWARD normal. The outward direction
    // of a torso tube is the radial direction from the body axis, so a quad
    // whose normal points inward has an inconsistent winding and its angle
    // samples must not be trusted.
    std::vector<bool> quadWindingOk(included.size(), true);
    for (size_t i = 0; i < included.size(); ++i) {
        const std::array<uint32_t, 4>& quad = included[i].quad;
        Vec3 centroid{0.0f, 0.0f, 0.0f};
        for (uint32_t idx : quad) {
            const Vec3& p = mesh.vertices[idx].position;
            centroid = centroid + p;
        }
        centroid = centroid * 0.25f;

        const Vec3 outward{centroid.x, 0.0f, centroid.z};
        const float outwardLen = std::sqrt(outward.lengthSq());
        if (outwardLen < 1e-6f) {
            // On the axis: cannot judge radially, accept the winding.
            continue;
        }
        const Vec3 n = seamQuadNormal(mesh, quad);
        if (n.dot(outward) <= 0.0f) {
            quadWindingOk[i] = false;
            ++m.windingInconsistencyCount;
        }
    }
    m.windingVerified = (m.windingInconsistencyCount == 0);
    if (!m.windingVerified) {
        m.notes.push_back("winding inconsistency detected: the affected quads are "
                          "excluded from the angle statistic");
    }

    // ---- Band angles ------------------------------------------------------
    //
    // The band quads form a ring around the body; consecutive quads in the
    // band are neighbours around the circumference. The angle between two
    // consecutive band quads' face normals is the ring's smoothness.
    std::vector<double> bandAngles;
    bandAngles.reserve(included.size());
    for (size_t i = 0; i < included.size(); ++i) {
        if (!quadWindingOk[i]) continue;
        const size_t next = (i + 1) % included.size();
        if (!quadWindingOk[next]) continue;
        const Vec3 n0 = seamQuadNormal(mesh, included[i].quad);
        const Vec3 n1 = seamQuadNormal(mesh, included[next].quad);
        bandAngles.push_back(seamAngleDeg(n0, n1));
    }

    if (bandAngles.empty()) {
        m.notes.push_back("no usable angle samples after the winding check");
        return m;
    }

    m.angleSampleCount = static_cast<int>(bandAngles.size());
    const Distribution stats = summarizeLocal(bandAngles);
    m.maxDeg = stats.max;
    m.meanDeg = stats.mean;
    m.p95Deg = stats.p95;

    // ---- Cap fan triangles ------------------------------------------------
    //
    // The definition requires the cap fan to be included and labelled
    // separately. The fan is identified STRUCTURALLY: a triangle that does
    // not belong to any quad of the canonical grid is a cap triangle, and a
    // set of such triangles that share one vertex is a fan.
    //
    // The gate statistic stays the BAND max; the combined number is reported
    // so the alternative reading of the definition is visible too.
    {
        // Which quads could contain a given triangle.
        std::map<uint32_t, std::vector<size_t>> vertexToQuads;
        for (size_t q = 0; q < quads.size(); ++q) {
            for (uint32_t v : quads[q]) vertexToQuads[v].push_back(q);
        }

        // Collect the triangles that are not part of the quad grid.
        std::vector<std::array<uint32_t, 3>> capTriangles;
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            const uint32_t a = mesh.indices[i];
            const uint32_t b = mesh.indices[i + 1];
            const uint32_t c = mesh.indices[i + 2];
            bool inGrid = false;
            auto it = vertexToQuads.find(a);
            if (it != vertexToQuads.end()) {
                for (size_t q : it->second) {
                    const std::array<uint32_t, 4>& quad = quads[q];
                    auto has = [&](uint32_t v) {
                        return v == quad[0] || v == quad[1] || v == quad[2] || v == quad[3];
                    };
                    if (has(b) && has(c)) {
                        inGrid = true;
                        break;
                    }
                }
            }
            if (!inGrid) capTriangles.push_back({a, b, c});
        }

        if (!capTriangles.empty()) {
            // Find the shared vertex: the pole of the fan.
            uint32_t pole = capTriangles[0][0];
            int bestCount = 0;
            for (int k = 0; k < 3; ++k) {
                const uint32_t candidate = capTriangles[0][static_cast<size_t>(k)];
                int count = 0;
                for (const std::array<uint32_t, 3>& tri : capTriangles) {
                    if (tri[0] == candidate || tri[1] == candidate || tri[2] == candidate) ++count;
                }
                if (count > bestCount) {
                    bestCount = count;
                    pole = candidate;
                }
            }

            // Order the fan's triangles around the pole so the adjacent
            // pairs are the real neighbours, and take each triangle's normal
            // from its OWN winding (not a synthesised reverse, which would
            // always read 180 deg).
            std::vector<std::array<uint32_t, 3>> fanTriangles;
            for (const std::array<uint32_t, 3>& tri : capTriangles) {
                if (tri[0] == pole || tri[1] == pole || tri[2] == pole) {
                    fanTriangles.push_back(tri);
                }
            }
            const Vec3 polePos = mesh.vertices[pole].position;
            auto azimuthOf = [&](uint32_t v) {
                const Vec3& p = mesh.vertices[v].position;
                return std::atan2(p.z - polePos.z, p.x - polePos.x);
            };
            std::sort(fanTriangles.begin(), fanTriangles.end(),
                      [&](const std::array<uint32_t, 3>& a, const std::array<uint32_t, 3>& b) {
                          const float aa = azimuthOf(a[0] == pole ? a[1] : a[0]);
                          const float ab = azimuthOf(b[0] == pole ? b[1] : b[0]);
                          return aa < ab;
                      });

            std::vector<double> capAngles;
            for (size_t i = 0; i < fanTriangles.size(); ++i) {
                const std::array<uint32_t, 3>& t0 = fanTriangles[i];
                const std::array<uint32_t, 3>& t1 = fanTriangles[(i + 1) % fanTriangles.size()];
                const Vec3& a0 = mesh.vertices[t0[0]].position;
                const Vec3& b0 = mesh.vertices[t0[1]].position;
                const Vec3& c0 = mesh.vertices[t0[2]].position;
                const Vec3& a1 = mesh.vertices[t1[0]].position;
                const Vec3& b1 = mesh.vertices[t1[1]].position;
                const Vec3& c1 = mesh.vertices[t1[2]].position;
                const Vec3 n0 = (b0 - a0).cross(c0 - a0);
                const Vec3 n1 = (b1 - a1).cross(c1 - a1);
                const float l0 = std::sqrt(n0.lengthSq());
                const float l1 = std::sqrt(n1.lengthSq());
                if (l0 < 1e-12f || l1 < 1e-12f) continue;
                capAngles.push_back(seamAngleDeg(n0 * (1.0f / l0), n1 * (1.0f / l1)));
            }
            m.capTriangleCount = static_cast<int>(fanTriangles.size());
            m.capAngleSampleCount = static_cast<int>(capAngles.size());
            if (!capAngles.empty()) {
                const Distribution capStats = summarizeLocal(capAngles);
                m.capMaxDeg = std::max(m.capMaxDeg, capStats.max);
                m.capMeanDeg = std::max(m.capMeanDeg, capStats.mean);
            }
            m.notes.push_back("the cap fan is included and reported separately; the gate "
                              "statistic is the band max, and maxDegIncludingCap carries the "
                              "alternative reading");
        }
    }

    m.maxDegIncludingCap = std::max(m.maxDeg, m.capMaxDeg);

    m.applicability = SeamApplicability::Applicable;
    m.passes = (m.maxDeg <= m.thresholdDeg);
    return m;
}

} // namespace v2
} // namespace shape
} // namespace pae
