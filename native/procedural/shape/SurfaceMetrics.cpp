#include "procedural/shape/SurfaceMetrics.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace pae::shape {
namespace {

constexpr float kPi = 3.14159265358979323846f;

// Squared distance from a point to a triangle (Ericson, Real-Time Collision
// Detection). Returns the squared distance and the closest point.
double closestPointOnTriangle(const Vec3& p, const Vec3& a, const Vec3& b, const Vec3& c,
                              Vec3& closest) {
    const Vec3 ab = b - a;
    const Vec3 ac = c - a;
    const Vec3 ap = p - a;
    const double d1 = ab.dot(ap);
    const double d2 = ac.dot(ap);
    if (d1 <= 0.0 && d2 <= 0.0) {
        closest = a;
        return (p - a).lengthSq();
    }

    const Vec3 bp = p - b;
    const double d3 = ab.dot(bp);
    const double d4 = ac.dot(bp);
    if (d3 >= 0.0 && d4 <= d3) {
        closest = b;
        return (p - b).lengthSq();
    }

    const double vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0) {
        const double v = d1 / (d1 - d3);
        closest = a + ab * static_cast<float>(v);
        return (p - closest).lengthSq();
    }

    const Vec3 cp = p - c;
    const double d5 = ab.dot(cp);
    const double d6 = ac.dot(cp);
    if (d6 >= 0.0 && d5 <= d6) {
        closest = c;
        return (p - c).lengthSq();
    }

    const double vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0) {
        const double w = d2 / (d2 - d6);
        closest = a + ac * static_cast<float>(w);
        return (p - closest).lengthSq();
    }

    const double va = d3 * d6 - d5 * d4;
    if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0) {
        const double w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        closest = b + (c - b) * static_cast<float>(w);
        return (p - closest).lengthSq();
    }

    const double denom = 1.0 / (va + vb + vc);
    const double v = vb * denom;
    const double w = vc * denom;
    closest = a + ab * static_cast<float>(v) + ac * static_cast<float>(w);
    return (p - closest).lengthSq();
}

double percentile(std::vector<double> values, double p) {
    if (values.empty()) return 0.0;
    std::sort(values.begin(), values.end());
    const double idx = p * static_cast<double>(values.size() - 1);
    const size_t lo = static_cast<size_t>(std::floor(idx));
    const size_t hi = static_cast<size_t>(std::ceil(idx));
    const double frac = idx - static_cast<double>(lo);
    return values[lo] * (1.0 - frac) + values[hi] * frac;
}

} // namespace

double distanceToSurface(const MeshData& mesh, const Vec3& p) {
    double best = std::numeric_limits<double>::max();
    Vec3 closest;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const Vec3& a = mesh.vertices[mesh.indices[i]].position;
        const Vec3& b = mesh.vertices[mesh.indices[i + 1]].position;
        const Vec3& c = mesh.vertices[mesh.indices[i + 2]].position;
        const double d2 = closestPointOnTriangle(p, a, b, c, closest);
        if (d2 < best) best = d2;
    }
    if (best == std::numeric_limits<double>::max()) return 0.0;
    return std::sqrt(best);
}

SurfaceDistance measureSurfaceDistance(const std::vector<Vec3>& a, const std::vector<Vec3>& b) {
    SurfaceDistance result;
    result.sampleCountA = static_cast<int>(a.size());
    result.sampleCountB = static_cast<int>(b.size());
    if (a.empty() || b.empty()) return result;

    // A -> B
    double forwardMax = 0.0;
    for (const Vec3& p : a) {
        double best = std::numeric_limits<double>::max();
        for (const Vec3& q : b) {
            const double d = (p - q).lengthSq();
            if (d < best) best = d;
        }
        forwardMax = std::max(forwardMax, std::sqrt(best));
    }

    // B -> A, with the distribution of the per-sample distances.
    std::vector<double> distances;
    distances.reserve(b.size());
    double backwardMax = 0.0;
    double sum = 0.0;
    double sumSq = 0.0;
    for (const Vec3& q : b) {
        double best = std::numeric_limits<double>::max();
        for (const Vec3& p : a) {
            const double d = (q - p).lengthSq();
            if (d < best) best = d;
        }
        const double dist = std::sqrt(best);
        distances.push_back(dist);
        backwardMax = std::max(backwardMax, dist);
        sum += dist;
        sumSq += dist * dist;
    }

    result.forwardHausdorff = forwardMax;
    result.backwardHausdorff = backwardMax;
    result.hausdorff = std::max(forwardMax, backwardMax);
    result.mean = sum / static_cast<double>(b.size());
    result.rms = std::sqrt(sumSq / static_cast<double>(b.size()));
    result.p95 = percentile(distances, 0.95);
    result.valid = true;
    return result;
}

MeshDistance measureMeshDistance(const MeshData& mesh, const std::vector<Vec3>& samples) {
    MeshDistance result;
    result.sampleCount = static_cast<int>(samples.size());
    if (samples.empty() || mesh.indices.size() < 3) return result;

    std::vector<double> distances;
    distances.reserve(samples.size());
    double sum = 0.0;
    double sumSq = 0.0;
    double worst = 0.0;
    for (const Vec3& p : samples) {
        const double d = distanceToSurface(mesh, p);
        distances.push_back(d);
        sum += d;
        sumSq += d * d;
        worst = std::max(worst, d);
    }
    result.hausdorff = worst;
    result.mean = sum / static_cast<double>(samples.size());
    result.rms = std::sqrt(sumSq / static_cast<double>(samples.size()));
    result.p95 = percentile(distances, 0.95);
    result.valid = true;
    return result;
}

std::vector<Vec3> sampleImplicitSurface(const std::function<float(Vec3)>& field,
                                        const BodyJoints& joints,
                                        const SurfaceSamplingSpec& spec) {
    std::vector<Vec3> samples;
    const float yBottom = joints.hips.y + spec.yBottomOffset;
    const float yTop = joints.chest.y + spec.yTopOffset;
    constexpr float kStep = 0.004f;

    for (int i = 0; i < spec.rings; ++i) {
        const float v = spec.rings > 1 ? static_cast<float>(i) / static_cast<float>(spec.rings - 1)
                                       : 0.0f;
        const float y = yBottom + (yTop - yBottom) * v;
        for (int j = 0; j < spec.segments; ++j) {
            const float u = static_cast<float>(j) / static_cast<float>(spec.segments);
            const float angle = 2.0f * kPi * u;
            const Vec3 dir{std::cos(angle), 0.0f, std::sin(angle)};

            // Find the first crossing, walking outward from the axis.
            float previous = field(Vec3{0.0f, y, 0.0f});
            bool inside = previous < 0.0f;
            float t = 0.0f;
            bool crossed = false;
            for (t = kStep; t <= 0.55f; t += kStep) {
                const float current = field(Vec3{dir.x * t, y, dir.z * t});
                if (inside && current >= 0.0f) {
                    crossed = true;
                    break;
                }
                previous = current;
                inside = current < 0.0f;
            }
            if (!crossed) continue;

            // Refine by bisection.
            float lo = t - kStep;
            float hi = t;
            for (int it = 0; it < 20; ++it) {
                const float mid = (lo + hi) * 0.5f;
                if (field(Vec3{dir.x * mid, y, dir.z * mid}) < 0.0f) lo = mid;
                else hi = mid;
            }
            const float r = (lo + hi) * 0.5f;
            samples.push_back(Vec3{dir.x * r, y, dir.z * r});
        }
    }
    return samples;
}

std::vector<Vec3> sampleParametricSurface(const std::function<Vec3(float, float)>& surface,
                                           const BodyJoints& joints,
                                           const SurfaceSamplingSpec& spec) {
    std::vector<Vec3> samples;
    for (int i = 0; i < spec.rings; ++i) {
        const float v = spec.rings > 1 ? static_cast<float>(i) / static_cast<float>(spec.rings - 1)
                                       : 0.0f;
        for (int j = 0; j < spec.segments; ++j) {
            const float u = static_cast<float>(j) / static_cast<float>(spec.segments);
            samples.push_back(surface(v, u));
        }
    }
    return samples;
}

float SurfaceGrid::ringY(int ring) const {
    if (rings <= 1) return yBottom;
    const float t = static_cast<float>(ring) / static_cast<float>(rings - 1);
    return yBottom + (yTop - yBottom) * t;
}

int SurfaceGrid::ringForHeight(float y) const {
    if (rings <= 1) return 0;
    const float span = yTop - yBottom;
    if (std::fabs(span) < 1e-9f) return 0;
    float t = (y - yBottom) / span;
    t = std::clamp(t, 0.0f, 1.0f);
    int ring = static_cast<int>(std::lround(t * static_cast<float>(rings - 1)));
    return std::clamp(ring, 0, rings - 1);
}

int SurfaceGrid::segmentForAzimuth(float u) const {
    if (segments <= 0) return 0;
    float wrapped = u - std::floor(u);
    int segment = static_cast<int>(std::lround(wrapped * static_cast<float>(segments)));
    segment %= segments;
    if (segment < 0) segment += segments;
    return segment;
}

namespace {

// Unit normal of an implicit field from a central-difference gradient.
// The field is negative inside, so the gradient points OUTWARD.
Vec3 fieldNormal(const std::function<float(Vec3)>& field, const Vec3& p, float h) {
    const float dx = field(Vec3{p.x + h, p.y, p.z}) - field(Vec3{p.x - h, p.y, p.z});
    const float dy = field(Vec3{p.x, p.y + h, p.z}) - field(Vec3{p.x, p.y - h, p.z});
    const float dz = field(Vec3{p.x, p.y, p.z + h}) - field(Vec3{p.x, p.y, p.z - h});
    Vec3 n{dx, dy, dz};
    const float len = std::sqrt(n.lengthSq());
    if (len < 1e-12f) return Vec3{0.0f, 1.0f, 0.0f};
    return n * (1.0f / len);
}

// Points a normal away from the body axis (the Y axis through the origin).
Vec3 orientOutward(Vec3 n, const Vec3& p) {
    const Vec3 radial{p.x, 0.0f, p.z};
    if (radial.lengthSq() > 1e-10f && n.dot(radial) < 0.0f) return n * -1.0f;
    return n;
}

} // namespace

SurfaceGrid sampleImplicitSurfaceGrid(const std::function<float(Vec3)>& field,
                                      const BodyJoints& joints,
                                      const SurfaceSamplingSpec& spec) {
    SurfaceGrid grid;
    grid.rings = spec.rings;
    grid.segments = spec.segments;
    grid.yBottom = joints.hips.y + spec.yBottomOffset;
    grid.yTop = joints.chest.y + spec.yTopOffset;
    const size_t cells = static_cast<size_t>(std::max(0, spec.rings)) *
                         static_cast<size_t>(std::max(0, spec.segments));
    grid.positions.assign(cells, Vec3{});
    grid.normals.assign(cells, Vec3{0.0f, 1.0f, 0.0f});
    grid.valid.assign(cells, 0);

    constexpr float kStep = 0.004f;
    constexpr float kNormalStep = 0.0015f;

    for (int i = 0; i < spec.rings; ++i) {
        const float v = spec.rings > 1 ? static_cast<float>(i) / static_cast<float>(spec.rings - 1)
                                       : 0.0f;
        const float y = grid.yBottom + (grid.yTop - grid.yBottom) * v;
        for (int j = 0; j < spec.segments; ++j) {
            const float u = static_cast<float>(j) / static_cast<float>(spec.segments);
            const float angle = 2.0f * kPi * u;
            const Vec3 dir{std::cos(angle), 0.0f, std::sin(angle)};

            // Find the first crossing, walking outward from the axis.
            float previous = field(Vec3{0.0f, y, 0.0f});
            bool inside = previous < 0.0f;
            float t = 0.0f;
            bool crossed = false;
            for (t = kStep; t <= 0.55f; t += kStep) {
                const float current = field(Vec3{dir.x * t, y, dir.z * t});
                if (inside && current >= 0.0f) {
                    crossed = true;
                    break;
                }
                previous = current;
                inside = current < 0.0f;
            }
            if (!crossed) continue;

            // Refine by bisection.
            float lo = t - kStep;
            float hi = t;
            for (int it = 0; it < 20; ++it) {
                const float mid = (lo + hi) * 0.5f;
                if (field(Vec3{dir.x * mid, y, dir.z * mid}) < 0.0f) lo = mid;
                else hi = mid;
            }
            const float r = (lo + hi) * 0.5f;
            const Vec3 p{dir.x * r, y, dir.z * r};

            const size_t idx = grid.index(i, j);
            grid.positions[idx] = p;
            grid.normals[idx] = fieldNormal(field, p, kNormalStep);
            grid.valid[idx] = 1;
        }
    }
    return grid;
}

SurfaceGrid sampleParametricSurfaceGrid(const std::function<Vec3(float, float)>& surface,
                                         const std::function<Vec3(float, float)>& normal,
                                         const BodyJoints& joints,
                                         const SurfaceSamplingSpec& spec) {
    SurfaceGrid grid;
    grid.rings = spec.rings;
    grid.segments = spec.segments;
    grid.yBottom = joints.hips.y + spec.yBottomOffset;
    grid.yTop = joints.chest.y + spec.yTopOffset;
    const size_t cells = static_cast<size_t>(std::max(0, spec.rings)) *
                         static_cast<size_t>(std::max(0, spec.segments));
    grid.positions.assign(cells, Vec3{});
    grid.normals.assign(cells, Vec3{0.0f, 1.0f, 0.0f});
    grid.valid.assign(cells, 0);

    constexpr float kNormalStep = 1e-3f;

    for (int i = 0; i < spec.rings; ++i) {
        const float v = spec.rings > 1 ? static_cast<float>(i) / static_cast<float>(spec.rings - 1)
                                       : 0.0f;
        for (int j = 0; j < spec.segments; ++j) {
            const float u = static_cast<float>(j) / static_cast<float>(spec.segments);
            const Vec3 p = surface(v, u);

            Vec3 n{};
            if (normal) {
                n = normal(v, u);
            } else {
                const Vec3 pu = surface(v, u + kNormalStep) - surface(v, u - kNormalStep);
                const Vec3 pv = surface(v + kNormalStep, u) - surface(v - kNormalStep, u);
                n = pu.cross(pv);
            }
            const float len = std::sqrt(n.lengthSq());
            if (len < 1e-12f) {
                n = Vec3{p.x, 0.0f, p.z};
                const float rl = std::sqrt(n.lengthSq());
                if (rl < 1e-12f) n = Vec3{0.0f, 1.0f, 0.0f};
                else n = n * (1.0f / rl);
            } else {
                n = n * (1.0f / len);
            }
            n = orientOutward(n, p);

            const size_t idx = grid.index(i, j);
            grid.positions[idx] = p;
            grid.normals[idx] = n;
            grid.valid[idx] = 1;
        }
    }
    return grid;
}

Distribution summarize(std::vector<double> values) {
    Distribution d;
    d.count = static_cast<int>(values.size());
    if (values.empty()) return d;
    double sum = 0.0;
    for (double v : values) sum += v;
    d.mean = sum / static_cast<double>(values.size());
    d.p50 = percentile(values, 0.50);
    d.p95 = percentile(values, 0.95);
    d.max = *std::max_element(values.begin(), values.end());
    return d;
}

} // namespace pae::shape
