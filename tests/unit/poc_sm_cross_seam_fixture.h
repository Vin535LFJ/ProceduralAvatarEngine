#pragma once

// Shared synthetic two-patch geometry for the cross-boundary seam experiment.
// Included by both the unit tests and the evidence probe so that the reported
// numbers and the asserted numbers come from exactly the same construction.
//
// Geometry: a lower cylinder and an upper cone meet on a circular ring at
// y = 0. The upper cone flares outward at slope tan(foldDeg), so the TRUE
// cross-boundary dihedral at the shared edge is exactly foldDeg for every
// segment count. Both patches have outward winding, and the two triangles
// adjacent to a shared edge traverse that edge in opposite directions.

#include "procedural/MeshData.h"
#include "procedural/shape/QuadTemplate.h"
#include "procedural/shape/v2/CrossBoundarySeamAudit.h"
#include "procedural/shape/v2/SeamAuditV2.h"

#include <array>
#include <cmath>
#include <vector>

namespace pae {
namespace shape {
namespace v2 {
namespace cross_seam_fixture {

constexpr float kPi = 3.14159265358979323846f;

struct FoldedSeam {
    SeamPatch lower;   // side A: cylinder below the ring
    SeamPatch upper;   // side B: cone above the ring
    MeshData combined; // the same two surfaces in one mesh, for the legacy tool
    std::vector<std::array<uint32_t, 4>> quads;
    std::vector<CropBoundary> crops;
    int segments = 0;
    int rowsPerPatch = 0;
    float foldDeg = 0.0f;
};

inline float rowHeight() { return 0.02f; }
inline float radius() { return 0.50f; }

inline FoldedSeam makeFoldedSeam(int segments, int rowsPerPatch, float foldDeg,
                                 bool flipUpper = false, float ringOffsetFraction = 0.0f) {
    FoldedSeam out;
    const int n = segments > 3 ? segments : 3;
    const int m = rowsPerPatch > 2 ? rowsPerPatch : 2;
    out.segments = n;
    out.rowsPerPatch = m;
    out.foldDeg = foldDeg;

    const float h = rowHeight();
    const float r = radius();
    const float slope = std::tan(foldDeg * kPi / 180.0f);
    const float dTheta = 2.0f * kPi / static_cast<float>(n);
    const float shift = ringOffsetFraction * dTheta;

    // Vertex id layout in `combined`:
    //   seam ring        : 0 .. n-1                    (y = 0)
    //   lower rows j>=1  : j*n .. j*n+n-1              (y = -j*h)
    //   upper rows k>=1  : (m-1+k)*n .. (m-1+k)*n+n-1  (y = +k*h)
    auto addRing = [&](MeshData& mesh, int row, bool upperRow) {
        const float y = upperRow ? static_cast<float>(row) * h
                                 : -static_cast<float>(row) * h;
        const float ringR = upperRow ? r + static_cast<float>(row) * h * slope : r;
        for (int i = 0; i < n; ++i) {
            const float theta = static_cast<float>(i) * dTheta;
            Vertex v;
            v.position = Vec3{ringR * std::cos(theta), y, ringR * std::sin(theta)};
            v.uv = Vec2{static_cast<float>(i) / static_cast<float>(n), 0.0f};
            v.weights = Vec4{1.0f, 0.0f, 0.0f, 0.0f};
            v.color = Vec4{0.78f, 0.78f, 0.80f, 1.0f};
            mesh.vertices.push_back(v);
        }
    };

    // ---- Combined mesh -----------------------------------------------------
    addRing(out.combined, 0, true);                    // seam (row 0)
    for (int j = 1; j < m; ++j) addRing(out.combined, j, false);   // lower
    for (int k = 1; k < m; ++k) addRing(out.combined, k, true);    // upper

    auto seamId = [&](int i) { return static_cast<uint32_t>(i); };
    auto lowerId = [&](int j, int i) { return static_cast<uint32_t>(j * n + i); };
    auto upperId = [&](int k, int i) {
        return static_cast<uint32_t>((m - 1 + k) * n + i);
    };

    auto appendTri = [&](MeshData& mesh, uint32_t a, uint32_t b, uint32_t c) {
        mesh.indices.push_back(a);
        mesh.indices.push_back(b);
        mesh.indices.push_back(c);
    };

    for (int j = 0; j + 1 < m; ++j) {
        for (int i = 0; i < n; ++i) {
            const int in = (i + 1) % n;
            const uint32_t a = (j == 0) ? seamId(i) : lowerId(j, i);
            const uint32_t b = (j == 0) ? seamId(in) : lowerId(j, in);
            const uint32_t c = lowerId(j + 1, in);
            const uint32_t d = lowerId(j + 1, i);
            out.quads.push_back({a, b, c, d});
            // Lower patch: rows descend with j -> (a,b,c)/(a,c,d) is outward.
            appendTri(out.combined, a, b, c);
            appendTri(out.combined, a, c, d);
        }
    }
    for (int k = 0; k + 1 < m; ++k) {
        for (int i = 0; i < n; ++i) {
            const int in = (i + 1) % n;
            const uint32_t a = (k == 0) ? seamId(i) : upperId(k, i);
            const uint32_t b = (k == 0) ? seamId(in) : upperId(k, in);
            const uint32_t c = upperId(k + 1, in);
            const uint32_t d = upperId(k + 1, i);
            out.quads.push_back({a, b, c, d});
            // Upper patch: rows ascend with k -> (a,c,b)/(a,d,c) is outward.
            appendTri(out.combined, a, c, b);
            appendTri(out.combined, a, d, c);
        }
    }

    CropBoundary crop;
    crop.label = "shoulder-root";
    crop.ringIndex = 0;
    crop.vertexCount = n;
    crop.edgeCount = n;
    crop.loopCount = 1;
    crop.closed = true;
    crop.y = 0.0f;
    out.crops.push_back(crop);

    // ---- Side A: the lower cylinder ---------------------------------------
    addRing(out.lower.mesh, 0, true);
    for (int j = 1; j < m; ++j) addRing(out.lower.mesh, j, false);
    for (int j = 0; j + 1 < m; ++j) {
        for (int i = 0; i < n; ++i) {
            const int in = (i + 1) % n;
            const uint32_t a = (j == 0) ? seamId(i) : lowerId(j, i);
            const uint32_t b = (j == 0) ? seamId(in) : lowerId(j, in);
            appendTri(out.lower.mesh, a, b, lowerId(j + 1, in));
            appendTri(out.lower.mesh, a, lowerId(j + 1, in), lowerId(j + 1, i));
        }
    }
    for (int i = 0; i < n; ++i) out.lower.boundary.push_back(seamId(i));

    // ---- Side B: the upper cone --------------------------------------------
    addRing(out.upper.mesh, 0, true);
    for (int k = 1; k < m; ++k) addRing(out.upper.mesh, k, true);
    // Reuse the seam ids; remap upper rows into a compact local index space.
    for (int i = 0; i < n; ++i) out.upper.boundary.push_back(seamId(i));
    for (int k = 0; k + 1 < m; ++k) {
        for (int i = 0; i < n; ++i) {
            const int in = (i + 1) % n;
            const uint32_t a = (k == 0) ? seamId(i) : static_cast<uint32_t>(n + (k - 1) * n + i);
            const uint32_t b = (k == 0) ? seamId(in) : static_cast<uint32_t>(n + (k - 1) * n + in);
            const uint32_t c = static_cast<uint32_t>(n + k * n + in);
            const uint32_t d = static_cast<uint32_t>(n + k * n + i);
            if (flipUpper) {
                // Reversed winding: inward normals, same-direction edge walk.
                appendTri(out.upper.mesh, a, b, c);
                appendTri(out.upper.mesh, a, c, d);
            } else {
                appendTri(out.upper.mesh, a, c, b);
                appendTri(out.upper.mesh, a, d, c);
            }
        }
    }

    // A tangential shift of the upper seam ring breaks position matching
    // without changing index order.
    if (ringOffsetFraction != 0.0f) {
        for (int i = 0; i < n; ++i) {
            const float theta = static_cast<float>(i) * dTheta + shift;
            out.upper.mesh.vertices[static_cast<size_t>(i)].position =
                Vec3{r * std::cos(theta), 0.0f, r * std::sin(theta)};
        }
    }

    return out;
}

} // namespace cross_seam_fixture
} // namespace v2
} // namespace shape
} // namespace pae
