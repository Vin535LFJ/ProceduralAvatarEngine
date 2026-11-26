#pragma once

// OBJ reader/writer for the host-only template proof.
//
// Contract (R1 audit requirement): vertex order is NEVER re-arranged. The
// Stable Vertex ID of the base body is the OBJ `v` line order (0-based).

#include "ht_common.h"

#include <map>
#include <string>
#include <vector>

namespace pae::ht {

struct PolyFace {
    std::vector<int> v;    // 0-based index into PolyMesh::pos
    std::vector<int> vt;   // 0-based index into PolyMesh::uv, -1 when absent
};

struct PolyMesh {
    std::string sourcePath;
    std::string sourceSha256;
    std::vector<D3> pos;
    std::vector<D2> uv;
    std::vector<PolyFace> faces;
    std::vector<std::string> faceGroup;  // OBJ `g` active when the face was read
    std::vector<int> faceLine;           // 1-based line number of the `f` statement
    std::vector<std::string> groupOrder; // groups in order of first appearance
    std::vector<std::string> useMtl;     // per face, empty when none
    int normalCount = 0;
    int commentLineCount = 0;
    int faceStatementsWithNegativeIndex = 0;

    // Group access
    std::vector<int> facesOfGroup(const std::string& group) const;
    std::vector<std::string> groupNamesWithPrefix(const std::string& prefix) const;

    // Compact sub-mesh made of the faces of one group. `usedVertex` receives
    // the stable vertex id (index into pos) of each compact vertex, so callers
    // can prove nothing was re-ordered.
    void extractGroup(const std::string& group, PolyMesh& out, std::vector<int>& usedVertex) const;
};

bool readObj(const std::string& path, PolyMesh& out, std::string& error);

// Writes a triangulated neutral OBJ (quads are split along the 0-2 diagonal).
// Vertex order of the input is preserved one-to-one.
bool writeTriangulatedObj(const std::string& path, const PolyMesh& mesh);

} // namespace pae::ht
