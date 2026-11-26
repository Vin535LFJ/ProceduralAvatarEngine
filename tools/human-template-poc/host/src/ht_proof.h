#pragma once

// Shared loading of the host-only proof inputs: base mesh body, topology,
// rig file, Rig V2 variants, region labels and the selected shape targets.
// All positions are converted to METRES on load (unit scale 0.1 m per mesh
// unit), so every later step works in PAE's unit system.

#include "ht_common.h"
#include "ht_measure.h"
#include "ht_obj.h"
#include "ht_rig.h"
#include "ht_target.h"
#include "ht_template.h"
#include "ht_topology.h"

#include <map>
#include <string>
#include <vector>

namespace pae::ht {

struct ShapeParam {
    std::string key;             // PAE parameter name
    std::string label;
    std::string expectedRegions; // regions the parameter is expected to move
    std::vector<std::string> incrFiles, decrFiles;  // relative to the data directory
    ShapeTarget incr, decr;      // merged when a side pair is used
    std::string note;            // caveats, e.g. "standalone macro target"
};

struct ProofContext {
    std::string dataDir;     // <repo>/makehuman/data
    std::string repoRoot;    // <repo> root (LICENSE files)
    double unitScale = 0.1;  // metres per mesh unit
    PolyMesh obj;            // full OBJ (body + helper + joint groups)
    std::vector<D3> full;    // every OBJ position in metres (targets also move helper geometry)
    PolyMesh body;           // compact "body" group, stable ids in bodyStableIds
    std::vector<int> bodyStableIds;
    Topology topo;
    RigFile rigFile;
    std::map<std::string, D3> joints;
    Rig rigDeform, rigCore;
    std::vector<Region> regions;
    std::vector<ShapeParam> params;
    std::vector<std::string> log;

    void info(const std::string& line) { log.push_back(line); }
    bool load(const std::string& dataDirectory, const std::string& repoRootDir, std::string& error);
};

std::string dataPath(const ProofContext& c, const std::string& rel);
std::string repoPath(const ProofContext& c, const std::string& rel);

// Shape targets move the WHOLE OBJ (body + helper + joint vertices). The
// joint helper cubes are what the rig is resolved on, so targets must be
// applied to the full surface; the body part is what gets measured/rendered.
void applyToFull(const ProofContext& c, const std::vector<std::pair<const ShapeTarget*, double>>& weighted,
                 std::vector<D3>& fullOut);
void applyToBody(const ProofContext& c, const std::vector<std::pair<const ShapeTarget*, double>>& weighted,
                 std::vector<D3>& bodyOut);
// Rig V2 re-resolved on a deformed full surface (metres).
bool rigOnSurface(const ProofContext& c, const std::vector<D3>& fullMetres, bool coreOnly, Rig& out);

// Merges left/right targets into one additive target.
ShapeTarget mergeTargets(const ShapeTarget& a, const ShapeTarget& b, const std::string& name);

// Helper-group census of the source OBJ (joint cubes, other helpers).
struct GroupCensus {
    int jointGroups = 0, otherHelperGroups = 0;
    long jointFaces = 0, otherHelperFaces = 0;
    std::string otherHelperNames;
};
GroupCensus censusGroups(const PolyMesh& obj);

// Assembles the PAEHumanTemplate from the loaded context: quad topology with
// stable ids, regions, landmarks (every rig joint + a few derived surface
// extremes), shape targets, the Rig V2 and the Top-4 weights.
HumanTemplate buildTemplate(const ProofContext& c, const Rig& rig, const SkinWeights& weights,
                            const std::vector<TemplateShapeTarget>& targets, const std::string& sourceCommit,
                            const std::vector<SourceFile>& files);

} // namespace pae::ht
