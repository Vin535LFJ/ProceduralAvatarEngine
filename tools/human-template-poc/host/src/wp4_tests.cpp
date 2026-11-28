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

// Build a rest frame exactly as wp4_rig.cpp buildRig does: y=norm(tail-head),
// z=anterior projected perp to y, x=y cross z; R columns=(x,y,z); restGlobal=T(head)*R.
void setFrame(wp4::RigV2& r, size_t i, const D3& head, const D3& tail) {
    const D3 y = (tail - head).normalized();
    D3 ant(0, 0, 1);
    D3 z = ant - y * ant.dot(y);
    if (z.length() < 0.05) { ant = D3(0, 1, 0); z = ant - y * ant.dot(y); }
    z = z.normalized();
    const D3 x = y.cross(z).normalized();
    r.ax[i] = x; r.ay[i] = y; r.az[i] = z;
    Affine R;
    R.m[0][0] = x.x; R.m[0][1] = y.x; R.m[0][2] = z.x;
    R.m[1][0] = x.y; R.m[1][1] = y.y; R.m[1][2] = z.y;
    R.m[2][0] = x.z; R.m[2][1] = y.z; R.m[2][2] = z.z;
    r.restGlobal[i] = Affine::translation(head) * R;
}
void finishRestLocal(wp4::RigV2& r) {
    for (size_t i = 0; i < r.bones.size(); ++i) {
        const int p = r.bones[i].parent;
        r.restLocal[i] = p >= 0 ? r.restGlobal[static_cast<size_t>(p)].inverse() * r.restGlobal[i] : r.restGlobal[i];
    }
}
// Single oblique bone: head origin, tail (0.6,0.8,0) => ax=(0.8,-0.6,0), ay=(0.6,0.8,0), az=(0,0,1).
wp4::RigV2 obliqueBone() {
    wp4::RigV2 r; r.profile = wp4::Profile::Full92;
    ht::RigBone b; b.name = "ob"; b.parent = -1; b.head = D3(0, 0, 0); b.tail = D3(0.6, 0.8, 0); b.role = ht::Role::Spine;
    r.bones = {b}; r.deformGroup = {"torso"};
    r.ax.resize(1); r.ay.resize(1); r.az.resize(1); r.restGlobal.resize(1); r.restLocal.resize(1);
    setFrame(r, 0, b.head, b.tail); finishRestLocal(r);
    return r;
}
// Extract world rotation axis from a skin matrix (rotation part), for angle ~90deg.
D3 skinAxis(const Affine& s) {
    return D3(s.m[2][1] - s.m[1][2], s.m[0][2] - s.m[2][0], s.m[1][0] - s.m[0][1]).normalized();
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

    // === M2-POSE-FRAME-AUDIT-01: non-axis-aligned frame tests ===
    // These expose the local-vs-world rotation-axis bug. With an oblique rest
    // frame (R != I), a local-axis rotation must rotate about the bone's REST
    // WORLD axis (rig.ax/ay/az), centered at head. Ground truth is computed
    // independently via Affine::rotation(restWorldAxis, angle).
    const wp4::RigV2 ob = obliqueBone();
    const D3 oax = ob.ax[0], oay = ob.ay[0], oaz = ob.az[0];
    auto checkLocalRot = [&](int axis, const D3& p, const char* name) {
        const D3 waxis = axis == 0 ? oax : (axis == 1 ? oay : oaz);
        const D3 expected = Affine::rotation(waxis, 3.14159265358979323846 / 2).apply(p);  // head at origin
        const auto sw1 = manualWeights(1, {{0, 0, 0, 0}}, {{1, 0, 0, 0}}, {1});
        wp4::Pose pose{"rot", {{"ob", axis, 90.0}}};
        const auto sk = wp4::skinMatrices(ob, wp4::poseGlobals(ob, pose));
        const auto posed = wp4::skinPositions({p}, sw1, sk);
        const double err = (posed[0] - expected).length();
        check(err < 1e-9, name, fmt("err=%.6f got(%.3f,%.3f,%.3f) exp(%.3f,%.3f,%.3f)", err,
              posed[0].x, posed[0].y, posed[0].z, expected.x, expected.y, expected.z));
        // actual world rotation axis must equal the rest world axis
        const D3 actualAxis = skinAxis(sk[0]);
        const double axisErr = (actualAxis - waxis).length();
        check(axisErr < 1e-6, (std::string(name) + "_axis").c_str(),
              fmt("axisErr=%.6f actual(%.3f,%.3f,%.3f) expect(%.3f,%.3f,%.3f)", axisErr,
                  actualAxis.x, actualAxis.y, actualAxis.z, waxis.x, waxis.y, waxis.z));
    };
    checkLocalRot(0, oay, "oblique_local_X");  // rotate ay about ax -> az
    checkLocalRot(1, oaz, "oblique_local_Y");  // rotate az about ay -> ax
    checkLocalRot(2, oax, "oblique_local_Z");  // rotate ax about az -> ay

    // rotation center fixed at head + no scale: head vertex stays, length preserved.
    {
        const auto sw2 = manualWeights(2, {{0, 0, 0, 0}, {0, 0, 0, 0}}, {{1, 0, 0, 0}, {1, 0, 0, 0}}, {1, 1});
        wp4::Pose pose{"rot", {{"ob", 0, 73.0}}};
        const auto sk = wp4::skinMatrices(ob, wp4::poseGlobals(ob, pose));
        const auto posed = wp4::skinPositions({D3(0, 0, 0), oay}, sw2, sk);
        check(posed[0].length() < 1e-9, "oblique_center_fixed", fmt("head moved %.6f", posed[0].length()));
        check(near(posed[1].length(), 1.0, 1e-9), "oblique_no_scale", fmt("len=%.6f", posed[1].length()));
    }

    // oblique bind recovery
    {
        const auto sw1 = manualWeights(1, {{0, 0, 0, 0}}, {{1, 0, 0, 0}}, {1});
        check(wp4::bindRecoveryError({oay}, sw1, ob) < 1e-12, "oblique_bind_recovery");
    }

    // 3-bone oblique chain: inheritance + child local axis follows parent + length.
    {
        wp4::RigV2 ch; ch.profile = wp4::Profile::Full92;
        ht::RigBone b0; b0.name = "c0"; b0.parent = -1; b0.head = D3(0, 0, 0); b0.tail = D3(0.6, 0.8, 0); b0.role = ht::Role::Spine;
        ht::RigBone b1; b1.name = "c1"; b1.parent = 0; b1.head = D3(0.6, 0.8, 0); b1.tail = D3(1.2, 1.6, 0.3); b1.role = ht::Role::Spine;
        ht::RigBone b2; b2.name = "c2"; b2.parent = 1; b2.head = D3(1.2, 1.6, 0.3); b2.tail = D3(1.5, 2.2, 0.9); b2.role = ht::Role::Spine;
        ch.bones = {b0, b1, b2}; ch.deformGroup = {"torso", "torso", "torso"};
        ch.ax.resize(3); ch.ay.resize(3); ch.az.resize(3); ch.restGlobal.resize(3); ch.restLocal.resize(3);
        setFrame(ch, 0, b0.head, b0.tail); setFrame(ch, 1, b1.head, b1.tail); setFrame(ch, 2, b2.head, b2.tail);
        finishRestLocal(ch);
        const auto swc = manualWeights(3, {{0, 0, 0, 0}, {1, 0, 0, 0}, {2, 0, 0, 0}}, {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}, {1, 1, 1});
        const std::vector<D3> restc = {b0.head, b1.head, b2.head};
        // rotate root only: children heads must follow rigidly, lengths preserved.
        wp4::Pose pr{"root", {{"c0", 0, 60.0}}};
        const auto gr = wp4::poseGlobals(ch, pr);
        const auto skr = wp4::skinMatrices(ch, gr);
        const auto posedr = wp4::skinPositions(restc, swc, skr);
        const double l01 = (posedr[1] - posedr[0]).length(), l01rest = (b1.head - b0.head).length();
        const double l12 = (posedr[2] - posedr[1]).length(), l12rest = (b2.head - b1.head).length();
        check(near(l01, l01rest, 1e-9) && near(l12, l12rest, 1e-9), "oblique_chain_root_length",
              fmt("l01 %.6f/%.6f l12 %.6f/%.6f", l01, l01rest, l12, l12rest));
        check(posedr[0].length() < 1e-9, "oblique_chain_root_center");
        // child local rotation after parent: c2 local Z rotation axis must be the
        // POSED world axis (parent motion applied), not the rest world axis.
        wp4::Pose pc{"combo", {{"c0", 0, 60.0}, {"c2", 2, 45.0}}};
        const auto gc = wp4::poseGlobals(ch, pc);
        // posed world axis of c2 local Z = rotation part of gc[2] * (0,0,1)
        const D3 posedZ = gc[2].rotateOnly(D3(0, 0, 1)).normalized();
        const D3 restZ = ch.az[2];
        // after parent rotation the posed Z must differ from rest Z (parent moved it)
        check((posedZ - restZ).length() > 1e-3, "oblique_child_axis_follows_parent",
              fmt("posedZ(%.3f,%.3f,%.3f) restZ(%.3f,%.3f,%.3f)", posedZ.x, posedZ.y, posedZ.z, restZ.x, restZ.y, restZ.z));
        // c2 skin rotation axis equals posed world Z (rotation about own posed axis)
        const auto skc = wp4::skinMatrices(ch, gc);
        // isolate c2 rotation: compare c2 global with/without its own rotation
        wp4::Pose pno{"nop", {{"c0", 0, 60.0}}};
        const auto gno = wp4::poseGlobals(ch, pno);
        const Affine rel = gno[2].inverse() * gc[2];  // c2 own rotation in its rest frame
        const D3 relAxis = skinAxis(rel);
        check((relAxis - D3(0, 0, 1)).length() < 1e-6, "oblique_child_local_axis_pure",
              fmt("relAxis(%.3f,%.3f,%.3f) expect(0,0,1)", relAxis.x, relAxis.y, relAxis.z));
    }

    // mirror symmetry: L/R oblique bones, same local rotation -> mirrored world motion.
    {
        wp4::RigV2 mir; mir.profile = wp4::Profile::Full92;
        ht::RigBone bl; bl.name = "L"; bl.parent = -1; bl.head = D3(0.2, 0, 0); bl.tail = D3(0.8, 0.8, 0); bl.role = ht::Role::UpperArm;
        ht::RigBone br; br.name = "R"; br.parent = -1; br.head = D3(-0.2, 0, 0); br.tail = D3(-0.8, 0.8, 0); br.role = ht::Role::UpperArm;
        mir.bones = {bl, br}; mir.deformGroup = {"arm.L", "arm.R"};
        mir.ax.resize(2); mir.ay.resize(2); mir.az.resize(2); mir.restGlobal.resize(2); mir.restLocal.resize(2);
        setFrame(mir, 0, bl.head, bl.tail); setFrame(mir, 1, br.head, br.tail); finishRestLocal(mir);
        const auto swm = manualWeights(2, {{0, 0, 0, 0}, {1, 0, 0, 0}}, {{1, 0, 0, 0}, {1, 0, 0, 0}}, {1, 1});
        // Mirror-correct correspondence: the buildRig frame convention uses a fixed
        // anterior (0,0,1) + right-hand rule, so L and R share the SAME world az and
        // have the same handedness (frames are NOT mirror images). Mirror-symmetric
        // world motion therefore requires OPPOSITE local signs (L +90, R -90), per
        // task §3.3. Same-sign local rotation gives same-sense world motion.
        wp4::Pose pm{"mir", {{"L", 2, 90.0}, {"R", 2, -90.0}}};
        const auto skm = wp4::skinMatrices(mir, wp4::poseGlobals(mir, pm));
        const D3 pl = bl.head + mir.ay[0];  // point along L bone Y
        const D3 pr = br.head + mir.ay[1];
        const auto posedm = wp4::skinPositions({pl, pr}, swm, skm);
        const D3 dl = posedm[0] - bl.head, dr = posedm[1] - br.head;
        // mirrored: dl.x == -dr.x, dl.y == dr.y, dl.z == dr.z
        check(near(dl.x, -dr.x, 1e-9) && near(dl.y, dr.y, 1e-9) && near(dl.z, dr.z, 1e-9),
              "oblique_mirror_symmetry", fmt("L(%.3f,%.3f,%.3f) R(%.3f,%.3f,%.3f)", dl.x, dl.y, dl.z, dr.x, dr.y, dr.z));
    }

    std::printf("== wp4 tests: %d passed, %d failed ==\n", gPass, gFail);
    return gFail == 0 ? 0 : 1;
}