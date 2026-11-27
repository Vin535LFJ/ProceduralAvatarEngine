#pragma once

// WP2 static Render Mesh builder: PAEHumanTemplate (quad topology, metres,
// f64) -> engine MeshData (f32, triangles) for the existing GLES3 path.
//
// STATIC path only (VR-1B WP1/WP2): vertices carry joints={0}, weights={1,0,0,0}
// so the existing skinned shader renders the rest pose under an identity bone
// palette (the mode-1 precedent). Template weights are stored for later WPs but
// are NOT applied here. No LegacyRigV1 / UBO[22] / 64B packing change.
//
// Triangulation (decision D6): quads are split along the diagonal that
// maximises the minimum triangle angle (quality-based, deterministic; ties go
// to the 0-2 diagonal used by the host-proof export). Other polygons fan.
// Normals: area-weighted vertex normals from the triangulated surface.

#include "procedural/MeshData.h"
#include "procedural/human_template/PAEHumanTemplateFormat.h"

#include <string>

namespace pae::human_template {

struct StaticRenderMeshReport {
    uint32_t vertexCount = 0;
    uint32_t triangleCount = 0;
    uint32_t quadCount = 0;
    uint32_t otherFaceCount = 0;
    uint32_t degenerateTriangles = 0;
};

// Builds the static render mesh. Returns false with `error` on invalid input
// (empty topology, out-of-range indices). Deterministic output.
bool buildStaticRenderMesh(const HumanTemplate& t, MeshData& out, std::string& error);
bool buildStaticRenderMesh(const HumanTemplate& t, MeshData& out, StaticRenderMeshReport& report,
                           std::string& error);

} // namespace pae::human_template