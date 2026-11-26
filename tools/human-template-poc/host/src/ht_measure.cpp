#include "ht_measure.h"

#include <algorithm>

namespace pae::ht {

Measures measureBody(const std::vector<D3>& pos, const std::vector<Region>& regions, const Rig& rig) {
    Measures m;
    if (pos.empty()) return m;
    D3 lo{1e30, 1e30, 1e30}, hi{-1e30, -1e30, -1e30};
    for (const D3& p : pos) {
        lo.x = std::min(lo.x, p.x); lo.y = std::min(lo.y, p.y); lo.z = std::min(lo.z, p.z);
        hi.x = std::max(hi.x, p.x); hi.y = std::max(hi.y, p.y); hi.z = std::max(hi.z, p.z);
    }
    m.stature = hi.y - lo.y;
    const double sole = lo.y;

    // Joint-derived limb lengths (mean over left/right) and joint heights.
    auto meanDistance = [](const std::vector<D3>& a, const std::vector<D3>& b) {
        if (a.size() != b.size() || a.empty()) return 0.0;
        double sum = 0.0;
        for (size_t i = 0; i < a.size(); ++i) sum += (a[i] - b[i]).length();
        return sum / static_cast<double>(a.size());
    };
    auto meanHeight = [&](const std::vector<D3>& points) {
        double sum = 0.0;
        for (const D3& p : points) sum += p.y;
        return points.empty() ? 0.0 : sum / static_cast<double>(points.size()) - sole;
    };
    // Joint-to-joint lengths. The source rig splits limbs into a short "01"
    // joint bone and an "02" shaft bone: upperarm01 (2.8 cm) + upperarm02,
    // so the elbow is upperarm02.tail and the wrist is the head of wrist.L.
    std::vector<D3> shoulder, elbow, wrist, hipJoint, knee, ankle;
    for (const RigBone& b : rig.bones) {
        const bool second = b.name.find("02") != std::string::npos;
        if (b.role == Role::UpperArm && !second) shoulder.push_back(b.head);
        if (b.role == Role::UpperArm && second) elbow.push_back(b.tail);
        if (b.role == Role::Hand && b.name.compare(0, 5, "wrist") == 0) wrist.push_back(b.head);
        if (b.role == Role::Thigh && !second) hipJoint.push_back(b.head);
        if (b.role == Role::Thigh && second) knee.push_back(b.tail);
        if (b.role == Role::Foot && b.name.compare(0, 4, "foot") == 0) ankle.push_back(b.head);
    }
    m.upperArmLength = meanDistance(shoulder, elbow);
    m.forearmLength = meanDistance(elbow, wrist);
    m.thighLength = meanDistance(hipJoint, knee);
    m.shankLength = meanDistance(knee, ankle);
    m.shoulderJointHeight = meanHeight(shoulder);
    m.hipJointHeight = meanHeight(hipJoint);

    // Horizontal slice extents restricted to a region set.
    auto extent = [&](double y, std::vector<Region> allowed, double& width, double& depth) {
        const double half = 0.015 * m.stature;
        double xMin = 1e30, xMax = -1e30, zMin = 1e30, zMax = -1e30;
        for (size_t v = 0; v < pos.size() && v < regions.size(); ++v) {
            if (std::find(allowed.begin(), allowed.end(), regions[v]) == allowed.end()) continue;
            if (std::fabs(pos[v].y - y) > half) continue;
            xMin = std::min(xMin, pos[v].x); xMax = std::max(xMax, pos[v].x);
            zMin = std::min(zMin, pos[v].z); zMax = std::max(zMax, pos[v].z);
        }
        width = xMax >= xMin ? xMax - xMin : 0.0;
        depth = zMax >= zMin ? zMax - zMin : 0.0;
    };
    double unused = 0.0;
    extent(sole + m.shoulderJointHeight - 0.025 * m.stature, {Region::ShoulderGirdle, Region::UpperArm}, m.bideltoidWidth, unused);
    extent(sole + 0.72 * m.stature, {Region::Thorax}, m.chestWidth, m.chestDepth);
    extent(sole + 0.60 * m.stature, {Region::Abdomen}, m.waistWidth, m.waistDepth);
    extent(sole + m.hipJointHeight, {Region::Pelvis, Region::Thigh}, m.hipWidth, unused);
    return m;
}

std::vector<double> regionDisplacementShare(const std::vector<Region>& regions, const std::vector<int>& vertex,
                                            const std::vector<D3>& delta, double* helperShare) {
    std::vector<double> sum(static_cast<size_t>(kRegionCount), 0.0);
    double total = 0.0, helper = 0.0;
    for (size_t i = 0; i < vertex.size() && i < delta.size(); ++i) {
        const double len = delta[i].length();
        if (len <= 1e-9) continue;
        if (vertex[i] < 0 || vertex[i] >= static_cast<int>(regions.size())) { helper += len; continue; }
        sum[static_cast<size_t>(regions[static_cast<size_t>(vertex[i])])] += len;
        total += len;
    }
    if (total > 0.0)
        for (double& s : sum) s /= total;
    if (helperShare) *helperShare = (total + helper) > 0.0 ? helper / (total + helper) : 0.0;
    return sum;
}

std::vector<double> regionMeanDisplacement(const std::vector<Region>& regions, const std::vector<int>& vertex,
                                           const std::vector<D3>& delta) {
    std::vector<double> sum(static_cast<size_t>(kRegionCount), 0.0);
    std::vector<long> count(static_cast<size_t>(kRegionCount), 0);
    for (Region r : regions) ++count[static_cast<size_t>(r)];
    for (size_t i = 0; i < vertex.size() && i < delta.size(); ++i) {
        if (vertex[i] < 0 || vertex[i] >= static_cast<int>(regions.size())) continue;
        sum[static_cast<size_t>(regions[static_cast<size_t>(vertex[i])])] += delta[i].length();
    }
    for (size_t r = 0; r < sum.size(); ++r) sum[r] = count[r] ? sum[r] / static_cast<double>(count[r]) : 0.0;
    return sum;
}

bool displacedBoundingBox(const std::vector<D3>& pos, const std::vector<int>& vertex, const std::vector<D3>& delta,
                          D3& lo, D3& hi) {
    lo = D3{1e30, 1e30, 1e30};
    hi = D3{-1e30, -1e30, -1e30};
    bool any = false;
    for (size_t i = 0; i < vertex.size() && i < delta.size(); ++i) {
        if (delta[i].length() <= 1e-6 || vertex[i] < 0 || vertex[i] >= static_cast<int>(pos.size())) continue;
        const D3& p = pos[static_cast<size_t>(vertex[i])];
        lo.x = std::min(lo.x, p.x); lo.y = std::min(lo.y, p.y); lo.z = std::min(lo.z, p.z);
        hi.x = std::max(hi.x, p.x); hi.y = std::max(hi.y, p.y); hi.z = std::max(hi.z, p.z);
        any = true;
    }
    return any;
}

} // namespace pae::ht
