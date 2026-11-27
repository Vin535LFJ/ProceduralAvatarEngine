#include "wp3_measure.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <utility>

using namespace pae::ht;

namespace pae::wp3 {

namespace {

constexpr double kEps = 1e-9;

// Intersect one triangle with the plane; append the segment (if any).
void cutTriangle(const D3& a, const D3& b, const D3& c, const D3& n, const D3& p,
                 std::vector<std::pair<D3, D3>>& segments, std::vector<D3>& points) {
    const D3 v[3] = {a, b, c};
    const double d[3] = {n.dot(a - p), n.dot(b - p), n.dot(c - p)};
    // Degenerate case: an edge lies exactly in the plane. Emit it once, from
    // the triangle whose third vertex is on the positive side (the adjacent
    // triangle on the negative side skips it), so the perimeter is not doubled.
    int onPlane = 0;
    int onIdx[2] = {0, 0};
    for (int i = 0; i < 3; ++i) if (std::fabs(d[i]) <= kEps && onPlane < 2) onIdx[onPlane++] = i;
    if (onPlane == 2) {
        const int third = 3 - onIdx[0] - onIdx[1];
        if (d[third] > kEps && (v[onIdx[0]] - v[onIdx[1]]).length() > 1e-10) {
            segments.push_back({v[onIdx[0]], v[onIdx[1]]});
            points.push_back(v[onIdx[0]]);
            points.push_back(v[onIdx[1]]);
        }
        return;
    }
    bool pos = false, neg = false;
    for (int i = 0; i < 3; ++i) {
        if (d[i] > kEps) pos = true;
        if (d[i] < -kEps) neg = true;
    }
    if (!pos || !neg) return;  // entirely on one side (or on the plane)
    D3 hit[2];
    int nh = 0;
    for (int i = 0; i < 3 && nh < 2; ++i) {
        const int j = (i + 1) % 3;
        if ((d[i] > kEps && d[j] < -kEps) || (d[i] < -kEps && d[j] > kEps)) {
            const double t = d[i] / (d[i] - d[j]);
            hit[nh++] = v[i] + (v[j] - v[i]) * t;
        } else if (std::fabs(d[i]) <= kEps) {
            // vertex exactly on the plane: take it once
            bool dup = false;
            for (int k = 0; k < nh; ++k) if ((hit[k] - v[i]).length() < 1e-7) dup = true;
            if (!dup) hit[nh++] = v[i];
        }
    }
    if (nh == 2 && (hit[0] - hit[1]).length() > 1e-10) {
        segments.push_back({hit[0], hit[1]});
        points.push_back(hit[0]);
        points.push_back(hit[1]);
    }
}

} // namespace

Section cutSection(const std::vector<D3>& pos, const std::vector<PolyFace>& faces,
                   const D3& normal, const D3& point) {
    Section s;
    const D3 n = normal.normalized();
    std::vector<std::pair<D3, D3>> segments;
    std::vector<D3> pts;
    for (const PolyFace& f : faces) {
        if (f.v.size() < 3) continue;
        // fan triangulation (quads are near-convex; the cut perimeter is the
        // same for either diagonal because the plane cut is linear)
        for (size_t k = 1; k + 1 < f.v.size(); ++k) {
            cutTriangle(pos[static_cast<size_t>(f.v[0])], pos[static_cast<size_t>(f.v[k])],
                        pos[static_cast<size_t>(f.v[k + 1])], n, point, segments, pts);
        }
    }
    for (const auto& seg : segments) s.perimeter += (seg.second - seg.first).length();
    s.points = pts.size();

    // In-plane frame for per-loop breadth/depth/area.
    D3 u, v;
    if (std::fabs(n.y) > 0.99) {
        u = {1, 0, 0};
        v = {0, 0, 1};
    } else {
        u = D3{0, 1, 0}.cross(n).normalized();
        v = n.cross(u).normalized();
    }

    // Chain segments into loops; per-loop perimeter / area / extents.
    auto key = [](const D3& p) {
        return std::make_tuple(std::llround(p.x * 1e7), std::llround(p.y * 1e7), std::llround(p.z * 1e7));
    };
    std::map<std::tuple<long, long, long>, std::vector<size_t>> ends;
    for (size_t i = 0; i < segments.size(); ++i) {
        ends[key(segments[i].first)].push_back(i);
        ends[key(segments[i].second)].push_back(i);
    }
    std::vector<bool> used(segments.size(), false);
    for (size_t start = 0; start < segments.size(); ++start) {
        if (used[start]) continue;
        used[start] = true;
        std::vector<D3> loopPts;
        loopPts.push_back(segments[start].first);
        loopPts.push_back(segments[start].second);
        double loopPerimeter = (segments[start].second - segments[start].first).length();
        auto cur = key(segments[start].second);
        const auto origin = key(segments[start].first);
        bool closed = false;
        for (int guard = 0; guard < 1000000; ++guard) {
            if (cur == origin) { closed = true; break; }
            auto it = ends.find(cur);
            if (it == ends.end()) break;
            size_t next = SIZE_MAX;
            for (size_t idx : it->second) if (!used[idx]) { next = idx; break; }
            if (next == SIZE_MAX) break;
            used[next] = true;
            const auto a = key(segments[next].first), b = key(segments[next].second);
            const bool forward = (a == cur);
            const D3& from = forward ? segments[next].first : segments[next].second;
            const D3& to = forward ? segments[next].second : segments[next].first;
            loopPerimeter += (to - from).length();
            loopPts.push_back(to);
            cur = forward ? b : a;
        }
        if (!closed) { ++s.openChains; continue; }
        SectionLoop lp;
        lp.perimeter = loopPerimeter;
        lp.points = loopPts.size();
        double uMin = 1e30, uMax = -1e30, vMin = 1e30, vMax = -1e30;
        D3 cent;
        double area2 = 0.0;
        for (size_t i = 0; i < loopPts.size(); ++i) {
            const D3& p = loopPts[i];
            cent += p;
            const double uu = u.dot(p), vv = v.dot(p);
            uMin = std::min(uMin, uu); uMax = std::max(uMax, uu);
            vMin = std::min(vMin, vv); vMax = std::max(vMax, vv);
            const D3& q = loopPts[(i + 1) % loopPts.size()];
            area2 += uu * v.dot(q) - u.dot(q) * vv;
        }
        lp.area = std::fabs(area2) * 0.5;
        lp.breadth = uMax - uMin;
        lp.depth = vMax - vMin;
        lp.centroid = cent * (1.0 / static_cast<double>(loopPts.size()));
        s.loops.push_back(lp);
    }
    return s;
}

const SectionLoop* Section::largestArea() const {
    const SectionLoop* best = nullptr;
    for (const SectionLoop& l : loops)
        if (!best || l.area > best->area) best = &l;
    return best;
}

const SectionLoop* Section::nearestTo(const D3& p) const {
    const SectionLoop* best = nullptr;
    double bestD = 1e30;
    for (const SectionLoop& l : loops) {
        const double d = (l.centroid - p).length();
        if (d < bestD) { bestD = d; best = &l; }
    }
    return best;
}

Wp3Measures measureAll(const std::vector<D3>& bodyPos, const std::vector<Region>& regions,
                       const Rig& rig, const std::vector<PolyFace>& bodyFaces) {
    Wp3Measures m;
    double yMin = 1e30, yMax = -1e30;
    for (const D3& p : bodyPos) { yMin = std::min(yMin, p.y); yMax = std::max(yMax, p.y); }
    m.soleY = yMin;
    m.stature = yMax - yMin;

    // Joint selection mirrors ht_measure.cpp (01 joint bone / 02 shaft bone).
    std::vector<D3> shoulder, elbow, wrist, hipJoint, knee, ankle;
    const RigBone* upperArmL = nullptr;
    for (const RigBone& b : rig.bones) {
        const bool second = b.name.find("02") != std::string::npos;
        if (b.role == Role::UpperArm && !second) shoulder.push_back(b.head);
        if (b.role == Role::UpperArm && second) elbow.push_back(b.tail);
        if (b.role == Role::Hand && b.name.compare(0, 5, "wrist") == 0) wrist.push_back(b.head);
        if (b.role == Role::Thigh && !second) hipJoint.push_back(b.head);
        if (b.role == Role::Thigh && second) knee.push_back(b.tail);
        if (b.role == Role::Foot && b.name.compare(0, 4, "foot") == 0) ankle.push_back(b.head);
        if (b.name == "upperarm01.L") upperArmL = &b;
    }
    auto meanDistance = [](const std::vector<D3>& a, const std::vector<D3>& b) {
        if (a.empty() || b.empty() || a.size() != b.size()) return 0.0;
        double sum = 0.0;
        for (size_t i = 0; i < a.size(); ++i) sum += (a[i] - b[i]).length();
        return sum / static_cast<double>(a.size());
    };
    auto meanHeight = [&](const std::vector<D3>& pts) {
        if (pts.empty()) return 0.0;
        double sum = 0.0;
        for (const D3& p : pts) sum += p.y;
        return sum / static_cast<double>(pts.size()) - yMin;
    };
    m.upperArmLength = meanDistance(shoulder, elbow);
    m.forearmLength = meanDistance(elbow, wrist);
    m.thighLength = meanDistance(hipJoint, knee);
    m.shankLength = meanDistance(knee, ankle);
    m.shoulderJointHeight = meanHeight(shoulder);
    m.hipJointHeight = meanHeight(hipJoint);

    // Biacromial proxy: distance between the L/R shoulder joint centres.
    // NOT the anatomical acromion distance (no acromion landmark on this mesh).
    if (shoulder.size() == 2) m.biacromialProxy = (shoulder[0] - shoulder[1]).length();

    // Shoulder band extents, region-restricted (vertex-band caliber, 1.5% half-width).
    const double bandY = yMin + m.shoulderJointHeight - 0.025 * m.stature;
    const double half = 0.015 * m.stature;
    auto bandExtent = [&](const std::vector<Region>& allowed) {
        double xMin = 1e30, xMax = -1e30;
        for (size_t v = 0; v < bodyPos.size() && v < regions.size(); ++v) {
            if (std::find(allowed.begin(), allowed.end(), regions[v]) == allowed.end()) continue;
            if (std::fabs(bodyPos[v].y - bandY) > half) continue;
            xMin = std::min(xMin, bodyPos[v].x);
            xMax = std::max(xMax, bodyPos[v].x);
        }
        return xMax >= xMin ? xMax - xMin : 0.0;
    };
    m.shoulderYoke = bandExtent({Region::Neck, Region::Thorax, Region::ShoulderGirdle});
    m.outerShoulder = bandExtent({Region::Neck, Region::Thorax, Region::ShoulderGirdle, Region::UpperArm});

    // True horizontal cross-sections. The torso loop is the largest-area one;
    // arm loops (A-pose hands hang at waist/hip height) are excluded.
    const Section chest = cutSection(bodyPos, bodyFaces, {0, 1, 0}, {0, yMin + 0.72 * m.stature, 0});
    if (const SectionLoop* lp = chest.largestArea()) {
        m.chestBreadth = lp->breadth; m.chestDepth = lp->depth; m.chestCircumference = lp->perimeter;
    }
    const Section waist = cutSection(bodyPos, bodyFaces, {0, 1, 0}, {0, yMin + 0.60 * m.stature, 0});
    if (const SectionLoop* lp = waist.largestArea()) {
        m.waistBreadth = lp->breadth; m.waistDepth = lp->depth; m.waistCircumference = lp->perimeter;
    }
    const Section hip = cutSection(bodyPos, bodyFaces, {0, 1, 0}, {0, yMin + m.hipJointHeight, 0});
    if (const SectionLoop* lp = hip.largestArea()) {
        m.hipBreadth = lp->breadth; m.hipDepth = lp->depth; m.hipCircumference = lp->perimeter;
    }

    // Upper-arm circumference: oblique section perpendicular to the bone axis
    // at the shaft midpoint; take the loop nearest that point (the plane may
    // also graze the torso).
    const RigBone* shaft = nullptr;
    for (const RigBone& b : rig.bones)
        if (b.name == "upperarm02.L") shaft = &b;
    if (!shaft) shaft = upperArmL;
    if (shaft) {
        const D3 axis = (shaft->tail - shaft->head).normalized();
        const D3 mid = (shaft->head + shaft->tail) * 0.5;
        const Section arm = cutSection(bodyPos, bodyFaces, axis, mid);
        if (const SectionLoop* lp = arm.nearestTo(mid)) {
            m.upperArmCircumferenceL = lp->perimeter;
            m.upperArmBreadthL = lp->breadth;
        }
    }
    return m;
}

RegionLinkage regionLinkage(const std::vector<Region>& regions, const std::vector<int>& vertex,
                            const std::vector<D3>& delta, size_t bodyCount) {
    RegionLinkage l;
    double sum[14] = {0}, mx[14] = {0};
    long cnt[14] = {0};
    double helperSum = 0.0;
    long helperCnt = 0;
    for (size_t i = 0; i < vertex.size() && i < delta.size(); ++i) {
        const double len = delta[i].length();
        if (len <= 1e-9) continue;
        const int v = vertex[i];
        if (v < 0 || static_cast<size_t>(v) >= bodyCount) { helperSum += len; ++helperCnt; continue; }
        const int r = static_cast<int>(regions[static_cast<size_t>(v)]);
        if (r < 0 || r >= 14) continue;
        sum[r] += len; mx[r] = std::max(mx[r], len); ++cnt[r];
    }
    for (int r = 0; r < 14; ++r) {
        l.mean[r] = cnt[r] ? sum[r] / static_cast<double>(cnt[r]) : 0.0;
        l.max[r] = mx[r];
    }
    l.helperMean = helperCnt ? helperSum / static_cast<double>(helperCnt) : 0.0;
    return l;
}

} // namespace pae::wp3