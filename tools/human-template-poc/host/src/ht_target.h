#pragma once

// MakeHuman `.target` reader and additive shape application for the
// host-only proof. The reader does NOT assume the documented conventions:
// the index base (0/1) and the axis order are detected from the data with
// reference targets, because MPFB2's format documentation describes its
// Blender import convention, which differs from the MakeHuman mesh frame.

#include "ht_common.h"
#include "ht_obj.h"

#include <string>
#include <vector>

namespace pae::ht {

struct ShapeTarget {
    std::string name;      // file stem
    std::string relPath;   // relative to the asset data directory
    std::string sha256;
    int indexBase = 1;     // index base that was used to resolve the rows
    std::vector<int> vertex;  // 0-based stable vertex id in the body mesh
    std::vector<D3> delta;    // displacement in mesh units, (x, y, z)
    std::vector<std::string> categoryHeaders;
    long dataRows = 0, rowsOutsideBody = 0, commentLines = 0, malformedLines = 0;
};

bool readTarget(const std::string& path, const std::string& relPath, long bodyVertexCount, int indexBase,
                ShapeTarget& out, std::string& error);

// Diagnostic: spatial spread (bbox diagonal) of the vertices a target moves,
// evaluated under both candidate index bases. The correct base produces the
// smaller spread for a local target such as head-trans-up.
struct IndexBaseProbe {
    double spreadBase0 = 0.0, spreadBase1 = 0.0;
    long outOfRangeBase0 = 0, outOfRangeBase1 = 0;
};
IndexBaseProbe probeIndexBase(const std::string& path, const std::vector<D3>& objPositions);
char dominantAxis(const ShapeTarget& t);  // 'x', 'y' or 'z'

void applyTargets(const std::vector<D3>& base,
                  const std::vector<std::pair<const ShapeTarget*, double>>& weighted,
                  std::vector<D3>& out);

struct DisplacementStats {
    long movedVertices = 0;  // |delta| > 1e-9 after weighting
    double l1 = 0.0, maxDisp = 0.0, meanDisp = 0.0;
};
DisplacementStats displacementStats(const std::vector<D3>& base, const std::vector<D3>& deformed);

} // namespace pae::ht
