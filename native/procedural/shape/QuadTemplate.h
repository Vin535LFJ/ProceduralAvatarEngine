#pragma once

// G1 (AUTH-07): fixed quad topology template for the PoC-A scope
// (SPEC T1 Torso Canonical Topology + T2 Shoulder-root Connection Topology).
//
// The template's vertex/index count NEVER depends on the shape parameters:
// that is the whole point of the template routes (R2 and R3). A route differs
// only in HOW the template vertices are placed:
//   R2 - ray projection onto the anatomy SDF isosurface
//   R3 - evaluation on a cubic B-spline control lattice built from the
//        anatomical envelope

#include "procedural/MeshData.h"
#include "procedural/humanoid/anatomy/AnatomyField.h"
#include "skeleton/Skeleton.h"

#include <array>
#include <functional>
#include <string>
#include <vector>

namespace pae::shape {

struct QuadTemplateSpec {
    int rings = 26;     // vertical quad rows
    int segments = 32;  // quads around
    float yBottomOffset = -0.02f; // metres relative to the hip landmark
    float yTopOffset = 0.00f;     // metres relative to the chest landmark
    // T2 (SPEC 10.1): the top ring is the SHOULDER-ROOT crop boundary and is
    // open by default - it is the explicit attachment ring a future upper arm
    // is stitched onto. Closing it would hide the boundary and lose T2.
    // The bottom ring is capped with a pelvis pole so the torso stays a closed
    // surface and volume measurements remain meaningful.
    bool capBottom = true;
    bool capTop = false;
};

struct TemplateTopology {
    int rings = 0;
    int segments = 0;
    int vertexCount = 0;
    int quadCount = 0;
    int triangleCount = 0;
};

TemplateTopology templateTopology(const QuadTemplateSpec& spec);

// An explicitly registered crop boundary of the template.
//
// SPEC 10.1 T2 requires the shoulder root to carry an EXPLICIT crop
// boundary. A crop boundary is a legitimate place where the mesh ends
// (boundary edges > 0); everything else must stay watertight.
struct CropBoundary {
    std::string label;      // "shoulder-root" / "pelvis-crop"
    int ringIndex = -1;     // template ring the boundary follows
    int vertexCount = 0;    // vertices on the boundary ring
    int edgeCount = 0;      // boundary edges contributed
    int loopCount = 0;      // boundary loops found on this ring
    bool closed = false;    // the ring forms one closed loop
    float y = 0.0f;         // world height of the boundary
};

struct TemplateMesh {
    MeshData mesh;
    std::vector<std::array<uint32_t, 4>> quads;
    std::vector<CropBoundary> cropBoundaries;
    bool valid = false;
    std::string error;
};

// Anatomical envelope cross-section (half width / half depth in metres).
struct EnvelopeSection {
    float halfWidth = 0.15f;
    float halfDepth = 0.10f;
};

EnvelopeSection envelopeAt(const anatomy::HumanAnatomyParameters& params,
                           const BodyJoints& joints, float y);

// R2: nominal template grid projected onto the anatomy SDF isosurface.
TemplateMesh projectTemplateOntoField(const QuadTemplateSpec& spec,
                                       const anatomy::HumanAnatomyParameters& params,
                                       const BodyJoints& joints);

// R3: cubic B-spline control lattice derived from the envelope.
TemplateMesh buildControlSurfaceTemplate(const QuadTemplateSpec& spec,
                                         const anatomy::HumanAnatomyParameters& params,
                                         const BodyJoints& joints);

// R3 control surface: a clamped (vertical) / periodic (horizontal) uniform
// cubic B-spline lattice.
//
// Vertical:   the first three and last three control rows are duplicated at
//             the hip and chest planes, so the surface passes THROUGH them.
// Horizontal: the control columns wrap around the body, so the ring closes
//             without a seam.
struct ControlSurface {
    int rows = 0;
    int cols = 0;
    std::vector<std::vector<Vec3>> points;
    float yBottom = 0.0f;
    float yTop = 0.0f;
};

ControlSurface buildControlSurface(const QuadTemplateSpec& spec,
                                   const anatomy::HumanAnatomyParameters& params,
                                   const BodyJoints& joints);

// Evaluates the R3 control surface at the template parameter (v, u).
//   v in [0,1] runs bottom -> top of the template band
//   u in [0,1) runs around the body
Vec3 evaluateControlSurface(const ControlSurface& surface, float v, float u);

// Radius of the FIRST isosurface lobe reached by marching outward from the
// body axis at height `y` and azimuth `u` (u in [0,1), 0 = +X).
//
// The upper arm is a SEPARATE lobe further out from the torso, so a plain
// "nearest surface" search would happily land on the arm and drag the
// template away from the body. The PoC-A reference region (torso + shoulder
// root, SPEC T1/T2) must therefore use the first crossing, which is the
// torso. Returns a negative value when no crossing is found.
float torsoRadiusAt(const std::function<float(Vec3)>& field, float y, float u,
                    float maxDistance = 0.55f);

float torsoRadiusAt(const anatomy::HumanAnatomyParameters& params,
                    const BodyJoints& joints, float y, float u,
                    float maxDistance = 0.55f);

// Shared helpers.
void computeVertexNormals(MeshData& mesh);
std::string writeObj(const MeshData& mesh, const std::string& path);

} // namespace pae::shape
