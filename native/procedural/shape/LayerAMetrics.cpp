#include "procedural/shape/LayerAMetrics.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <unordered_map>

namespace pae::shape {
namespace {

constexpr double kPi = 3.14159265358979323846;

// Normal of a quad, from the cross product of its diagonals.
Vec3 quadNormal(const MeshData& mesh, const std::array<uint32_t, 4>& quad) {
    const Vec3& a = mesh.vertices[quad[0]].position;
    const Vec3& b = mesh.vertices[quad[1]].position;
    const Vec3& c = mesh.vertices[quad[2]].position;
    const Vec3& d = mesh.vertices[quad[3]].position;
    const Vec3 diag1 = c - a;
    const Vec3 diag2 = d - b;
    Vec3 n = diag1.cross(diag2);
    const float len = std::sqrt(n.lengthSq());
    if (len < 1e-12f) return Vec3{0.0f, 1.0f, 0.0f};
    return n * (1.0f / len);
}

// Dihedral deflection between two faces, in degrees.
// 0 degrees means the two faces are coplanar; 180 means they fold back on
// each other. This is the deflection of the SURFACE, which is what "smooth
// seam" means for a body mesh.
double dihedralDeg(const Vec3& n0, const Vec3& n1) {
    float d = n0.dot(n1);
    d = std::clamp(d, -1.0f, 1.0f);
    return std::acos(static_cast<double>(d)) * 180.0 / kPi;
}

} // namespace

std::vector<AnatomicalLandmark> anatomicalLandmarkSet(const BodyJoints& joints,
                                                      const QuadTemplateSpec& spec) {
    // The levels are anchored to the shared landmark skeleton so that every
    // route - and the target reference - resolves the SAME anatomical heights.
    const float iliac = joints.hips.y + 0.04f;
    const float waist = joints.hips.y + 0.12f;
    const float lowerRib = joints.spine.y;
    const float sternum = joints.chest.y - 0.08f;
    const float shoulderRoot = joints.chest.y + spec.yTopOffset;

    return {
        {"iliac-crest-left", iliac, 0.000f},
        {"iliac-crest-front", iliac, 0.250f},
        {"waist-left", waist, 0.000f},
        {"waist-back", waist, 0.750f},
        {"lower-rib-left", lowerRib, 0.000f},
        {"lower-rib-front", lowerRib, 0.250f},
        {"sternum", sternum, 0.250f},
        {"upper-chest-left", sternum, 0.000f},
        {"scapula-back", sternum, 0.750f},
        {"shoulder-root-left", shoulderRoot, 0.000f},
        {"shoulder-root-right", shoulderRoot, 0.500f},
        {"shoulder-root-left-front", shoulderRoot, 0.125f},
        {"shoulder-root-left-back", shoulderRoot, 0.875f},
        {"shoulder-root-right-front", shoulderRoot, 0.375f},
        {"shoulder-root-right-back", shoulderRoot, 0.625f},
    };
}

LandmarkError measureLandmarkError(const SurfaceGrid& route,
                                   const SurfaceGrid& target,
                                   const std::vector<AnatomicalLandmark>& landmarks) {
    LandmarkError result;
    if (route.rings == 0 || target.rings == 0) return result;
    if (route.segments != target.segments) return result;

    std::vector<double> errors;
    errors.reserve(landmarks.size());
    for (const AnatomicalLandmark& landmark : landmarks) {
        LandmarkErrorEntry entry;
        entry.name = landmark.name;
        // Both grids are sampled with the same rule, so the ring and segment
        // resolution is identical: the landmark lands on the SAME cell for
        // both surfaces.
        const int ring = target.ringForHeight(landmark.y);
        const int segment = target.segmentForAzimuth(landmark.u);
        if (!route.isValid(ring, segment) || !target.isValid(ring, segment)) {
            result.entries.push_back(entry);
            continue;
        }
        const Vec3& a = route.positions[static_cast<size_t>(route.index(ring, segment))];
        const Vec3& b = target.positions[static_cast<size_t>(target.index(ring, segment))];
        entry.error = static_cast<double>((a - b).length());
        entry.valid = true;
        errors.push_back(entry.error);
        result.entries.push_back(entry);
    }
    if (errors.empty()) return result;
    result.distribution = summarize(std::move(errors));
    result.valid = true;
    return result;
}

NormalDeviation measureNormalDeviation(const SurfaceGrid& route, const SurfaceGrid& target) {
    NormalDeviation result;
    if (route.rings == 0 || target.rings == 0) return result;
    if (route.segments != target.segments) return result;

    std::vector<double> angles;
    angles.reserve(static_cast<size_t>(route.rings * route.segments));
    for (int r = 0; r < route.rings; ++r) {
        for (int s = 0; s < route.segments; ++s) {
            if (!route.isValid(r, s) || !target.isValid(r, s)) continue;
            const Vec3& na = route.normals[static_cast<size_t>(route.index(r, s))];
            const Vec3& nb = target.normals[static_cast<size_t>(target.index(r, s))];
            float d = na.dot(nb);
            // A flipped normal is an error, NOT a 5-degree discrepancy. Keep
            // the raw angle so the statistic cannot hide it, and report the
            // flip count separately.
            if (d < 0.0f) ++result.flipped;
            d = std::clamp(d, -1.0f, 1.0f);
            angles.push_back(std::acos(static_cast<double>(d)) * 180.0 / kPi);
        }
    }
    if (angles.empty()) return result;
    result.angleDeg = summarize(std::move(angles));
    result.compared = static_cast<int>(result.angleDeg.count);
    result.valid = true;
    return result;
}

SectionDeviation measureSectionDeviation(const SurfaceGrid& route, const SurfaceGrid& target,
                                         int bandCount) {
    SectionDeviation result;
    if (route.rings == 0 || target.rings == 0) return result;
    if (route.segments != target.segments) return result;
    if (bandCount < 1) bandCount = 1;

    std::vector<double> absolute;
    for (int band = 0; band < bandCount; ++band) {
        // Bands tile the grid from the bottom to the top without overlap.
        const int lo = static_cast<int>(static_cast<long long>(band) * route.rings / bandCount);
        const int hi = static_cast<int>(static_cast<long long>(band + 1) * route.rings / bandCount);
        SectionBand section;
        section.y = route.ringY(lo);

        std::vector<double> signedDeviation;
        for (int r = lo; r < hi; ++r) {
            for (int s = 0; s < route.segments; ++s) {
                if (!route.isValid(r, s) || !target.isValid(r, s)) continue;
                const Vec3& a = route.positions[static_cast<size_t>(route.index(r, s))];
                const Vec3& b = target.positions[static_cast<size_t>(target.index(r, s))];
                const double radiusA = std::sqrt(static_cast<double>(a.x) * a.x +
                                                 static_cast<double>(a.z) * a.z);
                const double radiusB = std::sqrt(static_cast<double>(b.x) * b.x +
                                                 static_cast<double>(b.z) * b.z);
                const double deviation = radiusA - radiusB;
                signedDeviation.push_back(deviation);
                absolute.push_back(std::fabs(deviation));
            }
        }
        if (signedDeviation.empty()) continue;
        section.radialDeviation = summarize(std::move(signedDeviation));
        section.samples = static_cast<int>(section.radialDeviation.count);
        result.bands.push_back(section);
    }
    if (absolute.empty()) return result;
    result.absoluteDeviation = summarize(std::move(absolute));
    result.valid = true;
    return result;
}

SeamAudit auditSeams(const MeshData& mesh,
                     const std::vector<std::array<uint32_t, 4>>& quads,
                     const std::vector<CropBoundary>& crops,
                     double smoothThresholdDeg) {
    SeamAudit result;
    if (quads.empty()) return result;

    // ---- Interior seams: quad edges shared by exactly two quads ----
    // Edge key: the two endpoint indices, sorted.
    std::map<std::pair<uint32_t, uint32_t>, std::vector<size_t>> edgeQuads;
    for (size_t q = 0; q < quads.size(); ++q) {
        const std::array<uint32_t, 4>& quad = quads[q];
        for (int e = 0; e < 4; ++e) {
            const uint32_t a = quad[e];
            const uint32_t b = quad[(e + 1) % 4];
            edgeQuads[{std::min(a, b), std::max(a, b)}].push_back(q);
        }
    }

    std::vector<double> dihedrals;
    for (const auto& entry : edgeQuads) {
        if (entry.second.size() != 2) continue;
        const Vec3& n0 = quadNormal(mesh, quads[entry.second[0]]);
        const Vec3& n1 = quadNormal(mesh, quads[entry.second[1]]);
        dihedrals.push_back(dihedralDeg(n0, n1));
    }
    if (!dihedrals.empty()) {
        result.interiorSeamEdges = static_cast<int>(dihedrals.size());
        const Distribution stats = summarize(dihedrals);
        result.meanDihedralDeg = stats.mean;
        result.p95DihedralDeg = stats.p95;
        result.maxDihedralDeg = stats.max;
        for (double d : dihedrals) {
            if (d <= smoothThresholdDeg) ++result.smoothSeamEdges;
            else ++result.hardSeamEdges;
        }
    }

    // ---- The T2 shoulder-root crop ring ----
    // The crop sits on template ring R. The quads that touch it are the ones
    // of band R-1, and the ring's own smoothness is the deflection between
    // two consecutive quads around that band.
    //
    // Template layout: quad(i, j) = { i*S + j, i*S + jn, (i+1)*S + jn,
    // (i+1)*S + j }, so the band index of a quad is quad[0] / S.
    for (const CropBoundary& crop : crops) {
        if (crop.ringIndex < 1 || crop.vertexCount < 3) continue;
        const int segments = crop.vertexCount;
        const int band = crop.ringIndex - 1;

        std::vector<size_t> bandQuads;
        for (size_t q = 0; q < quads.size(); ++q) {
            const uint32_t lower = quads[q][0];
            if (lower >= mesh.vertices.size()) continue;
            if (static_cast<int>(lower / static_cast<uint32_t>(segments)) != band) continue;
            bandQuads.push_back(q);
        }
        if (bandQuads.size() < 3) continue;

        // Consecutive quads around the band share an edge; the deflection
        // between their normals is the seam's smoothness.
        std::vector<double> bandDihedrals;
        bandDihedrals.reserve(bandQuads.size());
        for (size_t i = 0; i < bandQuads.size(); ++i) {
            const size_t next = (i + 1) % bandQuads.size();
            const Vec3& n0 = quadNormal(mesh, quads[bandQuads[i]]);
            const Vec3& n1 = quadNormal(mesh, quads[bandQuads[next]]);
            bandDihedrals.push_back(dihedralDeg(n0, n1));
        }

        // The upper row of the band quads IS the crop ring.
        std::vector<double> radii;
        std::vector<float> heights;
        for (size_t q : bandQuads) {
            const std::array<uint32_t, 4>& quad = quads[q];
            for (uint32_t index : {quad[2], quad[3]}) {
                if (index >= mesh.vertices.size()) continue;
                const Vec3& p = mesh.vertices[index].position;
                heights.push_back(p.y);
                radii.push_back(std::sqrt(static_cast<double>(p.x) * p.x +
                                          static_cast<double>(p.z) * p.z));
            }
        }

        const Distribution stats = summarize(bandDihedrals);
        result.cropRingEdges = static_cast<int>(bandQuads.size());
        result.cropRingMeanDihedralDeg = stats.mean;
        result.cropRingMaxDihedralDeg = stats.max;
        result.cropRingSmooth = stats.max <= smoothThresholdDeg;

        if (!radii.empty()) {
            const double mean = std::accumulate(radii.begin(), radii.end(), 0.0) /
                                static_cast<double>(radii.size());
            double variance = 0.0;
            for (double r : radii) variance += (r - mean) * (r - mean);
            variance /= static_cast<double>(radii.size());
            result.cropRingMeanRadius = mean;
            result.cropRingRadiusSpread = std::sqrt(variance);
        }
        if (!heights.empty()) {
            const float mean = std::accumulate(heights.begin(), heights.end(), 0.0f) /
                               static_cast<float>(heights.size());
            float worst = 0.0f;
            for (float h : heights) worst = std::max(worst, std::fabs(h - mean));
            result.cropRingPlanarityMm = static_cast<double>(worst) * 1000.0;
        }
        break;  // one crop ring is enough for the PoC-A scope
    }

    result.valid = true;
    return result;
}

} // namespace pae::shape
