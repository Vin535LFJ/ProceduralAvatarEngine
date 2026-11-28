// VR-1B M6 finger geometry refinement research (Host whitelist).
// Fist failure was attributed to geometry (inter-finger penetration + fold),
// weight purity normal. This study (1) measures finger loop density, (2) builds
// a conforming 4-way quad subdivision (adds finger loops / structured topology)
// with weight transfer, (3) compares fist per-finger flips + self-intersection
// base vs refined. Host-only; no Android, no DQS, no production template change.

#include "ht_common.h"
#include "ht_proof.h"
#include "ht_render.h"
#include "ht_rig.h"
#include "ht_topology.h"
#include "wp4_poses.h"
#include "wp4_rig.h"
#include "wp4_skin.h"
#include "wp4_weights.h"

#include "procedural/MeshData.h"
#include "procedural/humanoid/canonical/CanonicalTopology.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

using namespace pae;
using ht::D3;
using ht::fmt;

namespace {
std::string arg(int argc, char** argv, const std::string& k, const std::string& d) {
    for (int i = 1; i + 1 < argc; ++i) if (k == argv[i]) return argv[i + 1];
    return d;
}
// parse "finger.0F.0P.S" -> (F, S); else (-1, 0). F digit is at index 8.
std::pair<int, char> parseFinger(const std::string& id) {
    if (id.rfind("finger.0", 0) != 0 || id.size() < 14) return {-1, 0};
    const int f = id[8] - '0';
    const char side = id[id.size() - 1];
    if (f < 1 || f > 5 || (side != 'L' && side != 'R')) return {-1, 0};
    return {f, side};
}
int dominantBone(const ht::SkinWeights& sw, size_t v) {
    int best = -1; double bw = -1;
    for (int k = 0; k < sw.count[v]; ++k) if (sw.w[v][k] > bw) { bw = sw.w[v][k]; best = sw.bone[v][k]; }
    return best;
}
// merge weighted bone influences -> top-4 normalized
void mergeWeights(const std::vector<std::pair<int, double>>& in, std::array<uint16_t, 4>& bone,
                  std::array<float, 4>& w, uint8_t& count) {
    std::map<int, double> m;
    for (const auto& p : in) m[p.first] += p.second;
    std::vector<std::pair<double, int>> s;
    for (const auto& kv : m) s.push_back({kv.second, kv.first});
    std::sort(s.begin(), s.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
    bone = {0, 0, 0, 0}; w = {0, 0, 0, 0}; count = 0;
    double sum = 0;
    for (size_t i = 0; i < s.size() && i < 4; ++i) sum += s[i].first;
    if (sum <= 0) return;
    for (size_t i = 0; i < s.size() && i < 4; ++i) {
        bone[i] = static_cast<uint16_t>(s[i].second);
        w[i] = static_cast<float>(s[i].first / sum);
        ++count;
    }
}

struct FingerSubMesh {
    ht::PolyMesh mesh;
    ht::SkinWeights weights;
    std::vector<std::pair<int, char>> faceFinger;  // per face finger identity (or -1)
    std::vector<int> vertFingerF;                  // per vertex finger F (or -1)
    std::vector<char> vertFingerS;
};

// Conforming 4-way quad subdivision with weight transfer + finger inheritance.
FingerSubMesh subdivide(const ht::PolyMesh& body, const ht::SkinWeights& sw, const wp4::RigV2& rig) {
    FingerSubMesh out;
    out.mesh.pos = body.pos;
    out.weights.vertexCount = static_cast<int>(body.pos.size());
    out.weights.bone = sw.bone; out.weights.w = sw.w; out.weights.count = sw.count;
    out.weights.fallback = sw.fallback;
    out.vertFingerF.assign(body.pos.size(), -1);
    out.vertFingerS.assign(body.pos.size(), 0);
    for (size_t v = 0; v < body.pos.size(); ++v) {
        const int b = dominantBone(sw, v);
        if (b >= 0 && static_cast<size_t>(b) < rig.bones.size()) {
            const auto fc = parseFinger(rig.bones[static_cast<size_t>(b)].name);
            out.vertFingerF[v] = fc.first; out.vertFingerS[v] = fc.second;
        }
    }
    std::map<std::pair<int, int>, int> edgeMid;
    auto midpoint = [&](int a, int b) -> int {
        if (a > b) std::swap(a, b);
        const auto key = std::make_pair(a, b);
        auto it = edgeMid.find(key);
        if (it != edgeMid.end()) return it->second;
        const int id = static_cast<int>(out.mesh.pos.size());
        out.mesh.pos.push_back((body.pos[static_cast<size_t>(a)] + body.pos[static_cast<size_t>(b)]) * 0.5);
        std::vector<std::pair<int, double>> inf;
        for (int k = 0; k < sw.count[a]; ++k) inf.push_back({sw.bone[a][k], sw.w[a][k]});
        for (int k = 0; k < sw.count[b]; ++k) inf.push_back({sw.bone[b][k], sw.w[b][k]});
        std::array<uint16_t, 4> bn; std::array<float, 4> ww; uint8_t cc;
        mergeWeights(inf, bn, ww, cc);
        out.weights.bone.push_back(bn); out.weights.w.push_back(ww); out.weights.count.push_back(cc);
        out.weights.fallback.push_back(0);
        const int fa = out.vertFingerF[static_cast<size_t>(a)], fb = out.vertFingerF[static_cast<size_t>(b)];
        out.vertFingerF.push_back(fa == fb ? fa : -1);
        out.vertFingerS.push_back(fa == fb ? out.vertFingerS[static_cast<size_t>(a)] : 0);
        edgeMid[key] = id;
        return id;
    };
    for (const ht::PolyFace& f : body.faces) {
        if (f.v.size() != 4) { out.mesh.faces.push_back(f); out.faceFinger.push_back({-1, 0}); continue; }
        const int v0 = f.v[0], v1 = f.v[1], v2 = f.v[2], v3 = f.v[3];
        const int m01 = midpoint(v0, v1), m12 = midpoint(v1, v2), m23 = midpoint(v2, v3), m30 = midpoint(v3, v0);
        const int c = static_cast<int>(out.mesh.pos.size());
        out.mesh.pos.push_back((body.pos[static_cast<size_t>(v0)] + body.pos[static_cast<size_t>(v1)] +
                                body.pos[static_cast<size_t>(v2)] + body.pos[static_cast<size_t>(v3)]) * 0.25);
        std::vector<std::pair<int, double>> inf;
        for (int v : {v0, v1, v2, v3}) for (int k = 0; k < sw.count[v]; ++k) inf.push_back({sw.bone[v][k], sw.w[v][k]});
        std::array<uint16_t, 4> bn; std::array<float, 4> ww; uint8_t cc;
        mergeWeights(inf, bn, ww, cc);
        out.weights.bone.push_back(bn); out.weights.w.push_back(ww); out.weights.count.push_back(cc);
        out.weights.fallback.push_back(0);
        int ff = out.vertFingerF[static_cast<size_t>(v0)]; char fs = out.vertFingerS[static_cast<size_t>(v0)];
        for (int v : {v1, v2, v3}) if (out.vertFingerF[static_cast<size_t>(v)] != ff) { ff = -1; fs = 0; }
        out.vertFingerF.push_back(ff); out.vertFingerS.push_back(fs);
        auto quad = [&](int a, int b, int cc2, int d) {
            ht::PolyFace q; q.v = {a, b, cc2, d}; q.vt = {-1, -1, -1, -1};
            out.mesh.faces.push_back(q); out.faceFinger.push_back({ff, fs});
        };
        quad(v0, m01, c, m30); quad(v1, m12, c, m01); quad(v2, m23, c, m12); quad(v3, m30, c, m23);
    }
    out.weights.vertexCount = static_cast<int>(out.mesh.pos.size());
    out.weights.maxInfluences = 4;
    out.weights.generator = "subdiv-transfer";
    return out;
}

// per-finger flips on a posed mesh
std::map<std::string, long> perFingerFlips(const ht::PolyMesh& body, const std::vector<D3>& rest,
                                           const std::vector<D3>& posed, const std::vector<std::pair<int, char>>& faceFinger) {
    std::map<std::string, long> out;
    for (size_t f = 0; f < body.faces.size(); ++f) {
        const auto fc = faceFinger[f];
        if (fc.first < 0) continue;
        const auto& v = body.faces[f].v;
        D3 nr, np;
        const size_t k = v.size();
        for (size_t i = 0; i < k; ++i) {
            const D3& a = rest[v[i]]; const D3& b = rest[v[(i + 1) % k]];
            nr.x += (a.y - b.y) * (a.z + b.z); nr.y += (a.z - b.z) * (a.x + b.x); nr.z += (a.x - b.x) * (a.y + b.y);
            const D3& pa = posed[v[i]]; const D3& pb = posed[v[(i + 1) % k]];
            np.x += (pa.y - pb.y) * (pa.z + pb.z); np.y += (pa.z - pb.z) * (pa.x + pb.x); np.z += (pa.x - pb.x) * (pa.y + pb.y);
        }
        if (nr.normalized().dot(np.normalized()) < 0) {
            const std::string key = "finger.0" + std::to_string(fc.first) + "." + fc.second;
            out[key]++;
        }
    }
    return out;
}

size_t selfIntPairs(const ht::PolyMesh& body, const std::vector<D3>& pos) {
    pae::MeshData mesh; mesh.vertices.resize(pos.size());
    std::vector<D3> accum(pos.size());
    auto tri = [&](int a, int b, int c) {
        const D3 fn = (pos[b] - pos[a]).cross(pos[c] - pos[a]);
        accum[a] += fn; accum[b] += fn; accum[c] += fn;
        mesh.indices.push_back(a); mesh.indices.push_back(b); mesh.indices.push_back(c);
    };
    for (const auto& f : body.faces) {
        if (f.v.size() == 4) { tri(f.v[0], f.v[1], f.v[2]); tri(f.v[0], f.v[2], f.v[3]); }
        else for (size_t k = 1; k + 1 < f.v.size(); ++k) tri(f.v[0], f.v[k], f.v[k + 1]);
    }
    pae::Aabb bounds;
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        mesh.vertices[i].position = pae::Vec3(static_cast<float>(pos[i].x), static_cast<float>(pos[i].y), static_cast<float>(pos[i].z));
        const D3 n = accum[i].normalized();
        mesh.vertices[i].normal = pae::Vec3(static_cast<float>(n.x), static_cast<float>(n.y), static_cast<float>(n.z));
        bounds.expand(mesh.vertices[i].position);
    }
    mesh.bounds = bounds;
    return pae::canonical::detectSelfIntersections(mesh, 64).intersectingPairs;
}
} // namespace

int main(int argc, char** argv) {
    const std::string dataDir = arg(argc, argv, "--data", "");
    const std::string repoRoot = arg(argc, argv, "--repo", "");
    const std::string outDir = arg(argc, argv, "--out", "wp6finger-out");
    if (dataDir.empty() || repoRoot.empty()) { std::printf("usage: --data --repo --out\n"); return 2; }
    ht::makeDirs(outDir); ht::makeDirs(outDir + "/images");
    std::string log;
    auto say = [&](const std::string& s) { log += s + "\n"; std::printf("%s\n", s.c_str()); };

    ht::ProofContext ctx; std::string err;
    if (!ctx.load(dataDir, repoRoot, err)) { std::printf("load failed: %s\n", err.c_str()); return 1; }
    const int rootV = wp4::resolveRootAnchor(ctx.obj, ctx.full);
    wp4::JointSet js; wp4::resolveJoints(ctx.obj, ctx.full, rootV, js);
    wp4::RigV2 rig; wp4::buildRig(js, wp4::Profile::Full92, rig, err);
    wp4::WeightOptions opt;
    const ht::SkinWeights w0 = wp4::generateWeights(ctx.body, ctx.topo, rig, wp4::WeightMethod::Region, opt);
    say(fmt("[load] body verts=%zu faces=%zu", ctx.body.pos.size(), ctx.body.faces.size()));

    // ---- (1) finger topology measurement -----------------------------------
    std::string topoMd = "# M6 finger topology (base mesh, generated)\n\n";
    topoMd += "| finger | vertices | faces | mean verts/face-ring proxy |\n|---|---|---|---|\n";
    std::map<std::string, long> fVerts, fFaces;
    std::vector<std::pair<int, char>> baseFaceFinger(ctx.body.faces.size(), {-1, 0});
    for (size_t v = 0; v < ctx.body.pos.size(); ++v) {
        const int b = dominantBone(w0, v);
        if (b < 0) continue;
        const auto fc = parseFinger(rig.bones[static_cast<size_t>(b)].name);
        if (fc.first < 0) continue;
        fVerts["finger.0" + std::to_string(fc.first) + "." + fc.second]++;
    }
    for (size_t f = 0; f < ctx.body.faces.size(); ++f) {
        int ff = -1; char fs = 0; bool all = true;
        for (int v : ctx.body.faces[f].v) {
            const int b = dominantBone(w0, static_cast<size_t>(v));
            const auto fc = b >= 0 ? parseFinger(rig.bones[static_cast<size_t>(b)].name) : std::pair<int, char>{-1, 0};
            if (fc.first < 0) { all = false; break; }
            if (ff < 0) { ff = fc.first; fs = fc.second; }
            else if (fc.first != ff || fc.second != fs) { all = false; break; }
        }
        if (all && ff > 0) { baseFaceFinger[f] = {ff, fs}; fFaces["finger.0" + std::to_string(ff) + "." + fs]++; }
    }
    for (const auto& kv : fVerts)
        topoMd += fmt("| %s | %ld | %ld | %.2f |\n", kv.first.c_str(), kv.second, fFaces[kv.first],
                      fFaces[kv.first] ? static_cast<double>(kv.second) / fFaces[kv.first] : 0.0);
    topoMd += "\nLow verts-per-face and few longitudinal rings => fingers collapse/fold when curled (straw-like). ";
    topoMd += "Subdivision adds circumferential + longitudinal resolution.\n";
    ht::writeFile(outDir + "/finger-topology.md", topoMd);
    say("[topology] per-finger vertex/face counts written");

    // ---- (2) subdivision + weight transfer ---------------------------------
    const double t0 = ht::nowMs();
    FingerSubMesh sub = subdivide(ctx.body, w0, rig);
    say(fmt("[subdiv] verts %zu -> %zu, faces %zu -> %zu, %.0f ms", ctx.body.pos.size(), sub.mesh.pos.size(),
            ctx.body.faces.size(), sub.mesh.faces.size(), ht::nowMs() - t0));

    // weight purity of subdivided finger vertices (confound check): fraction of
    // weight mass on the correct finger bones. Low purity => transfer degradation.
    {
        double pure = 0, total = 0; long fv = 0;
        for (size_t v = 0; v < sub.mesh.pos.size(); ++v) {
            const int ff = sub.vertFingerF[v];
            if (ff < 0) continue;
            ++fv;
            const std::string pre = "finger.0" + std::to_string(ff) + ".";
            const std::string suf = std::string(".") + sub.vertFingerS[v];
            for (int k = 0; k < sub.weights.count[v]; ++k) {
                const int b = sub.weights.bone[v][k];
                const double w = sub.weights.w[v][k];
                total += w;
                if (b >= 0 && static_cast<size_t>(b) < rig.bones.size()) {
                    const std::string& id = rig.bones[static_cast<size_t>(b)].name;
                    if (id.rfind(pre, 0) == 0 && id.size() >= suf.size() && id.compare(id.size() - suf.size(), suf.size(), suf) == 0) pure += w;
                }
            }
        }
        const double purity = total > 0 ? pure / total : 1.0;
        say(fmt("[purity] subdiv finger vertices=%ld weight purity=%.4f (base Region purity was ~1.00)", fv, purity));
        ht::writeFile(outDir + "/subdiv-purity.md", fmt("# Subdivided finger weight purity\n\nfinger vertices: %ld\nweight purity (mass on correct finger bones): %.4f\n\nBase Region weights had purity ~1.00. A drop here would confound the flip comparison.\n", fv, purity));
    }

    // ---- (3) fist + finger_curl: base vs refined ---------------------------
    const auto suite = wp4::poseSuite();
    const int W = 360, H = 720;
    ht::Camera front = ht::makeCamera("front", 0, 0, W, H);
    { std::vector<ht::Camera> v{front}; std::vector<const std::vector<D3>*> s{&ctx.body.pos}; ht::fitSharedFrames(v, s); front = v[0]; }
    const auto regs = ht::labelRegions(ctx.body, rig.asHtRig());
    const ht::Rig hr = rig.asHtRig();
    std::string cmpCsv = "pose,mesh,verts,faces,globalFlips,selfIntPairs";
    for (int f = 1; f <= 5; ++f) for (char s : {'L', 'R'}) cmpCsv += ",finger.0" + std::to_string(f) + "." + s;
    cmpCsv += "\n";
    std::string cmpMd = "# M6 fist refinement: base vs subdivided (generated)\n\n";
    for (const std::string& pn : {std::string("fist"), std::string("finger_curl")}) {
        const wp4::Pose* pose = wp4::findPose(suite, pn);
        if (!pose) continue;
        const auto g = wp4::poseGlobals(rig, *pose);
        const auto skin = wp4::skinMatrices(rig, g);
        const auto posedBase = wp4::skinPositions(ctx.body.pos, w0, skin);
        const auto posedSub = wp4::skinPositions(sub.mesh.pos, sub.weights, skin);
        const auto dqBase = ht::compareSurfaces(ctx.body, posedBase, ctx.topo);
        ht::Topology subTopo; ht::buildTopology(sub.mesh, subTopo);
        const auto dqSub = ht::compareSurfaces(sub.mesh, posedSub, subTopo);
        const size_t siBase = selfIntPairs(ctx.body, posedBase);
        const size_t siSub = selfIntPairs(sub.mesh, posedSub);
        const auto pfBase = perFingerFlips(ctx.body, ctx.body.pos, posedBase, baseFaceFinger);
        const auto pfSub = perFingerFlips(sub.mesh, sub.mesh.pos, posedSub, sub.faceFinger);
        auto row = [&](const std::string& mesh, const std::vector<D3>& posed, long flips, size_t si, const std::map<std::string, long>& pf) {
            cmpCsv += fmt("%s,%s,%zu,%zu,%ld,%zu", pn.c_str(), mesh.c_str(), posed.size(), mesh == "base" ? ctx.body.faces.size() : sub.mesh.faces.size(), flips, si);
            for (int f = 1; f <= 5; ++f) for (char s : {'L', 'R'}) { const std::string k = "finger.0" + std::to_string(f) + "." + s; cmpCsv += fmt(",%ld", pf.count(k) ? pf.at(k) : 0); }
            cmpCsv += "\n";
        };
        row("base", posedBase, dqBase.flippedFaces, siBase, pfBase);
        row("subdiv", posedSub, dqSub.flippedFaces, siSub, pfSub);
        cmpMd += "## " + pn + fmt("\n\n| mesh | verts | faces | globalFlips | flipRate | selfIntPairs |\n|---|---|---|---|---|---|\n| base | %zu | %zu | %ld | %.4f | %zu |\n| subdiv | %zu | %zu | %ld | %.4f | %zu |\n\n",
                     ctx.body.pos.size(), ctx.body.faces.size(), dqBase.flippedFaces, static_cast<double>(dqBase.flippedFaces) / ctx.body.faces.size(), siBase,
                     sub.mesh.pos.size(), sub.mesh.faces.size(), dqSub.flippedFaces, static_cast<double>(dqSub.flippedFaces) / sub.mesh.faces.size(), siSub);
        cmpMd += "| finger | base flips | subdiv flips |\n|---|---|---|\n";
        for (int f = 1; f <= 5; ++f) for (char s : {'L', 'R'}) {
            const std::string k = "finger.0" + std::to_string(f) + "." + s;
            cmpMd += fmt("| %s | %ld | %ld |\n", k.c_str(), pfBase.count(k) ? pfBase.at(k) : 0, pfSub.count(k) ? pfSub.at(k) : 0);
        }
        cmpMd += "\n";
        // render closeups (hand region)
        ht::Camera cu = ht::makeCamera("cu", 25, 8, 320, 320);
        const D3 focus = js.group.at("joint-l-hand");
        ht::fitCameraToBox(cu, focus - D3(0.12, 0.12, 0.12), focus + D3(0.12, 0.12, 0.12));
        ht::RenderSurface sb{&ctx.body, &posedBase, &regs, &w0, &hr};
        ht::writePngFile(outDir + "/images/" + pn + "-base-closeup.png", ht::renderSurface(sb, cu, ht::ShadeMode::Clay, true));
        const auto regsSub = ht::labelRegions(sub.mesh, rig.asHtRig());
        ht::RenderSurface ss{&sub.mesh, &posedSub, &regsSub, &sub.weights, &hr};
        ht::writePngFile(outDir + "/images/" + pn + "-subdiv-closeup.png", ht::renderSurface(ss, cu, ht::ShadeMode::Clay, true));
        say(fmt("[pose] %-12s base flips=%ld selfInt=%zu | subdiv flips=%ld selfInt=%zu", pn.c_str(), dqBase.flippedFaces, siBase, dqSub.flippedFaces, siSub));
    }
    ht::writeFile(outDir + "/fist-refinement.csv", cmpCsv);
    ht::writeFile(outDir + "/fist-refinement.md", cmpMd);
    ht::writeFile(outDir + "/run.log", log);
    say("[done] M6 finger refinement study complete");
    ht::writeFile(outDir + "/run.log", log);
    return 0;
}