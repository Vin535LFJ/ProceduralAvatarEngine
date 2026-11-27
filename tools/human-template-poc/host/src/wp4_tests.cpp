// WP4-HOST-RIG-01 unit tests: rig table integrity + CPU LBS math + weight
// diagnostics. Pure synthetic geometry; no MakeHuman assets required.

#include "ht_common.h"
#include "ht_obj.h"
#include "ht_topology.h"
#include "wp4_quality.h"
#include "wp4_rig.h"
#include "wp4_skin.h"
#include "wp4_weights.h"

#include <cmath>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

using namespace pae;
using ht::D3;
using ht::Affine;
using ht::fmt;

namespace {
int gPass = 0, gFail = 0;
void check(bool ok, const char* name, const std::string& detail = "") {
    if (ok) { ++gPass; std::printf("[PASS] %s\n", name); }
    else { ++gFail; std::printf("[FAIL] %s %s\n", name, detail.c_str()); }
}
bool near(double a, double b, double tol) { return std::fabs(a - b) <= tol; }

// Two-bone vertical chain: a (0,0,0)->(0,1,0), b (0,1,0)->(0,2,0).
wp4::RigV2 twoBone() {
    wp4::RigV2 r;
    r.profile = wp4::Profile::Full92;
    ht::RigBone a; a.name = "a"; a.parent = -1; a.head = D3(0, 0, 0); a.tail = D3(0, 1, 0); a.role = ht::Role::Spine;
    ht::RigBone b; b.name = "b"; b.parent = 0; b.head = D3(0, 1, 0); b.tail = D3(0, 2, 0); b.role = ht::Role::Spine;
    r.bones = {a, b};
    r.deformGroup = {"torso", "torso"};
    r.ax = {D3(1, 0, 0), D3(1, 0, 0)};
    r.ay = {D3(0, 1, 0), D3(0, 1, 0)};
    r.az = {D3(0, 0, 1), D3(0, 0, 1)};
    r.restGlobal = {Affine::identity(), Affine::translation(D3(0, 1, 0))};
    r.restLocal = {Affine::identity(), Affine::translation(D3(0, 1, 0))};
    return r;
}

ht::SkinWeights manualWeights(int vc, const std::vector<std::array<uint16_t, 4>>& bone,
                              const std::vector<std::array<float, 4>>& w, const std::vector<uint8_t>& cnt) {
    ht::SkinWeights sw;
    sw.vertexCount = vc; sw.bone = bone; sw.w = w; sw.count = cnt;
    sw.fallback.assign(static_cast<size_t>(vc), 0);
    return sw;
}
} // namespace

int main() {
    // --- table integrity ---
    const auto& table = wp4::boneTable();
    check(table.size() == 92, "table_full92_count", std::to_string(table.size()));
    int core = 0; std::set<std::string> ids; bool uniq = true, parentsOk = true;
    std::set<std::string> all;
    for (const auto& b : table) all.insert(b.id);
    for (const auto& b : table) {
        if (b.core) ++core;
        if (!ids.insert(b.id).second) uniq = false;
        if (*b.parent && !all.count(b.parent)) parentsOk = false;
    }
    check(core == 20, "table_core20_count", std::to_string(core));
    check(uniq, "table_unique_ids");
    check(parentsOk, "table_parents_exist");
    check(all.count("root") == 1, "table_has_root");

    // --- LBS math ---
    const wp4::RigV2 rig = twoBone();
    const std::vector<D3> rest = {D3(0, 0.5, 0), D3(0, 1.5, 0), D3(0, 2, 0)};
    const auto sw = manualWeights(3,
        {{0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}},
        {{1, 0, 0, 0}, {0.5f, 0.5f, 0, 0}, {1, 0, 0, 0}},
        {1, 2, 1});

    const double be = wp4::bindRecoveryError(rest, sw, rig);
    check(be < 1e-12, "lbs_bind_recovery", std::to_string(be));

    wp4::Pose pb{"bend", {{"b", 0, 90.0}}};  // rotate bone b about local X +90
    const auto g = wp4::poseGlobals(rig, pb);
    const auto skin = wp4::skinMatrices(rig, g);
    const auto posed = wp4::skinPositions(rest, sw, skin);
    check(near(posed[2].x, 0, 1e-9) && near(posed[2].y, 1, 1e-9) && near(posed[2].z, 1, 1e-9),
          "lbs_single_bone_rotation", fmt("(%.3f,%.3f,%.3f)", posed[2].x, posed[2].y, posed[2].z));
    check(near(posed[0].y, 0.5, 1e-9), "lbs_unaffected_vertex");
    // 50/50 vertex below the joint: bone a keeps it, bone b rotates it.
    check(near(posed[1].y, 1.25, 1e-9) && near(posed[1].z, 0.25, 1e-9), "lbs_weighted_blend",
          fmt("(%.3f,%.3f,%.3f)", posed[1].x, posed[1].y, posed[1].z));

    const std::vector<D3> rn = {D3(0, 0, 1), D3(0, 0, 1), D3(0, 0, 1)};
    const auto pn = wp4::skinNormals(rn, sw, skin);
    check(near(pn[2].y, -1, 1e-9) && near(pn[2].z, 0, 1e-9), "lbs_normal_rotation",
          fmt("(%.3f,%.3f,%.3f)", pn[2].x, pn[2].y, pn[2].z));

    // child inherits parent rotation: rotate a about X +90 -> b head moves.
    wp4::Pose pa{"parent", {{"a", 0, 90.0}}};
    const auto ga = wp4::poseGlobals(rig, pa);
    check(near(ga[1].t.z, 1, 1e-9) && near(ga[1].t.y, 0, 1e-9), "pose_parent_child_inheritance",
          fmt("(%.3f,%.3f,%.3f)", ga[1].t.x, ga[1].t.y, ga[1].t.z));

    // --- validation rejects bad rigs ---
    wp4::RigV2 zero = twoBone(); zero.bones[1].tail = zero.bones[1].head;
    check(!wp4::validateRig(zero).noZeroLength, "validate_zero_length");
    wp4::RigV2 cyc = twoBone(); cyc.bones[0].parent = 1;
    check(!wp4::validateRig(cyc).noCycle, "validate_cycle");

    // --- weight summaries mass conservation ---
    const auto sums = wp4::boneWeightSummaries(rig, sw);
    double mass = 0; for (const auto& s : sums) mass += s.mass;
    check(near(mass, 3.0, 1e-6), "weight_mass_conservation", std::to_string(mass));

    // --- cross-side diagnostic ---
    wp4::RigV2 sided = twoBone();
    sided.deformGroup = {"arm.L", "arm.R"};
    sided.bones[0].role = ht::Role::UpperArm; sided.bones[1].role = ht::Role::UpperArm;
    ht::PolyMesh body;
    body.pos = {D3(0.1, 0.5, 0), D3(0.1, 1, 0), D3(0.1, 1.5, 0), D3(0.1, 0.75, 0.1)};
    ht::PolyFace f; f.v = {0, 1, 2, 3}; f.vt = {-1, -1, -1, -1}; body.faces.push_back(f);
    ht::Topology topo; ht::buildTopology(body, topo);
    const auto swx = manualWeights(4,
        {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
        {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
        {1, 1, 1, 1});
    const auto diag = wp4::diagnoseWeights(body, topo, sided, swx);
    check(diag.crossSideVertices == 4 && near(diag.crossSideMass, 4.0, 1e-6), "diag_cross_side",
          fmt("verts=%ld mass=%.3f", diag.crossSideVertices, diag.crossSideMass));

    // --- region mask (distance) ---
    wp4::JointRegion jr{"test", D3(0, 1, 0), 0.3, -1, 0};
    const std::vector<ht::Region> labels(body.pos.size(), ht::Region::Thorax);
    const auto mask = wp4::regionMask(body, body.pos, labels, jr);
    check(mask[1] && !mask[0], "region_mask_distance");

    std::printf("== wp4 tests: %d passed, %d failed ==\n", gPass, gFail);
    return gFail == 0 ? 0 : 1;
}