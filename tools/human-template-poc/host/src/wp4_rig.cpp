#include "wp4_rig.h"

#include <algorithm>
#include <deque>
#include <set>

namespace pae::wp4 {
using ht::D3;

const char* profileName(Profile p) { return p == Profile::Core20 ? "core20" : "full92"; }

// --- PAE authored semantic table -------------------------------------------
// Provenance: hierarchy/semantic ids authored by PAE for WP4. Joint positions
// come exclusively from CC0 base.obj joint groups. The PENDING default.mhskel
// was used once as a host research reference to cross-check deviations; it is
// NOT read by this module at runtime.
const std::vector<BoneDef>& boneTable() {
    static std::deque<std::string> pool;  // stable storage for generated names
    static std::vector<BoneDef> table;
    if (!table.empty()) return table;
    auto S = [&](const std::string& s) -> const char* { pool.push_back(s); return pool.back().c_str(); };

    // Central column + core girdles/limbs (explicit).
    table.push_back({"root", "", ht::Role::Root, "", "joint-pelvis", "torso", true});
    table.push_back({"pelvis.L", "root", ht::Role::Pelvis, "joint-pelvis", "joint-l-upper-leg", "torso", false});
    table.push_back({"pelvis.R", "root", ht::Role::Pelvis, "joint-pelvis", "joint-r-upper-leg", "torso", false});
    table.push_back({"spine.05", "root", ht::Role::Spine, "joint-pelvis", "joint-spine-4", "torso", true});
    table.push_back({"spine.04", "spine.05", ht::Role::Spine, "joint-spine-4", "joint-spine-3", "torso", false});
    table.push_back({"spine.03", "spine.04", ht::Role::Spine, "joint-spine-3", "joint-spine-2", "torso", true});
    table.push_back({"spine.02", "spine.03", ht::Role::Spine, "joint-spine-2", "joint-spine-1", "torso", false});
    table.push_back({"spine.01", "spine.02", ht::Role::Spine, "joint-spine-1", "joint-neck", "torso", true});
    table.push_back({"neck", "spine.01", ht::Role::Neck, "joint-neck", "joint-head", "torso", true});
    table.push_back({"head", "neck", ht::Role::Head, "joint-head", "joint-head-2", "torso", true});

    for (const char* side : {"L", "R"}) {
        const std::string s(side);
        const std::string low = s == "L" ? "l" : "r";
        const std::string shoulderG = "shoulder." + s, armG = "arm." + s, handG = "hand." + s;
        const std::string legG = "leg." + s, footG = "foot." + s;
        auto J = [&](const std::string& name) { return S("joint-" + low + "-" + name); };

        table.push_back({S("clavicle." + s), "spine.01", ht::Role::Clavicle, J("clavicle"), J("scapula"), S(shoulderG), true});
        table.push_back({S("shoulder." + s), S("clavicle." + s), ht::Role::Shoulder, J("scapula"), J("shoulder"), S(shoulderG), false});
        table.push_back({S("upper_arm." + s), S("shoulder." + s), ht::Role::UpperArm, J("shoulder"), J("elbow"), S(armG), true});
        table.push_back({S("forearm." + s), S("upper_arm." + s), ht::Role::Forearm, J("elbow"), J("hand"), S(armG), true});
        table.push_back({S("hand." + s), S("forearm." + s), ht::Role::Hand, J("hand"), J("hand-3"), S(handG), true});

        // Metacarpals 1..4 -> fingers 2..5 proximal.
        const char* mcHead[4] = {"hand-3", "hand-3", "hand-2", "hand-2"};
        for (int m = 1; m <= 4; ++m) {
            const std::string id = "metacarpal.0" + std::to_string(m) + "." + s;
            const std::string finger = "finger.0" + std::to_string(m + 1) + ".01." + s;
            table.push_back({S(id), S("hand." + s), ht::Role::Hand, J(mcHead[m - 1]),
                             J("finger-" + std::to_string(m + 1) + "-1"), S(handG), false});
            (void)finger;
        }
        // Fingers 1..5, phalanges 1..3 (finger.01 = thumb, parent = hand).
        for (int f = 1; f <= 5; ++f) {
            for (int p = 1; p <= 3; ++p) {
                const std::string id = "finger.0" + std::to_string(f) + ".0" + std::to_string(p) + "." + s;
                const std::string parent = p == 1
                    ? (f == 1 ? "hand." + s : "metacarpal.0" + std::to_string(f - 1) + "." + s)
                    : "finger.0" + std::to_string(f) + ".0" + std::to_string(p - 1) + "." + s;
                table.push_back({S(id), S(parent), ht::Role::Finger,
                                 J("finger-" + std::to_string(f) + "-" + std::to_string(p)),
                                 J("finger-" + std::to_string(f) + "-" + std::to_string(p + 1)), S(handG), false});
            }
        }
        // Leg.
        table.push_back({S("thigh." + s), S("pelvis." + s), ht::Role::Thigh, J("upper-leg"), J("knee"), S(legG), true});
        table.push_back({S("shin." + s), S("thigh." + s), ht::Role::Shin, J("knee"), J("ankle"), S(legG), true});
        table.push_back({S("foot." + s), S("shin." + s), ht::Role::Foot, J("ankle"), J("foot-1"), S(footG), true});
        // Toes: toe.01 has 2 phalanges, toes 02..05 have 3.
        for (int t = 1; t <= 5; ++t) {
            const int ph = t == 1 ? 2 : 3;
            for (int p = 1; p <= ph; ++p) {
                const std::string id = "toe.0" + std::to_string(t) + ".0" + std::to_string(p) + "." + s;
                const std::string parent = p == 1 ? "foot." + s
                    : "toe.0" + std::to_string(t) + ".0" + std::to_string(p - 1) + "." + s;
                table.push_back({S(id), S(parent), ht::Role::Toe,
                                 J("toe-" + std::to_string(t) + "-" + std::to_string(p)),
                                 J("toe-" + std::to_string(t) + "-" + std::to_string(p + 1)), S(footG), false});
            }
        }
    }
    return table;
}

// --- Joint resolution -------------------------------------------------------
static std::map<std::string, std::vector<int>> groupVertices(const ht::PolyMesh& obj) {
    std::map<std::string, std::vector<int>> gv;
    for (size_t f = 0; f < obj.faces.size(); ++f) {
        const std::string& g = obj.faceGroup[f];
        if (g.rfind("joint-", 0) != 0) continue;
        for (int v : obj.faces[f].v) gv[g].push_back(v);
    }
    for (auto& kv : gv) {
        std::sort(kv.second.begin(), kv.second.end());
        kv.second.erase(std::unique(kv.second.begin(), kv.second.end()), kv.second.end());
    }
    return gv;
}

int resolveRootAnchor(const ht::PolyMesh& obj, const std::vector<ht::D3>& full) {
    // PAE authored anchor: pelvis centroid + (0, -0.015, -0.092) m; nearest body
    // surface vertex at rest. Tracked by stable id afterwards.
    const auto gv = groupVertices(obj);
    auto it = gv.find("joint-pelvis");
    if (it == gv.end() || it->second.empty()) return -1;
    D3 pelvis;
    for (int v : it->second) pelvis += full[static_cast<size_t>(v)];
    pelvis = pelvis * (1.0 / static_cast<double>(it->second.size()));
    const D3 anchor = pelvis + D3(0.0, -0.015, -0.092);
    // body surface vertices = vertices referenced by the "body" group faces.
    std::set<int> bodyVerts;
    for (size_t f = 0; f < obj.faces.size(); ++f)
        if (obj.faceGroup[f] == "body")
            for (int v : obj.faces[f].v) bodyVerts.insert(v);
    int best = -1; double bestD = 1e300;
    for (int v : bodyVerts) {
        const double d = (full[static_cast<size_t>(v)] - anchor).lengthSq();
        if (d < bestD) { bestD = d; best = v; }
    }
    return best;
}

bool resolveJoints(const ht::PolyMesh& obj, const std::vector<ht::D3>& full, int rootVertex, JointSet& out) {
    out = JointSet{};
    const auto gv = groupVertices(obj);
    for (const auto& kv : gv) {
        D3 sum;
        for (int v : kv.second) sum += full[static_cast<size_t>(v)];
        out.group[kv.first] = sum * (1.0 / static_cast<double>(kv.second.size()));
    }
    // Every group referenced by the table must exist.
    std::set<std::string> needed;
    for (const BoneDef& b : boneTable()) {
        if (*b.headGroup) needed.insert(b.headGroup);
        if (*b.tailGroup) needed.insert(b.tailGroup);
    }
    for (const std::string& g : needed)
        if (!out.group.count(g)) out.unresolved.push_back(g);

    if (rootVertex < 0) rootVertex = resolveRootAnchor(obj, full);
    out.rootHeadVertex = rootVertex;
    if (rootVertex >= 0 && static_cast<size_t>(rootVertex) < full.size())
        out.rootHead = full[static_cast<size_t>(rootVertex)];
    else
        out.unresolved.push_back("root-anchor");

    // L/R symmetry: mirror x of the right joint, compare to left.
    for (const auto& kv : out.group) {
        const std::string& name = kv.first;
        if (name.rfind("joint-l-", 0) != 0) continue;
        const std::string rname = "joint-r-" + name.substr(8);
        auto r = out.group.find(rname);
        if (r == out.group.end()) continue;
        const D3 mir = D3(-r->second.x, r->second.y, r->second.z);
        const double dev = (kv.second - mir).length();
        out.maxLrAsymmetry = std::max(out.maxLrAsymmetry, dev);
        ++out.lrPairs;
    }
    return out.unresolved.empty();
}

// --- Rig build --------------------------------------------------------------
int RigV2::find(const std::string& id) const {
    for (size_t i = 0; i < bones.size(); ++i) if (bones[i].name == id) return static_cast<int>(i);
    return -1;
}

ht::Rig RigV2::asHtRig() const {
    ht::Rig r;
    r.name = std::string("PAE-") + profileName(profile);
    r.source = "pae-authored-table+cc0-joint-groups";
    r.license = "PAE-owned definition; joint positions from CC0 base.obj";
    r.formatVersion = 2;
    r.bones = bones;
    return r;
}

static ht::D3 boneFrameZ(const ht::D3& y) {
    // Z = anterior projected perpendicular to Y; fallback when near-parallel.
    D3 ant(0, 0, 1);
    D3 z = ant - y * ant.dot(y);
    if (z.length() < 0.05) {
        ant = D3(0, 1, 0);
        z = ant - y * ant.dot(y);
    }
    return z.normalized();
}

bool buildRig(const JointSet& js, Profile profile, RigV2& out, std::string& error, bool anatomicalShoulder) {
    out = RigV2{};
    out.profile = profile;
    out.version = anatomicalShoulder ? kRigVersionAnatomical : kRigVersion;
    const auto& table = boneTable();
    std::map<std::string, const BoneDef*> byId;
    for (const BoneDef& b : table) byId[b.id] = &b;

    // Local joint positions; adopt the anatomical shoulder center if requested.
    std::map<std::string, D3> jp = js.group;
    if (anatomicalShoulder) {
        for (const char* side : {"l", "r"}) {
            const std::string sh = std::string("joint-") + side + "-shoulder";
            const std::string el = std::string("joint-") + side + "-elbow";
            auto s = jp.find(sh), e = jp.find(el);
            if (s == jp.end() || e == jp.end()) continue;
            const D3 axis = (e->second - s->second).normalized();  // shoulder->elbow (distal)
            s->second = s->second + axis * (-kShoulderProximalOffsetM);  // proximal
        }
    }

    // Keep set: full = all; core = core-flagged.
    auto kept = [&](const BoneDef& b) { return profile == Profile::Full92 || b.core; };
    auto jointPos = [&](const char* group, bool isRootHead) -> D3 {
        if (isRootHead) return js.rootHead;
        auto it = jp.find(group);
        return it == jp.end() ? D3{} : it->second;
    };

    std::map<std::string, int> index;
    for (const BoneDef& b : table) {
        if (!kept(b)) continue;
        ht::RigBone rb;
        rb.name = b.id;
        rb.role = b.role;
        rb.head = jointPos(b.headGroup, std::string(b.id) == "root");
        rb.tail = jointPos(b.tailGroup, false);
        rb.headJoint = b.headGroup;
        rb.tailJoint = b.tailGroup;
        rb.deform = true;
        index[b.id] = static_cast<int>(out.bones.size());
        out.bones.push_back(rb);
        out.deformGroup.push_back(b.deformGroup);
    }
    // Parents: nearest kept ancestor in the authored hierarchy.
    for (size_t i = 0; i < out.bones.size(); ++i) {
        const BoneDef* def = byId[out.bones[i].name];
        std::string p = def->parent;
        int guard = 0;
        while (!p.empty() && !index.count(p) && guard++ < static_cast<int>(table.size()) + 1) {
            auto it = byId.find(p);
            if (it == byId.end()) { p.clear(); break; }
            p = it->second->parent;
        }
        out.bones[i].parent = index.count(p) ? index[p] : -1;
    }
    // Local frames + rest transforms.
    const size_t n = out.bones.size();
    out.ax.resize(n); out.ay.resize(n); out.az.resize(n);
    out.restGlobal.resize(n); out.restLocal.resize(n);
    for (size_t i = 0; i < n; ++i) {
        const ht::RigBone& b = out.bones[i];
        const D3 y = (b.tail - b.head).normalized();
        const D3 z = boneFrameZ(y);
        const D3 x = y.cross(z).normalized();
        out.ax[i] = x; out.ay[i] = y; out.az[i] = z;
        ht::Affine R;
        R.m[0][0] = x.x; R.m[0][1] = y.x; R.m[0][2] = z.x;
        R.m[1][0] = x.y; R.m[1][1] = y.y; R.m[1][2] = z.y;
        R.m[2][0] = x.z; R.m[2][1] = y.z; R.m[2][2] = z.z;
        out.restGlobal[i] = ht::Affine::translation(b.head) * R;
        out.bones[i].planeNormal = z;  // roll reference
    }
    for (size_t i = 0; i < n; ++i) {
        const int p = out.bones[i].parent;
        out.restLocal[i] = p >= 0 ? out.restGlobal[static_cast<size_t>(p)].inverse() * out.restGlobal[i]
                                  : out.restGlobal[i];
    }
    const RigValidation v = validateRig(out);
    if (!v.ok) {
        error = "rig validation failed:";
        for (const std::string& s : v.problems) error += " " + s;
        return false;
    }
    return true;
}

RigValidation validateRig(const RigV2& rig) {
    RigValidation v;
    v.boneCount = static_cast<int>(rig.bones.size());
    std::set<std::string> ids;
    v.minBoneLength = 1e300;
    for (size_t i = 0; i < rig.bones.size(); ++i) {
        const ht::RigBone& b = rig.bones[i];
        if (!ids.insert(b.name).second) { v.uniqueIds = false; v.problems.push_back("dup id " + b.name); }
        if (b.parent >= 0 && static_cast<size_t>(b.parent) >= rig.bones.size()) {
            v.parentsExist = false; v.problems.push_back("bad parent " + b.name);
        }
        const double len = (b.tail - b.head).length();
        v.minBoneLength = std::min(v.minBoneLength, len);
        if (len < 1e-6) { v.noZeroLength = false; v.problems.push_back("zero length " + b.name); }
    }
    // Cycle check via ancestor walk.
    for (size_t i = 0; i < rig.bones.size(); ++i) {
        std::set<int> seen;
        int cur = static_cast<int>(i);
        while (cur >= 0) {
            if (!seen.insert(cur).second) { v.noCycle = false; v.problems.push_back("cycle at " + rig.bones[i].name); break; }
            cur = rig.bones[static_cast<size_t>(cur)].parent;
        }
    }
    v.ok = v.uniqueIds && v.noCycle && v.parentsExist && v.noZeroLength && v.boneCount > 0;
    return v;
}

std::vector<JointDeviation> compareToReference(const RigV2& pae, const ht::Rig& reference) {
    // Map PAE semantic ids to the reference source names for the shared bones.
    static const std::map<std::string, std::string> map = {
        {"root", "root"}, {"spine.05", "spine05"}, {"spine.04", "spine04"}, {"spine.03", "spine03"},
        {"spine.02", "spine02"}, {"spine.01", "spine01"}, {"neck", "neck01"}, {"head", "head"},
        {"clavicle.L", "clavicle.L"}, {"clavicle.R", "clavicle.R"},
        {"shoulder.L", "shoulder01.L"}, {"shoulder.R", "shoulder01.R"},
        {"upper_arm.L", "upperarm01.L"}, {"upper_arm.R", "upperarm01.R"},
        {"forearm.L", "lowerarm01.L"}, {"forearm.R", "lowerarm01.R"},
        {"hand.L", "wrist.L"}, {"hand.R", "wrist.R"},
        {"thigh.L", "upperleg01.L"}, {"thigh.R", "upperleg01.R"},
        {"shin.L", "lowerleg01.L"}, {"shin.R", "lowerleg01.R"},
        {"foot.L", "foot.L"}, {"foot.R", "foot.R"}, {"pelvis.L", "pelvis.L"}, {"pelvis.R", "pelvis.R"},
    };
    std::vector<JointDeviation> out;
    for (const ht::RigBone& b : pae.bones) {
        auto it = map.find(b.name);
        if (it == map.end()) continue;
        const int r = reference.find(it->second);
        if (r < 0) continue;
        JointDeviation d;
        d.bone = b.name;
        d.headDev = (b.head - reference.bones[static_cast<size_t>(r)].head).length();
        d.tailDev = (b.tail - reference.bones[static_cast<size_t>(r)].tail).length();
        out.push_back(d);
    }
    return out;
}

} // namespace pae::wp4