#include "wp4_weights.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace pae::wp4 {
using ht::D3;

const char* weightMethodName(WeightMethod m) {
    switch (m) {
        case WeightMethod::Distance: return "pae-distance";
        case WeightMethod::Heat: return "pae-heat";
        case WeightMethod::Region: return "pae-region";
    }
    return "?";
}

// --- Region / side compatibility -------------------------------------------
static std::string groupPrefix(const std::string& g) {
    const auto dot = g.find('.');
    return dot == std::string::npos ? g : g.substr(0, dot);
}
static char groupSide(const std::string& g) {
    if (g.size() >= 2 && g[g.size() - 2] == '.') return g[g.size() - 1];
    return 0;  // side-neutral (torso)
}
static char vertexSide(const D3& p) {
    if (p.x > 0.02) return 'L';
    if (p.x < -0.02) return 'R';
    return 0;
}
static bool regionAllows(ht::Region r, const std::string& prefix) {
    switch (r) {
        case ht::Region::Head: case ht::Region::Neck: case ht::Region::Thorax:
        case ht::Region::Abdomen: case ht::Region::Pelvis:
            return prefix == "torso";
        case ht::Region::ShoulderGirdle:
            return prefix == "torso" || prefix == "shoulder";
        case ht::Region::UpperArm: case ht::Region::Forearm:
            return prefix == "arm" || prefix == "shoulder";
        case ht::Region::Hand:
            return prefix == "hand" || prefix == "arm";
        case ht::Region::Thigh: case ht::Region::LowerLeg:
            return prefix == "leg";
        case ht::Region::Foot:
            return prefix == "foot" || prefix == "leg";
        default:
            return true;
    }
}
static bool influenceAllowed(ht::Region vr, char vs, const std::string& deformGroup) {
    const std::string prefix = groupPrefix(deformGroup);
    if (!regionAllows(vr, prefix)) return false;
    const char bs = groupSide(deformGroup);
    if (bs != 0 && vs != 0 && bs != vs) return false;  // cross-side guard
    return true;
}

ht::SkinWeights generateWeights(const ht::PolyMesh& body, const ht::Topology& topo, const RigV2& rig,
                                WeightMethod method, const WeightOptions& opt) {
    const ht::Rig hr = rig.asHtRig();
    const int smooth = opt.smoothingIterations > 0 ? opt.smoothingIterations : 2;
    const double dt = opt.diffusionTime > 0 ? opt.diffusionTime : 8.0;
    const int hi = opt.heatIterations > 0 ? opt.heatIterations : 60;
    const double hp = opt.heatPower > 0 ? opt.heatPower : 4.0;

    ht::SkinWeights sw;
    if (method == WeightMethod::Heat) {
        sw = ht::generateHeatWeights(body, topo, hr, dt, hi, hp);
    } else {
        sw = ht::generateDistanceWeights(body, topo, hr, smooth);
    }
    if (method != WeightMethod::Region) {
        sw.generator = weightMethodName(method);
        return sw;
    }

    // Region-constrained: filter distance influences by region + side, renorm;
    // empty vertices fall back to the nearest allowed bone segment.
    const std::vector<ht::Region> regions = ht::labelRegions(body, hr);
    const std::vector<double> radii = ht::boneRadii(body, hr);
    const size_t vc = body.pos.size(), bc = rig.bones.size();
    std::vector<std::string> prefix(bc), side(bc);
    for (size_t b = 0; b < bc; ++b) {
        prefix[b] = groupPrefix(rig.deformGroup[b]);
        side[b] = std::string(1, groupSide(rig.deformGroup[b]) ? groupSide(rig.deformGroup[b]) : ' ');
    }
    long reassigned = 0;
    for (size_t v = 0; v < vc; ++v) {
        const ht::Region vr = regions[v];
        const char vs = vertexSide(body.pos[v]);
        std::array<uint16_t, 4> nb{};
        std::array<float, 4> nw{};
        int kept = 0;
        double sum = 0.0;
        for (int k = 0; k < sw.count[v]; ++k) {
            const int b = sw.bone[v][static_cast<size_t>(k)];
            if (b < 0 || static_cast<size_t>(b) >= bc) continue;
            if (!influenceAllowed(vr, vs, rig.deformGroup[static_cast<size_t>(b)])) continue;
            nb[static_cast<size_t>(kept)] = static_cast<uint16_t>(b);
            nw[static_cast<size_t>(kept)] = sw.w[v][static_cast<size_t>(k)];
            sum += nw[static_cast<size_t>(kept)];
            ++kept;
        }
        if (kept == 0 || sum <= 0.0) {
            // nearest allowed bone segment
            int best = -1; double bestD = 1e300;
            for (size_t b = 0; b < bc; ++b) {
                if (!influenceAllowed(vr, vs, rig.deformGroup[b])) continue;
                const double d = ht::distanceToSegment(body.pos[v], rig.bones[b].head, rig.bones[b].tail)
                                 / std::max(radii[b], 1e-4);
                if (d < bestD) { bestD = d; best = static_cast<int>(b); }
            }
            if (best < 0) best = 0;
            nb[0] = static_cast<uint16_t>(best); nw[0] = 1.0f; kept = 1; sum = 1.0;
            ++reassigned;
        }
        for (int k = 0; k < kept; ++k) nw[static_cast<size_t>(k)] = static_cast<float>(nw[static_cast<size_t>(k)] / sum);
        sw.bone[v] = nb; sw.w[v] = nw; sw.count[v] = static_cast<uint8_t>(kept);
        sw.fallback[v] = static_cast<uint8_t>(ht::WeightFallback::Source);
    }
    sw.generator = "pae-region";
    sw.source += " (region-constrained; reassigned=" + std::to_string(reassigned) + ")";
    return sw;
}

WeightDiagnostics diagnoseWeights(const ht::PolyMesh& body, const ht::Topology& topo, const RigV2& rig,
                                  const ht::SkinWeights& sw) {
    WeightDiagnostics d;
    const ht::Rig hr = rig.asHtRig();
    const std::vector<ht::Region> regions = ht::labelRegions(body, hr);
    const std::vector<double> radii = ht::boneRadii(body, hr);
    const size_t vc = body.pos.size(), bc = rig.bones.size();
    d.vertices = static_cast<long>(vc);
    d.discardedMass = sw.massLostByCompression;
    d.generationMs = sw.generationMs;
    d.minSum = 1e300; d.maxSum = 0.0;
    double domDistSum = 0.0; long domCount = 0;
    for (size_t v = 0; v < vc && v < static_cast<size_t>(sw.vertexCount); ++v) {
        const int n = sw.count[v];
        if (n == 0) { ++d.unbound; continue; }
        d.maxInfluences = std::max(d.maxInfluences, n);
        double sum = 0.0, bestW = -1.0; int bestB = -1;
        const char vs = vertexSide(body.pos[v]);
        for (int k = 0; k < n; ++k) {
            const int b = sw.bone[v][static_cast<size_t>(k)];
            const double w = sw.w[v][static_cast<size_t>(k)];
            sum += w;
            if (w > bestW) { bestW = w; bestB = b; }
            if (b >= 0 && static_cast<size_t>(b) < bc) {
                const char bs = groupSide(rig.deformGroup[static_cast<size_t>(b)]);
                if (bs != 0 && vs != 0 && bs != vs) { d.crossSideMass += w; ++d.crossSideVertices; }
            }
        }
        d.minSum = std::min(d.minSum, sum);
        d.maxSum = std::max(d.maxSum, sum);
        if (bestB >= 0 && static_cast<size_t>(bestB) < bc) {
            const double dist = (body.pos[v] - rig.bones[static_cast<size_t>(bestB)].head).length();
            domDistSum += dist; ++domCount;
            if (!influenceAllowed(regions[v], vs, rig.deformGroup[static_cast<size_t>(bestB)]))
                ++d.farBoundVertices;
        }
    }
    if (d.minSum > 1e299) d.minSum = 0.0;
    d.meanDominantDistance = domCount ? domDistSum / static_cast<double>(domCount) : 0.0;

    // Smoothness: mean L1 weight difference across mesh edges (sparse union).
    double l1sum = 0.0; long edges = 0;
    for (const auto& e : topo.edges) {
        const int a = e[0], b = e[1];
        if (a >= sw.vertexCount || b >= sw.vertexCount) continue;
        std::map<int, double> wa, wb;
        for (int k = 0; k < sw.count[a]; ++k) wa[sw.bone[a][static_cast<size_t>(k)]] += sw.w[a][static_cast<size_t>(k)];
        for (int k = 0; k < sw.count[b]; ++k) wb[sw.bone[b][static_cast<size_t>(k)]] += sw.w[b][static_cast<size_t>(k)];
        std::set<int> keys;
        for (const auto& kv : wa) keys.insert(kv.first);
        for (const auto& kv : wb) keys.insert(kv.first);
        double l1 = 0.0;
        for (int k : keys) l1 += std::abs(wa[k] - wb[k]);
        l1sum += l1; ++edges;
    }
    d.smoothnessL1 = edges ? l1sum / static_cast<double>(edges) : 0.0;
    return d;
}

std::vector<BoneWeightSummary> boneWeightSummaries(const RigV2& rig, const ht::SkinWeights& sw) {
    std::vector<BoneWeightSummary> out(rig.bones.size());
    for (size_t b = 0; b < rig.bones.size(); ++b) out[b].bone = rig.bones[b].name;
    for (size_t v = 0; v < static_cast<size_t>(sw.vertexCount); ++v) {
        for (int k = 0; k < sw.count[v]; ++k) {
            const int b = sw.bone[v][static_cast<size_t>(k)];
            if (b < 0 || static_cast<size_t>(b) >= out.size()) continue;
            out[static_cast<size_t>(b)].mass += sw.w[v][static_cast<size_t>(k)];
            out[static_cast<size_t>(b)].vertices += 1;
        }
    }
    return out;
}

} // namespace pae::wp4