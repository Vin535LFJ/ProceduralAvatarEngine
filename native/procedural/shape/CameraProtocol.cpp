#include "procedural/shape/CameraProtocol.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace pae::shape {
namespace {

constexpr double kPi = 3.14159265358979323846;

Vec3 normalize(const Vec3& v) {
    const float len = std::sqrt(v.lengthSq());
    if (len < 1e-9f) return Vec3{0.0f, 1.0f, 0.0f};
    return v * (1.0f / len);
}

} // namespace

ViewFrame viewFrame(const View& view) {
    const double az = view.azimuthDeg * kPi / 180.0;
    const double el = view.elevationDeg * kPi / 180.0;
    // Camera position on a sphere around the body; it looks back at the body.
    const Vec3 position{static_cast<float>(std::sin(az) * std::cos(el)),
                        static_cast<float>(std::sin(el)),
                        static_cast<float>(std::cos(az) * std::cos(el))};
    ViewFrame frame;
    frame.dir = normalize(position * -1.0f);
    // World up is +Y; the image vertical axis is up projected onto the plane
    // perpendicular to the view direction.
    Vec3 up{0.0f, 1.0f, 0.0f};
    Vec3 right = up.cross(frame.dir);
    if (right.lengthSq() < 1e-9f) right = Vec3{1.0f, 0.0f, 0.0f};
    frame.right = normalize(right);
    frame.up = normalize(frame.dir.cross(frame.right));
    return frame;
}

const std::vector<View>& fixedViews() {
    // Azimuth 0 looks at the front (+Z towards the camera); +90 looks at the
    // character's left (+X). Elevation 0 is eye level.
    static const std::vector<View> views = {
        {"front", 0.0f, 0.0f},
        {"front-45-left", 45.0f, 0.0f},
        {"side-left", 90.0f, 0.0f},
        {"side-left-30up", 90.0f, 30.0f},
        {"back", 180.0f, 0.0f},
        {"back-45-right", 225.0f, 0.0f},
        {"front-30up", 0.0f, 30.0f},
    };
    return views;
}

std::vector<Vec3> projectView(const MeshData& mesh, const View& view) {
    const ViewFrame frame = viewFrame(view);
    std::vector<Vec3> projected;
    projected.reserve(mesh.vertices.size());
    for (const Vertex& v : mesh.vertices) {
        const Vec3& p = v.position;
        projected.push_back(Vec3{p.dot(frame.right), p.dot(frame.up), p.dot(frame.dir)});
    }
    return projected;
}

Silhouette extractSilhouette(const MeshData& mesh, const View& view,
                             const Vec3& axisLowWorld, const Vec3& axisHighWorld,
                             float yMin, float yMax) {
    Silhouette silhouette;
    silhouette.radius.assign(static_cast<size_t>(Silhouette::kBins),
                             std::numeric_limits<double>::quiet_NaN());

    const ViewFrame frame = viewFrame(view);
    // The body axis in image space, projected through the SAME frame as the
    // vertices.
    const Vec3 axisA = axisLowWorld;
    const Vec3 axisB = axisHighWorld;
    const float centreU = (axisA.dot(frame.right) + axisB.dot(frame.right)) * 0.5f;
    const float centreV = (axisA.dot(frame.up) + axisB.dot(frame.up)) * 0.5f;

    for (const Vertex& v : mesh.vertices) {
        // Clip to the shared PoC-A band.
        if (v.position.y < yMin || v.position.y > yMax) continue;
        ++silhouette.clippedVertexCount;
        const Vec3& p = v.position;
        const double du = static_cast<double>(p.dot(frame.right)) - centreU;
        const double dv = static_cast<double>(p.dot(frame.up)) - centreV;
        const double radius = std::sqrt(du * du + dv * dv);
        double angle = std::atan2(dv, du);
        if (angle < 0.0) angle += 2.0 * kPi;
        int bin = static_cast<int>(angle / (2.0 * kPi) * Silhouette::kBins);
        if (bin < 0) bin = 0;
        if (bin >= Silhouette::kBins) bin = Silhouette::kBins - 1;
        double& slot = silhouette.radius[static_cast<size_t>(bin)];
        if (std::isnan(slot) || radius > slot) slot = radius;
    }
    for (double r : silhouette.radius) {
        if (!std::isnan(r)) ++silhouette.filledBins;
    }
    return silhouette;
}

SilhouetteDelta compareSilhouettes(const Silhouette& reference, const Silhouette& other) {
    SilhouetteDelta delta;
    std::vector<double> diffs;
    for (size_t i = 0; i < reference.radius.size() && i < other.radius.size(); ++i) {
        const double a = reference.radius[i];
        const double b = other.radius[i];
        if (std::isnan(a) || std::isnan(b)) continue;
        diffs.push_back(b - a);
    }
    if (diffs.empty()) return delta;

    double sumAbs = 0.0;
    double maxAbs = 0.0;
    for (double d : diffs) {
        sumAbs += std::fabs(d);
        maxAbs = std::max(maxAbs, std::fabs(d));
    }
    std::vector<double> sorted = diffs;
    std::sort(sorted.begin(), sorted.end());
    const double idx = 0.5 * static_cast<double>(sorted.size() - 1);
    const size_t lo = static_cast<size_t>(std::floor(idx));
    const size_t hi = static_cast<size_t>(std::ceil(idx));
    const double frac = idx - static_cast<double>(lo);
    delta.p50 = sorted[lo] * (1.0 - frac) + sorted[hi] * frac;

    const double idx95 = 0.95 * static_cast<double>(sorted.size() - 1);
    const size_t lo95 = static_cast<size_t>(std::floor(idx95));
    const size_t hi95 = static_cast<size_t>(std::ceil(idx95));
    const double frac95 = idx95 - static_cast<double>(lo95);
    delta.p95 = sorted[lo95] * (1.0 - frac95) + sorted[hi95] * frac95;

    delta.maxAbs = maxAbs;
    delta.meanAbs = sumAbs / static_cast<double>(diffs.size());
    delta.comparedBins = static_cast<int>(diffs.size());
    delta.valid = true;
    return delta;
}

} // namespace pae::shape
