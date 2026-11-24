#include "procedural/shape/v2/CrossBoundarySeamAudit.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>

namespace pae {
namespace shape {
namespace v2 {

namespace {

constexpr double kPi = 3.14159265358979323846;

// Finds the triangle in `mesh` that has `v0` and `v1` as one of its three
// directed edges. `dir` is +1 when the triangle walks v0 -> v1 and -1 when it
// walks v1 -> v0. The first matching triangle wins; a boundary edge should be
// incident to exactly one surface triangle per patch.
struct EdgeTriangle {
    bool found = false;
    std::array<uint32_t, 3> ids{0, 0, 0};
    int dir = 0;
};

EdgeTriangle findEdgeTriangle(const MeshData& mesh, uint32_t v0, uint32_t v1) {
    EdgeTriangle result;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const uint32_t a = mesh.indices[i];
        const uint32_t b = mesh.indices[i + 1];
        const uint32_t c = mesh.indices[i + 2];
        const std::array<uint32_t, 3> tri = {a, b, c};
        for (int e = 0; e < 3; ++e) {
            const uint32_t from = tri[static_cast<size_t>(e)];
            const uint32_t to = tri[static_cast<size_t>((e + 1) % 3)];
            if (from == v0 && to == v1) {
                result.found = true;
                result.ids = tri;
                result.dir = 1;
                return result;
            }
            if (from == v1 && to == v0) {
                result.found = true;
                result.ids = tri;
                result.dir = -1;
                return result;
            }
        }
    }
    return result;
}

// Face normal of a real triangle, from its OWN winding. No synthesised or
// caller-supplied normal is used anywhere in this module.
bool triangleFaceNormal(const MeshData& mesh, const std::array<uint32_t, 3>& tri, Vec3* out) {
    if (tri[0] >= mesh.vertices.size() || tri[1] >= mesh.vertices.size() ||
        tri[2] >= mesh.vertices.size()) {
        return false;
    }
    const Vec3& a = mesh.vertices[tri[0]].position;
    const Vec3& b = mesh.vertices[tri[1]].position;
    const Vec3& c = mesh.vertices[tri[2]].position;
    const Vec3 n = (b - a).cross(c - a);
    const float len = std::sqrt(n.lengthSq());
    if (!(len > 1e-12f)) return false;
    *out = n * (1.0f / len);
    return true;
}

std::vector<double> summarizeAngles(std::vector<double> values) {
    std::vector<double> stats(3, 0.0);  // max, mean, p95
    if (values.empty()) return stats;
    std::sort(values.begin(), values.end());
    const double sum = std::accumulate(values.begin(), values.end(), 0.0);
    stats[0] = values.back();
    stats[1] = sum / static_cast<double>(values.size());
    const size_t rank = static_cast<size_t>(std::ceil(0.95 * static_cast<double>(values.size())));
    const size_t idx = rank > 0 ? rank - 1 : 0;
    stats[2] = values[std::min(values.size() - 1, idx)];
    return stats;
}

} // namespace

const char* crossSeamApplicabilityName(CrossSeamApplicability a) {
    switch (a) {
        case CrossSeamApplicability::Applicable: return "APPLICABLE";
        case CrossSeamApplicability::NotApplicable: return "NOT_APPLICABLE";
    }
    return "UNKNOWN";
}

double crossSeamAngleDeg(const Vec3& n0, const Vec3& n1) {
    float d = n0.dot(n1);
    d = std::clamp(d, -1.0f, 1.0f);
    return std::acos(static_cast<double>(d)) * 180.0 / kPi;
}

CrossBoundarySeamMeasurement measureCrossBoundarySeam(const SeamPatch& sideA,
                                                       const SeamPatch& sideB,
                                                       double positionToleranceM) {
    CrossBoundarySeamMeasurement m;

    if (sideA.boundary.size() < 2 || sideB.boundary.size() < 2) {
        m.notes.push_back("one or both patches are missing a boundary loop of at least 2 vertices");
        m.notes.push_back("NOT APPLICABLE must never be recorded as PASS");
        return m;
    }

    const size_t n = sideA.boundary.size();
    m.boundaryEdges = static_cast<int>(n);
    for (uint32_t v : sideA.boundary) {
        if (v >= sideA.mesh.vertices.size()) {
            m.notes.push_back("side A boundary contains an out-of-range vertex id");
            return m;
        }
    }
    for (uint32_t v : sideB.boundary) {
        if (v >= sideB.mesh.vertices.size()) {
            m.notes.push_back("side B boundary contains an out-of-range vertex id");
            return m;
        }
    }

    // ---- Pair boundary vertices by POSITION --------------------------------
    // Nearest unused B vertex within tolerance. Index order is deliberately
    // ignored here; order is checked separately below.
    std::vector<bool> usedB(sideB.boundary.size(), false);
    std::vector<int> matchAtoB(n, -1);   // A boundary position -> B boundary position
    std::vector<int> matchBtoA(sideB.boundary.size(), -1);
    double posSum = 0.0;      // sum of NEAREST distances over matched-by-nearest A vertices
    double posMax = 0.0;      // max NEAREST distance, including failed matches
    int nearestCount = 0;     // A vertices that had any unused B candidate
    const double tol2 = positionToleranceM * positionToleranceM;

    for (size_t i = 0; i < n; ++i) {
        const Vec3& pa = sideA.mesh.vertices[sideA.boundary[i]].position;
        int best = -1;
        double bestD2 = std::numeric_limits<double>::infinity();
        for (size_t j = 0; j < sideB.boundary.size(); ++j) {
            if (usedB[j]) continue;
            const Vec3& pb = sideB.mesh.vertices[sideB.boundary[j]].position;
            const Vec3 delta = pa - pb;
            const double d2 = static_cast<double>(delta.lengthSq());
            if (d2 < bestD2) {
                bestD2 = d2;
                best = static_cast<int>(j);
            }
        }
        if (best < 0) {
            ++m.unmatchedVertices;
            continue;
        }
        // Report the nearest distance even when it exceeds tolerance: a
        // mismatched boundary must still carry a measurable position error.
        const double d = std::sqrt(bestD2);
        posSum += d;
        posMax = std::max(posMax, d);
        ++nearestCount;
        if (bestD2 > tol2) {
            ++m.unmatchedVertices;
            continue;
        }
        usedB[static_cast<size_t>(best)] = true;
        matchAtoB[i] = best;
        matchBtoA[static_cast<size_t>(best)] = static_cast<int>(i);
    }
    m.matchedPairs = static_cast<int>(std::count_if(matchAtoB.begin(), matchAtoB.end(),
                                                    [](int j) { return j >= 0; }));

    if (nearestCount > 0) {
        m.positionErrorMaxM = posMax;
        m.positionErrorMeanM = posSum / static_cast<double>(nearestCount);
    }

    if (m.unmatchedVertices > 0) {
        m.notes.push_back("boundary vertices could not be paired within the position tolerance");
        m.notes.push_back("NOT APPLICABLE: the two patches do not share a boundary");
        return m;
    }

    // ---- Boundary order ----------------------------------------------------
    // A matched but re-ordered / reversed boundary is a real finding, not a
    // reason to silently re-index the mesh.
    {
        int increasing = 0;
        int decreasing = 0;
        for (size_t i = 0; i + 1 < n; ++i) {
            const int j0 = matchAtoB[i];
            const int j1 = matchAtoB[i + 1];
            if (j1 > j0) ++increasing;
            if (j1 < j0) ++decreasing;
        }
        m.boundaryOrderReversed = (decreasing > increasing);
        if (increasing > 0 && decreasing > 0) {
            m.notes.push_back("matched boundary order is neither uniform nor the reverse of side A");
        }
    }

    // ---- Per-edge cross-boundary angle ------------------------------------
    std::vector<double> angles;
    angles.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        const size_t iNext = (i + 1) % n;
        const uint32_t a0 = sideA.boundary[i];
        const uint32_t a1 = sideA.boundary[iNext];
        const int bIdx0 = matchAtoB[i];
        const int bIdx1 = matchAtoB[iNext];
        const uint32_t b0 = sideB.boundary[static_cast<size_t>(bIdx0)];
        const uint32_t b1 = sideB.boundary[static_cast<size_t>(bIdx1)];

        const EdgeTriangle ta = findEdgeTriangle(sideA.mesh, a0, a1);
        const EdgeTriangle tb = findEdgeTriangle(sideB.mesh, b0, b1);
        if (!ta.found || !tb.found) {
            ++m.missingTrianglePairs;
            continue;
        }

        // Correctly oriented neighbours traverse their shared edge in
        // OPPOSITE directions. Same direction => one patch is flipped.
        if (ta.dir == tb.dir) {
            ++m.windingInconsistentPairs;
            continue;
        }

        Vec3 na{0.0f, 0.0f, 0.0f};
        Vec3 nb{0.0f, 0.0f, 0.0f};
        if (!triangleFaceNormal(sideA.mesh, ta.ids, &na) ||
            !triangleFaceNormal(sideB.mesh, tb.ids, &nb)) {
            ++m.missingTrianglePairs;
            continue;
        }
        angles.push_back(crossSeamAngleDeg(na, nb));
    }

    m.windingVerified = (m.windingInconsistentPairs == 0);
    if (!m.windingVerified) {
        m.notes.push_back("some matched edges are traversed in the same direction by both patches");
    }
    if (m.missingTrianglePairs > 0) {
        m.notes.push_back("some matched edges have no surface triangle on at least one side");
    }

    if (angles.empty()) {
        m.notes.push_back("no usable triangle pair: NOT APPLICABLE, never PASS");
        return m;
    }

    // A partial winding failure must never be readable as a PASS from the
    // remaining subset. Keep the counters for diagnosis, but withhold the
    // applicability flag and the statistics entirely.
    if (!m.windingVerified) {
        m.notes.push_back("NOT APPLICABLE: winding is not verified on every matched edge");
        return m;
    }

    m.usableAngleSamples = static_cast<int>(angles.size());
    const std::vector<double> stats = summarizeAngles(std::move(angles));
    m.maxDeg = stats[0];
    m.meanDeg = stats[1];
    m.p95Deg = stats[2];
    m.applicability = CrossSeamApplicability::Applicable;
    return m;
}

} // namespace v2
} // namespace shape
} // namespace pae
