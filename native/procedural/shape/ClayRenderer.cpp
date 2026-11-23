#include "procedural/shape/ClayRenderer.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace pae::shape {
namespace {

// Clip a polygon against a horizontal world-space plane.
std::vector<Vec3> clipByY(const std::vector<Vec3>& poly, float y, bool keepAbove) {
    std::vector<Vec3> result;
    if (poly.size() < 3) return result;
    auto inside = [&](const Vec3& p) { return keepAbove ? p.y >= y : p.y <= y; };
    for (size_t i = 0; i < poly.size(); ++i) {
        const Vec3& cur = poly[i];
        const Vec3& next = poly[(i + 1) % poly.size()];
        const bool inCur = inside(cur);
        const bool inNext = inside(next);
        if (inCur) result.push_back(cur);
        if (inCur != inNext) {
            const float dy = next.y - cur.y;
            const float t = std::fabs(dy) < 1e-9f ? 0.0f : (y - cur.y) / dy;
            result.push_back(Vec3{cur.x + (next.x - cur.x) * t, y,
                                  cur.z + (next.z - cur.z) * t});
        }
    }
    return result;
}

struct ScreenPoint {
    float x = 0.0f;
    float y = 0.0f;
    float depth = 0.0f;
};

ScreenPoint toScreen(const Vec3& p, const ViewFrame& frame, const ClayFrame& spec) {
    const float u = p.dot(frame.right);
    const float v = p.dot(frame.up);
    ScreenPoint out;
    out.x = (u - spec.uMin) / (spec.uMax - spec.uMin) * static_cast<float>(spec.width - 1);
    out.y = (spec.vMax - v) / (spec.vMax - spec.vMin) * static_cast<float>(spec.height - 1);
    out.depth = p.dot(frame.dir);  // smaller = closer to the camera
    return out;
}

// Flat-shaded triangle with a depth buffer. Barycentric edge functions.
void rasterize(RgbImage& image, std::vector<float>& depth,
               const ScreenPoint& a, const ScreenPoint& b, const ScreenPoint& c,
               uint8_t shade) {
    const float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (std::fabs(area) < 1e-9f) return;
    // Use 1/|area| so both projected windings fill correctly.
    const float inv = (area < 0.0f ? -1.0f : 1.0f) / area;

    const int minX = std::max(0, static_cast<int>(std::floor(std::min({a.x, b.x, c.x}))));
    const int maxX = std::min(image.width - 1,
                              static_cast<int>(std::ceil(std::max({a.x, b.x, c.x}))));
    const int minY = std::max(0, static_cast<int>(std::floor(std::min({a.y, b.y, c.y}))));
    const int maxY = std::min(image.height - 1,
                              static_cast<int>(std::ceil(std::max({a.y, b.y, c.y}))));

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            const float px = static_cast<float>(x) + 0.5f;
            const float py = static_cast<float>(y) + 0.5f;
            const float w0 = ((b.x - a.x) * (py - a.y) - (b.y - a.y) * (px - a.x)) * inv;
            const float w1 = ((c.x - b.x) * (py - b.y) - (c.y - b.y) * (px - b.x)) * inv;
            const float w2 = ((a.x - c.x) * (py - c.y) - (a.y - c.y) * (px - c.x)) * inv;
            if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f) continue;
            const float d = w0 * c.depth + w1 * a.depth + w2 * b.depth;
            const size_t idx = static_cast<size_t>(y) * static_cast<size_t>(image.width) +
                               static_cast<size_t>(x);
            if (d >= depth[idx]) continue;
            depth[idx] = d;
            image.set(x, y, shade, shade, static_cast<uint8_t>(std::min(255, shade + 4)));
        }
    }
}

} // namespace

const Vec3& clayLightInViewFrame() {
    // Fixed light in the (right, up, TOWARDS CAMERA) frame. Never changed per
    // route or per view, so shading cannot favour one representation.
    static const Vec3 light{0.55f, 0.78f, 0.45f};
    return light;
}

std::vector<ClayFrame> computeSharedClayFrames(const std::vector<const MeshData*>& meshes,
                                               const std::vector<View>& views,
                                               float yMin, float yMax,
                                               int width, int height) {
    std::vector<ClayFrame> frames;
    frames.reserve(views.size());
    for (const View& view : views) {
        ClayFrame frame;
        frame.width = width;
        frame.height = height;
        const ViewFrame vf = viewFrame(view);
        double uMin = std::numeric_limits<double>::max();
        double uMax = std::numeric_limits<double>::lowest();
        double vMin = std::numeric_limits<double>::max();
        double vMax = std::numeric_limits<double>::lowest();

        for (const MeshData* mesh : meshes) {
            if (!mesh) continue;
            for (const Vertex& vertex : mesh->vertices) {
                const Vec3& p = vertex.position;
                if (p.y < yMin || p.y > yMax) continue;
                const double u = static_cast<double>(p.dot(vf.right));
                const double v = static_cast<double>(p.dot(vf.up));
                uMin = std::min(uMin, u);
                uMax = std::max(uMax, u);
                vMin = std::min(vMin, v);
                vMax = std::max(vMax, v);
            }
        }
        if (uMin <= uMax && vMin <= vMax) {
            const double margin = 0.06 * std::max(uMax - uMin, vMax - vMin);
            frame.uMin = static_cast<float>(uMin - margin);
            frame.uMax = static_cast<float>(uMax + margin);
            frame.vMin = static_cast<float>(vMin - margin);
            frame.vMax = static_cast<float>(vMax + margin);
        }
        frames.push_back(frame);
    }
    return frames;
}

RgbImage renderClayView(const MeshData& mesh, const View& view,
                        const ClayFrame& frame, float yMin, float yMax) {
    RgbImage image;
    image.width = frame.width;
    image.height = frame.height;
    image.clear(28, 28, 31);

    if (image.width <= 0 || image.height <= 0) return image;
    std::vector<float> depth(static_cast<size_t>(image.width) *
                             static_cast<size_t>(image.height),
                             std::numeric_limits<float>::max());

    const ViewFrame vf = viewFrame(view);
    const Vec3& l = clayLightInViewFrame();
    const Vec3 lightWorld = (vf.right * l.x + vf.up * l.y + vf.dir * -l.z).normalized(
        Vec3{0.0f, 1.0f, 0.0f});

    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const Vec3 a = mesh.vertices[mesh.indices[i]].position;
        const Vec3 b = mesh.vertices[mesh.indices[i + 1]].position;
        const Vec3 c = mesh.vertices[mesh.indices[i + 2]].position;

        // Clip to the shared PoC-A band with real plane clipping, so the cut
        // is clean instead of leaving holes where a triangle crosses it.
        std::vector<Vec3> poly{a, b, c};
        poly = clipByY(poly, yMin, true);
        if (poly.size() < 3) continue;
        poly = clipByY(poly, yMax, false);
        if (poly.size() < 3) continue;

        const Vec3 worldNormal = (b - a).cross(c - a).normalized(Vec3{});
        if (worldNormal.lengthSq() < 1e-9f) continue;
        // Backface cull: the camera looks along vf.dir.
        if (worldNormal.dot(vf.dir) >= 0.0f) continue;

        const float lambert = std::max(0.0f, worldNormal.dot(lightWorld));
        const float intensity = 0.28f + 0.72f * lambert;
        const uint8_t shade = static_cast<uint8_t>(std::clamp(
            static_cast<int>(std::lround(206.0f * intensity)), 0, 255));

        const ScreenPoint p0 = toScreen(poly[0], vf, frame);
        for (size_t k = 1; k + 1 < poly.size(); ++k) {
            const ScreenPoint p1 = toScreen(poly[k], vf, frame);
            const ScreenPoint p2 = toScreen(poly[k + 1], vf, frame);
            rasterize(image, depth, p0, p1, p2, shade);
        }
    }
    return image;
}

} // namespace pae::shape
