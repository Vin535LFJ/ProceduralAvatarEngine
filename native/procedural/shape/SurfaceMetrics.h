#pragma once

// G2 (SPEC 8.2 / 8.3): surface and canonical-mesh metrics.
//
// The three error layers must stay SEPARATE (SPEC 8.2):
//
//   E1  Reference Surface -> Target Reference        (Layer A, topology free)
//   E2  Reference Surface -> Canonical Mesh           (Layer B prerequisite)
//   E3  Canonical Mesh -> Target Reference            (Layer B)
//
// E2 is the cost of the fixed-topology projection and must never be mixed
// into E1 or E3. This module provides the primitives all three need:
//
//   - point-to-TRIANGLE distance (not point-to-vertex): a canonical mesh is
//     a faceted surface, and measuring to its vertices would understate the
//     deviation by a whole cell;
//   - a common surface sampling rule so every route contributes the SAME
//     number of samples from the SAME distribution (SPEC 8.1 #2/#3);
//   - bidirectional Hausdorff / RMS.

#include "procedural/MeshData.h"
#include "skeleton/Skeleton.h"

#include <functional>
#include <vector>

namespace pae::shape {

// Bidirectional surface-to-surface distance.
//
//   hausdorff  = max( max_{a in A} min_{b in B} d(a,b),
//                      max_{b in B} min_{a in A} d(a,b) )
//   rms        = sqrt( mean_{b in B} ( min_{a in A} d(a,b) )^2 )
//   mean       = mean_{b in B} min_{a in A} d(a,b)
struct SurfaceDistance {
    double hausdorff = 0.0;
    double rms = 0.0;
    double mean = 0.0;
    double p95 = 0.0;
    double forwardHausdorff = 0.0;  // A -> B
    double backwardHausdorff = 0.0; // B -> A
    int sampleCountA = 0;
    int sampleCountB = 0;
    bool valid = false;
};

// Bidirectional distance between two sample sets.
SurfaceDistance measureSurfaceDistance(const std::vector<Vec3>& a,
                                       const std::vector<Vec3>& b);

// Distance from a point to a triangle mesh (point to the closest point ON the
// surface, not to the closest vertex).
double distanceToSurface(const MeshData& mesh, const Vec3& p);

// Statistics of the distance from a sample set to a triangle mesh.
struct MeshDistance {
    double hausdorff = 0.0;
    double rms = 0.0;
    double mean = 0.0;
    double p95 = 0.0;
    int sampleCount = 0;
    bool valid = false;
};

MeshDistance measureMeshDistance(const MeshData& mesh, const std::vector<Vec3>& samples);

// The common surface sampling rule (SPEC 8.1 #2 "same sampling distribution",
// #3 "same sample count").
//
// One sample per (height, azimuth) cell of the PoC-A torso band: a ray is
// cast outward from the body axis and the FIRST surface crossing is taken.
// "First" matters: the upper arm is a separate lobe further out, and the
// template must not drift onto it.
//
// The band is expressed in the same landmark frame as the template so every
// route samples exactly the same anatomical region.
struct SurfaceSamplingSpec {
    int rings = 52;    // vertical samples
    int segments = 104; // azimuth samples
    float yBottomOffset = -0.02f; // relative to the hip landmark
    float yTopOffset = 0.00f;     // relative to the chest landmark
};

// Samples an implicit surface (negative inside) with the common rule.
std::vector<Vec3> sampleImplicitSurface(const std::function<float(Vec3)>& field,
                                        const BodyJoints& joints,
                                        const SurfaceSamplingSpec& spec);

// Samples a parametric surface given as f(v, u) -> point, with the same
// (height, azimuth) band. v runs over the band, u around the body.
std::vector<Vec3> sampleParametricSurface(const std::function<Vec3(float, float)>& surface,
                                           const BodyJoints& joints,
                                           const SurfaceSamplingSpec& spec);

// A surface sampled on the common (ring, segment) grid.
//
// The flat sample lists above lose the cell a sample came from, which makes
// "compare route A against route B at the SAME anatomical point" impossible -
// a flat list can only be compared with a nearest-neighbour search, and that
// measures the wrong thing (it hides a systematic radial offset behind the
// nearest point). The grid keeps the cell, so the landmark, normal and section
// metrics all compare the SAME (height, azimuth) cell of both surfaces.
struct SurfaceGrid {
    int rings = 0;
    int segments = 0;
    float yBottom = 0.0f;
    float yTop = 0.0f;
    std::vector<Vec3> positions;  // rings * segments, row major (ring, segment)
    std::vector<Vec3> normals;    // unit normals, pointing away from the body axis
    std::vector<char> valid;      // 0 when the ray found no crossing

    int index(int ring, int segment) const { return ring * segments + segment; }
    bool isValid(int ring, int segment) const {
        if (ring < 0 || ring >= rings || segment < 0 || segment >= segments) return false;
        return valid[static_cast<size_t>(index(ring, segment))] != 0;
    }
    // Height of a grid ring.
    float ringY(int ring) const;
    // The grid ring whose height is closest to `y`.
    int ringForHeight(float y) const;
    // The grid segment whose azimuth is closest to `u` (u in [0,1)).
    int segmentForAzimuth(float u) const;
};

// Samples an implicit surface (negative inside) on the common grid.
// The normal is the normalised SDF gradient, i.e. the exact isosurface
// normal of the continuous field.
SurfaceGrid sampleImplicitSurfaceGrid(const std::function<float(Vec3)>& field,
                                      const BodyJoints& joints,
                                      const SurfaceSamplingSpec& spec);

// Samples a parametric surface on the common grid. The normal is taken from
// the supplied normal function, or - when it is empty - from central
// differences of the surface function, and is then oriented away from the
// body axis so that every route's normals point the same way.
SurfaceGrid sampleParametricSurfaceGrid(const std::function<Vec3(float, float)>& surface,
                                         const std::function<Vec3(float, float)>& normal,
                                         const BodyJoints& joints,
                                         const SurfaceSamplingSpec& spec);

// Distribution of a per-sample quantity (used for the angle / distance
// histograms of SPEC 13 #6 and #7).
struct Distribution {
    double p50 = 0.0;
    double p95 = 0.0;
    double max = 0.0;
    double mean = 0.0;
    int count = 0;
};

Distribution summarize(std::vector<double> values);

} // namespace pae::shape
