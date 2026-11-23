#pragma once

// G2 (SPEC 8.4 / 13 #1): the fixed camera protocol.
//
// Every route is measured from the SAME seven fixed views with the SAME
// orthographic projection and the SAME silhouette extraction, so a silhouette
// difference can only come from the geometry and never from the camera.
//
// Coordinate system (SPEC 8.1 #7): metres, right handed, +Y up, +Z forward,
// +X character left.

#include "procedural/MeshData.h"

#include <string>
#include <vector>

namespace pae::shape {

struct View {
    std::string name;      // "front", "side-left", ...
    float azimuthDeg = 0.0f;  // 0 = looking at the front (+Z), 90 = from the character's left
    float elevationDeg = 0.0f;
};

// The seven fixed views of SPEC 8.4 / 13 #1.
const std::vector<View>& fixedViews();

// Orthonormal image frame of a fixed view.
//   right = image horizontal axis, up = image vertical axis,
//   dir   = direction the camera looks along (camera -> body).
// Exposed so the grey-clay renderer uses the EXACT same camera as the numeric
// silhouette protocol - never a separately constructed approximation.
struct ViewFrame {
    Vec3 right{1.0f, 0.0f, 0.0f};
    Vec3 up{0.0f, 1.0f, 0.0f};
    Vec3 dir{0.0f, 0.0f, -1.0f};
};

ViewFrame viewFrame(const View& view);

// Orthographic projection of a mesh into a view's image plane.
//   u = horizontal image axis, v = vertical image axis
// The returned points are in METRES, in the image plane.
std::vector<Vec3> projectView(const MeshData& mesh, const View& view);

// The silhouette of a mesh seen from a view: the outer radius of the shape
// in each angular bin around the body axis projection.
//
// The axis is the vertical body line, so the silhouette is a radial profile -
// the same representation for every route, which is what makes the
// route-to-route comparison meaningful.
//
// The mesh is CLIPPED to the shared PoC-A band (yMin..yMax) before the
// silhouette is taken. Without the clip, a route that only covers the torso
// would be compared against a reference that also has a head and arms, and
// the difference would measure anatomy that the route never claimed to
// produce.
struct Silhouette {
    static constexpr int kBins = 180;
    std::vector<double> radius;   // kBins entries, NaN where the bin is empty
    int filledBins = 0;
    int clippedVertexCount = 0;
};

Silhouette extractSilhouette(const MeshData& mesh, const View& view,
                             const Vec3& axisLowWorld, const Vec3& axisHighWorld,
                             float yMin, float yMax);

// Signed radial distance between two silhouettes at matching bins
// (SPEC 13 #1: "signed distance distribution, p50 / p95 / max").
struct SilhouetteDelta {
    double p50 = 0.0;
    double p95 = 0.0;
    double maxAbs = 0.0;
    double meanAbs = 0.0;
    int comparedBins = 0;
    bool valid = false;
};

SilhouetteDelta compareSilhouettes(const Silhouette& reference, const Silhouette& other);

} // namespace pae::shape
