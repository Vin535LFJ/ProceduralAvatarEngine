#include "wp4_quality.h"

#include <algorithm>
#include <cmath>

namespace pae::wp4 {
using ht::D3;

static D3 faceNormalFrom(const std::vector<D3>& p, const ht::PolyFace& f) {
    D3 n;
    const size_t k = f.v.size();
    for (size_t i = 0; i < k; ++i) {
        const D3& a = p[static_cast<size_t>(f.v[i])];
        const D3& b = p[static_cast<size_t>(f.v[(i + 1) % k])];
        n.x += (a.y - b.y) * (a.z + b.z);
        n.y += (a.z - b.z) * (a.x + b.x);
        n.z += (a.x - b.x) * (a.y + b.y);
    }
    return n.normalized();
}
static double faceAreaFrom(const std::vector<D3>& p, const ht::PolyFace& f) {
    D3 c;
    const size_t k = f.v.size();
    for (size_t i = 1; i + 1 < k; ++i)
        c += (p[static_cast<size_t>(f.v[i])] - p[static_cast<size_t>(f.v[0])])
                 .cross(p[static_cast<size_t>(f.v[i + 1])] - p[static_cast<size_t>(f.v[0])]);
    return 0.5 * c.length();
}

std::vector<JointRegion> jointRegions(const RigV2& rig, const JointSet& js) {
    std::vector<JointRegion> out;
    auto add = [&](const std::string& name, const std::string& group, double r) {
        auto it = js.group.find(group);
        if (it == js.group.end()) return;
        out.push_back({name, it->second, r, -1, 0});
    };
    for (const char* side : {"L", "R"}) {
        const std::string low = std::string(side) == "L" ? "l" : "r";
        const std::string s(side);
        add("shoulder_axilla." + s, "joint-" + low + "-shoulder", 0.075);
        add("elbow." + s, "joint-" + low + "-elbow", 0.055);
        add("wrist." + s, "joint-" + low + "-hand", 0.045);
        add("hip_groin." + s, "joint-" + low + "-upper-leg", 0.075);
        add("knee." + s, "joint-" + low + "-knee", 0.065);
        out.push_back({"fingers." + s, D3{}, 0.0, static_cast<int>(ht::Region::Hand), side[0]});
        out.push_back({"toes." + s, D3{}, 0.0, static_cast<int>(ht::Region::Foot), side[0]});
    }
    (void)rig;
    return out;
}

std::vector<bool> regionMask(const ht::PolyMesh& body, const std::vector<D3>& rest,
                             const std::vector<ht::Region>& labels, const JointRegion& reg) {
    std::vector<bool> mask(rest.size(), false);
    if (reg.labelMask >= 0) {
        for (size_t v = 0; v < rest.size(); ++v) {
            if (static_cast<int>(labels[v]) != reg.labelMask) continue;
            const char vs = rest[v].x > 0.02 ? 'L' : (rest[v].x < -0.02 ? 'R' : 0);
            if (reg.side != 0 && vs != reg.side) continue;
            mask[v] = true;
        }
    } else {
        const double r2 = reg.radius * reg.radius;
        for (size_t v = 0; v < rest.size(); ++v)
            if ((rest[v] - reg.center).lengthSq() < r2) mask[v] = true;
    }
    return mask;
}

RegionMetrics evaluateRegion(const ht::PolyMesh& body, const ht::Topology& topo,
                             const std::vector<D3>& rest, const std::vector<D3>& posed,
                             const std::vector<bool>& mask, const std::string& name) {
    RegionMetrics m;
    m.name = name;
    for (bool b : mask) if (b) ++m.vertices;
    double restArea = 0.0, posedArea = 0.0;
    for (size_t f = 0; f < body.faces.size(); ++f) {
        bool any = false;
        for (int v : body.faces[f].v) if (mask[static_cast<size_t>(v)]) { any = true; break; }
        if (!any) continue;
        ++m.faces;
        const D3 nr = faceNormalFrom(rest, body.faces[f]);
        const D3 np = faceNormalFrom(posed, body.faces[f]);
        const double dot = nr.dot(np);
        m.minNormalDot = std::min(m.minNormalDot, dot);
        if (dot < 0.0) ++m.flippedFaces;
        restArea += faceAreaFrom(rest, body.faces[f]);
        posedArea += faceAreaFrom(posed, body.faces[f]);
    }
    m.areaRatio = restArea > 0 ? posedArea / restArea : 1.0;
    std::vector<double> ratios;
    for (const auto& e : topo.edges) {
        if (!mask[static_cast<size_t>(e[0])] || !mask[static_cast<size_t>(e[1])]) continue;
        const double lr = (rest[static_cast<size_t>(e[1])] - rest[static_cast<size_t>(e[0])]).length();
        const double lp = (posed[static_cast<size_t>(e[1])] - posed[static_cast<size_t>(e[0])]).length();
        if (lr > 1e-9) ratios.push_back(lp / lr);
    }
    if (!ratios.empty()) {
        std::sort(ratios.begin(), ratios.end());
        m.edgeRatioMax = ratios.back();
        m.edgeRatioP99 = ratios[static_cast<size_t>(std::min<double>(ratios.size() - 1, ratios.size() * 0.99))];
    }
    return m;
}

} // namespace pae::wp4