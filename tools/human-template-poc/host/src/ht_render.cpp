#include "ht_render.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

#ifdef PAE_HT_WITH_PAE_EVIDENCE
#include "procedural/shape/CameraProtocol.h"
#include "procedural/shape/ClayRenderer.h"
#include "procedural/shape/ImageWriter.h"
#endif

namespace pae::ht {

namespace {

constexpr double kPi = 3.14159265358979323846;

struct ViewFrame {
    D3 right, up, dir;
};

// Same construction as PAE's viewFrame(): camera on a sphere, looking back.
ViewFrame makeFrame(double azimuthDeg, double elevationDeg) {
    const double az = azimuthDeg * kPi / 180.0, el = elevationDeg * kPi / 180.0;
    const D3 position{std::sin(az) * std::cos(el), std::sin(el), std::cos(az) * std::cos(el)};
    ViewFrame f;
    f.dir = (position * -1.0).normalized();
    D3 up{0.0, 1.0, 0.0};
    D3 right = up.cross(f.dir);
    if (right.lengthSq() < 1e-9) right = D3{1.0, 0.0, 0.0};
    f.right = right.normalized();
    f.up = f.dir.cross(f.right).normalized();
    return f;
}

struct VertexAttr {
    double u, v, d;       // image plane (metres) + depth along dir
    float r, g, b;
};

void toAttr(const D3& p, const Camera& c, VertexAttr& a) {
    a.u = p.x * c.right[0] + p.y * c.right[1] + p.z * c.right[2];
    a.v = p.x * c.up[0] + p.y * c.up[1] + p.z * c.up[2];
    a.d = p.x * c.dir[0] + p.y * c.dir[1] + p.z * c.dir[2];
}

void set(Rgb& image, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < 0 || y < 0 || x >= image.width || y >= image.height) return;
    const size_t i = (static_cast<size_t>(y) * static_cast<size_t>(image.width) + static_cast<size_t>(x)) * 3;
    image.pixels[i] = r;
    image.pixels[i + 1] = g;
    image.pixels[i + 2] = b;
}

// Edge-function triangle fill with a depth buffer (nearer = smaller d).
void fillTriangle(Rgb& image, std::vector<double>& depth, const Camera& c, const VertexAttr& a0, const VertexAttr& a1,
                  const VertexAttr& a2, uint8_t r, uint8_t g, uint8_t b) {
    const double sx = static_cast<double>(c.width) / (c.uMax - c.uMin);
    const double sy = static_cast<double>(c.height) / (c.vMax - c.vMin);
    auto screen = [&](const VertexAttr& a) { return std::pair<double, double>{(a.u - c.uMin) * sx, (c.vMax - a.v) * sy}; };
    const auto p0 = screen(a0), p1 = screen(a1), p2 = screen(a2);
    const int minX = std::max(0, static_cast<int>(std::floor(std::min({p0.first, p1.first, p2.first}))));
    const int maxX = std::min(c.width - 1, static_cast<int>(std::ceil(std::max({p0.first, p1.first, p2.first}))));
    const int minY = std::max(0, static_cast<int>(std::floor(std::min({p0.second, p1.second, p2.second}))));
    const int maxY = std::min(c.height - 1, static_cast<int>(std::ceil(std::max({p0.second, p1.second, p2.second}))));
    if (minX > maxX || minY > maxY) return;
    const double area = (p1.first - p0.first) * (p2.second - p0.second) - (p2.first - p0.first) * (p1.second - p0.second);
    if (std::fabs(area) < 1e-12) return;
    for (int y = minY; y <= maxY; ++y) {
        const double py = y + 0.5;
        for (int x = minX; x <= maxX; ++x) {
            const double px = x + 0.5;
            const double w0 = ((p1.first - px) * (p2.second - py) - (p2.first - px) * (p1.second - py)) / area;
            const double w1 = ((p2.first - px) * (p0.second - py) - (p0.first - px) * (p2.second - py)) / area;
            const double w2 = 1.0 - w0 - w1;
            if (w0 < 0.0 || w1 < 0.0 || w2 < 0.0) continue;
            const double d = w0 * a0.d + w1 * a1.d + w2 * a2.d;
            const size_t idx = static_cast<size_t>(y) * static_cast<size_t>(c.width) + static_cast<size_t>(x);
            if (d >= depth[idx]) continue;
            depth[idx] = d;
            set(image, x, y, r, g, b);
        }
    }
}

void fillTriangleGouraud(Rgb& image, std::vector<double>& depth, const Camera& c, const VertexAttr& a0, const VertexAttr& a1,
                         const VertexAttr& a2) {
    const double sx = static_cast<double>(c.width) / (c.uMax - c.uMin);
    const double sy = static_cast<double>(c.height) / (c.vMax - c.vMin);
    auto screen = [&](const VertexAttr& a) { return std::pair<double, double>{(a.u - c.uMin) * sx, (c.vMax - a.v) * sy}; };
    const auto p0 = screen(a0), p1 = screen(a1), p2 = screen(a2);
    const int minX = std::max(0, static_cast<int>(std::floor(std::min({p0.first, p1.first, p2.first}))));
    const int maxX = std::min(c.width - 1, static_cast<int>(std::ceil(std::max({p0.first, p1.first, p2.first}))));
    const int minY = std::max(0, static_cast<int>(std::floor(std::min({p0.second, p1.second, p2.second}))));
    const int maxY = std::min(c.height - 1, static_cast<int>(std::ceil(std::max({p0.second, p1.second, p2.second}))));
    if (minX > maxX || minY > maxY) return;
    const double area = (p1.first - p0.first) * (p2.second - p0.second) - (p2.first - p0.first) * (p1.second - p0.second);
    if (std::fabs(area) < 1e-12) return;
    for (int y = minY; y <= maxY; ++y) {
        const double py = y + 0.5;
        for (int x = minX; x <= maxX; ++x) {
            const double px = x + 0.5;
            const double w0 = ((p1.first - px) * (p2.second - py) - (p2.first - px) * (p1.second - py)) / area;
            const double w1 = ((p2.first - px) * (p0.second - py) - (p0.first - px) * (p2.second - py)) / area;
            const double w2 = 1.0 - w0 - w1;
            if (w0 < 0.0 || w1 < 0.0 || w2 < 0.0) continue;
            const double d = w0 * a0.d + w1 * a1.d + w2 * a2.d;
            const size_t idx = static_cast<size_t>(y) * static_cast<size_t>(c.width) + static_cast<size_t>(x);
            if (d >= depth[idx]) continue;
            depth[idx] = d;
            set(image, x, y,
                static_cast<uint8_t>(std::clamp(w0 * a0.r + w1 * a1.r + w2 * a2.r, 0.0, 255.0)),
                static_cast<uint8_t>(std::clamp(w0 * a0.g + w1 * a1.g + w2 * a2.g, 0.0, 255.0)),
                static_cast<uint8_t>(std::clamp(w0 * a0.b + w1 * a1.b + w2 * a2.b, 0.0, 255.0)));
        }
    }
}

// Deterministic palette from a small integer (bone index / region).
void palette(int index, float& r, float& g, float& b) {
    static const float table[12][3] = {
        {0.85f, 0.33f, 0.10f}, {0.90f, 0.65f, 0.10f}, {0.55f, 0.80f, 0.20f}, {0.20f, 0.75f, 0.55f},
        {0.15f, 0.65f, 0.85f}, {0.25f, 0.40f, 0.90f}, {0.55f, 0.30f, 0.90f}, {0.80f, 0.30f, 0.85f},
        {0.90f, 0.40f, 0.55f}, {0.65f, 0.50f, 0.35f}, {0.45f, 0.55f, 0.25f}, {0.70f, 0.70f, 0.72f}};
    const float* c = table[((index % 12) + 12) % 12];
    r = c[0] * 255.0f;
    g = c[1] * 255.0f;
    b = c[2] * 255.0f;
}

void drawLine3D(Rgb& image, const std::vector<double>& depth, const Camera& c, const VertexAttr& a, const VertexAttr& b,
                double bias, uint8_t r, uint8_t g, uint8_t bl) {
    const double sx = static_cast<double>(c.width) / (c.uMax - c.uMin);
    const double sy = static_cast<double>(c.height) / (c.vMax - c.vMin);
    const double ax = (a.u - c.uMin) * sx, ay = (c.vMax - a.v) * sy;
    const double bx = (b.u - c.uMin) * sx, by = (c.vMax - b.v) * sy;
    const int steps = std::max(1, static_cast<int>(std::ceil(std::max(std::fabs(bx - ax), std::fabs(by - ay)))));
    for (int i = 0; i <= steps; ++i) {
        const double t = static_cast<double>(i) / steps;
        const int x = static_cast<int>(ax + (bx - ax) * t), y = static_cast<int>(ay + (by - ay) * t);
        if (x < 0 || y < 0 || x >= c.width || y >= c.height) continue;
        const double d = a.d + (b.d - a.d) * t;
        if (d > depth[static_cast<size_t>(y) * static_cast<size_t>(c.width) + static_cast<size_t>(x)] + bias) continue;
        set(image, x, y, r, g, bl);
    }
}

} // namespace

std::vector<Camera> protocolCameras(int width, int height) {
    // PAE CameraProtocol::fixedViews(): name, azimuth, elevation.
    const struct { const char* name; double az, el; } views[] = {
        {"front", 0.0, 0.0},          {"front-45-left", 45.0, 0.0},   {"side-left", 90.0, 0.0},
        {"side-left-30up", 90.0, 30.0}, {"back", 180.0, 0.0},          {"back-45-right", 225.0, 0.0},
        {"front-30up", 0.0, 30.0},    {"side-right", 270.0, 0.0}};
    std::vector<Camera> cameras;
    for (const auto& v : views) {
        Camera c;
        c.name = v.name;
        c.width = width;
        c.height = height;
        const ViewFrame f = makeFrame(v.az, v.el);
        c.right[0] = static_cast<float>(f.right.x); c.right[1] = static_cast<float>(f.right.y); c.right[2] = static_cast<float>(f.right.z);
        c.up[0] = static_cast<float>(f.up.x); c.up[1] = static_cast<float>(f.up.y); c.up[2] = static_cast<float>(f.up.z);
        c.dir[0] = static_cast<float>(f.dir.x); c.dir[1] = static_cast<float>(f.dir.y); c.dir[2] = static_cast<float>(f.dir.z);
        cameras.push_back(c);
    }
    return cameras;
}

Camera makeCamera(const std::string& name, double azimuthDeg, double elevationDeg, int width, int height) {
    Camera c;
    c.name = name;
    c.width = width;
    c.height = height;
    const ViewFrame f = makeFrame(azimuthDeg, elevationDeg);
    c.right[0] = static_cast<float>(f.right.x); c.right[1] = static_cast<float>(f.right.y); c.right[2] = static_cast<float>(f.right.z);
    c.up[0] = static_cast<float>(f.up.x); c.up[1] = static_cast<float>(f.up.y); c.up[2] = static_cast<float>(f.up.z);
    c.dir[0] = static_cast<float>(f.dir.x); c.dir[1] = static_cast<float>(f.dir.y); c.dir[2] = static_cast<float>(f.dir.z);
    return c;
}

void fitCameraToBox(Camera& c, const D3& lo, const D3& hi) {
    // The eight box corners projected into the image plane give the extent.
    double uMin = 1e30, uMax = -1e30, vMin = 1e30, vMax = -1e30;
    for (int i = 0; i < 8; ++i) {
        const D3 p{i & 1 ? hi.x : lo.x, i & 2 ? hi.y : lo.y, i & 4 ? hi.z : lo.z};
        VertexAttr a;
        toAttr(p, c, a);
        uMin = std::min(uMin, a.u); uMax = std::max(uMax, a.u);
        vMin = std::min(vMin, a.v); vMax = std::max(vMax, a.v);
    }
    const double margin = 0.08 * std::max(uMax - uMin, vMax - vMin);
    const double cu = (uMin + uMax) * 0.5, spanU = (uMax - uMin) + 2.0 * margin;
    const double cv = (vMin + vMax) * 0.5, spanV = (vMax - vMin) + 2.0 * margin;
    const double aspect = static_cast<double>(c.width) / static_cast<double>(c.height);
    const double halfU = std::max(spanU * 0.5, spanV * 0.5 * aspect);
    const double halfV = std::max(spanV * 0.5, spanU * 0.5 / aspect);
    c.uMin = static_cast<float>(cu - halfU); c.uMax = static_cast<float>(cu + halfU);
    c.vMin = static_cast<float>(cv - halfV); c.vMax = static_cast<float>(cv + halfV);
}

void fitSharedFrames(std::vector<Camera>& cameras, const std::vector<const std::vector<D3>*>& positionSets) {
    for (Camera& c : cameras) {
        double uMin = 1e30, uMax = -1e30, vMin = 1e30, vMax = -1e30;
        for (const std::vector<D3>* set : positionSets) {
            if (!set) continue;
            for (const D3& p : *set) {
                VertexAttr a;
                toAttr(p, c, a);
                uMin = std::min(uMin, a.u); uMax = std::max(uMax, a.u);
                vMin = std::min(vMin, a.v); vMax = std::max(vMax, a.v);
            }
        }
        // Same 6 % margin as PAE's computeSharedClayFrames, applied to the
        // larger image extent so the whole figure always fits.
        const double margin = 0.06 * std::max(uMax - uMin, vMax - vMin);
        const double cu = (uMin + uMax) * 0.5, spanU = (uMax - uMin) + 2.0 * margin;
        const double cv = (vMin + vMax) * 0.5, spanV = (vMax - vMin) + 2.0 * margin;
        const double aspect = static_cast<double>(c.width) / static_cast<double>(c.height);
        const double halfU = std::max(spanU * 0.5, spanV * 0.5 * aspect);
        const double halfV = std::max(spanV * 0.5, spanU * 0.5 / aspect);
        c.uMin = static_cast<float>(cu - halfU); c.uMax = static_cast<float>(cu + halfU);
        c.vMin = static_cast<float>(cv - halfV); c.vMax = static_cast<float>(cv + halfV);
    }
}

Rgb renderSurface(const RenderSurface& s, const Camera& c, ShadeMode mode, bool wireOverlay) {
    Rgb image;
    image.width = c.width;
    image.height = c.height;
    // Same background as PAE's ClayRenderer.
    image.pixels.assign(static_cast<size_t>(c.width) * static_cast<size_t>(c.height) * 3, 0);
    for (size_t i = 0; i < image.pixels.size(); i += 3) { image.pixels[i] = 28; image.pixels[i + 1] = 28; image.pixels[i + 2] = 31; }
    std::vector<double> depth(static_cast<size_t>(c.width) * static_cast<size_t>(c.height), 1e30);
    if (!s.mesh || !s.positions || s.mesh->faces.empty()) return image;

    const PolyMesh& mesh = *s.mesh;
    const std::vector<D3>& pos = *s.positions;
    std::vector<VertexAttr> attr(pos.size());
    for (size_t v = 0; v < pos.size(); ++v) {
        toAttr(pos[v], c, attr[v]);
        if (mode == ShadeMode::WeightHeat || mode == ShadeMode::RegionColor) {
            float r = 200.0f, g = 200.0f, b = 200.0f;
            if (mode == ShadeMode::WeightHeat && s.weights && s.rig && v < static_cast<size_t>(s.weights->vertexCount)) {
                const uint8_t n = s.weights->count[v];
                int best = -1;
                float bestW = 0.0f;
                for (int k = 0; k < n; ++k)
                    if (s.weights->w[v][static_cast<size_t>(k)] > bestW) {
                        bestW = s.weights->w[v][static_cast<size_t>(k)];
                        best = s.weights->bone[v][static_cast<size_t>(k)];
                    }
                if (best >= 0) {
                    palette(best, r, g, b);
                    const float k = 0.35f + 0.65f * bestW;  // brighter = more single-bone
                    r *= k; g *= k; b *= k;
                }
            } else if (mode == ShadeMode::RegionColor && s.regions && v < s.regions->size()) {
                palette(static_cast<int>((*s.regions)[v]), r, g, b);
            }
            attr[v].r = r; attr[v].g = g; attr[v].b = b;
        }
    }

    // PAE ClayRenderer light in the (right, up, towards camera) frame.
    const double lr = 0.55, lu = 0.78, lt = 0.45;
    D3 light = (D3{c.right[0], c.right[1], c.right[2]} * lr + D3{c.up[0], c.up[1], c.up[2]} * lu +
                 D3{c.dir[0], c.dir[1], c.dir[2]} * (-lt)).normalized();

    for (const PolyFace& f : mesh.faces) {
        const size_t n = f.v.size();
        if (n < 3) continue;
        D3 fn;
        for (size_t k = 0; k < n; ++k) {  // Newell normal of the current polygon
            const D3& a = pos[static_cast<size_t>(f.v[k])];
            const D3& b = pos[static_cast<size_t>(f.v[(k + 1) % n])];
            fn.x += (a.y - b.y) * (a.z + b.z);
            fn.y += (a.z - b.z) * (a.x + b.x);
            fn.z += (a.x - b.x) * (a.y + b.y);
        }
        fn = fn.normalized();
        // The source winding is not assumed: shade the side that faces the camera.
        const double facing = fn.x * c.dir[0] + fn.y * c.dir[1] + fn.z * c.dir[2];
        if (facing > 0.0) fn = -fn;
        uint8_t r = 200, g = 200, b = 200;
        if (mode == ShadeMode::Clay || mode == ShadeMode::Wireframe) {
            const double lambert = std::max(0.0, fn.dot(light));
            const double intensity = 0.28 + 0.72 * lambert;
            const auto shade = static_cast<uint8_t>(std::clamp(std::lround(206.0 * intensity), 0L, 255L));
            r = g = b = shade;
        } else if (mode == ShadeMode::ViewNormal) {
            r = static_cast<uint8_t>(std::clamp(std::lround(255.0 * (0.5 + 0.5 * fn.dot(D3{c.right[0], c.right[1], c.right[2]}))), 0L, 255L));
            g = static_cast<uint8_t>(std::clamp(std::lround(255.0 * (0.5 + 0.5 * fn.dot(D3{c.up[0], c.up[1], c.up[2]}))), 0L, 255L));
            b = static_cast<uint8_t>(std::clamp(std::lround(255.0 * (0.5 - 0.5 * fn.dot(D3{c.dir[0], c.dir[1], c.dir[2]}))), 0L, 255L));
        }
        for (size_t k = 1; k + 1 < n; ++k) {
            const VertexAttr& a0 = attr[static_cast<size_t>(f.v[0])];
            const VertexAttr& a1 = attr[static_cast<size_t>(f.v[k])];
            const VertexAttr& a2 = attr[static_cast<size_t>(f.v[k + 1])];
            if (mode == ShadeMode::WeightHeat || mode == ShadeMode::RegionColor)
                fillTriangleGouraud(image, depth, c, a0, a1, a2);
            else
                fillTriangle(image, depth, c, a0, a1, a2, r, g, b);
        }
    }

    if (wireOverlay || mode == ShadeMode::Wireframe) {
        double dMin = 1e30, dMax = -1e30;
        for (double d : depth) {
            if (d >= 1e29) continue;
            dMin = std::min(dMin, d);
            dMax = std::max(dMax, d);
        }
        const double bias = (dMax > dMin ? (dMax - dMin) : 1.0) * 0.004;
        std::set<std::pair<int, int>> drawn;
        for (const PolyFace& f : mesh.faces) {
            const size_t n = f.v.size();
            for (size_t k = 0; k < n; ++k) {
                // Every topological edge exactly once.
                const int ia = f.v[k], ib = f.v[(k + 1) % n];
                if (!drawn.insert({std::min(ia, ib), std::max(ia, ib)}).second) continue;
                drawLine3D(image, depth, c, attr[static_cast<size_t>(ia)], attr[static_cast<size_t>(ib)], bias, 18, 18, 22);
            }
        }
    }
    return image;
}

bool writePngFile(const std::string& path, const Rgb& image) {
#ifdef PAE_HT_WITH_PAE_EVIDENCE
    pae::shape::RgbImage png;
    png.width = image.width;
    png.height = image.height;
    png.pixels = image.pixels;
    return pae::shape::writePng(path, png);
#else
    (void)path;
    (void)image;
    return false;
#endif
}

double claySilhouetteIoUWithPae(const PolyMesh& mesh, const Camera& camera) {
#ifdef PAE_HT_WITH_PAE_EVIDENCE
    // Rebuild the PAE camera from the same azimuth/elevation through PAE's
    // own protocol, then compare filled-pixel masks.
    pae::shape::View view;
    // Recover azimuth/elevation from the frame direction (camera position = -dir).
    const D3 posDir{-camera.dir[0], -camera.dir[1], -camera.dir[2]};
    const double el = std::asin(std::clamp(posDir.y, -1.0, 1.0)) * 180.0 / kPi;
    const double az = std::atan2(posDir.x, posDir.z) * 180.0 / kPi;
    view.name = camera.name;
    view.azimuthDeg = static_cast<float>(az);
    view.elevationDeg = static_cast<float>(el);

    pae::MeshData data;
    data.vertices.reserve(mesh.pos.size());
    for (const D3& p : mesh.pos) {
        pae::Vertex v;
        v.position = pae::Vec3{static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z)};
        data.vertices.push_back(v);
    }
    for (const PolyFace& f : mesh.faces)
        for (size_t k = 1; k + 1 < f.v.size(); ++k) {
            data.indices.push_back(static_cast<uint32_t>(f.v[0]));
            data.indices.push_back(static_cast<uint32_t>(f.v[k]));
            data.indices.push_back(static_cast<uint32_t>(f.v[k + 1]));
        }
    pae::shape::ClayFrame frame;
    frame.width = camera.width;
    frame.height = camera.height;
    frame.uMin = camera.uMin; frame.uMax = camera.uMax;
    frame.vMin = camera.vMin; frame.vMax = camera.vMax;
    double yMin = 1e30, yMax = -1e30;
    for (const D3& p : mesh.pos) { yMin = std::min(yMin, p.y); yMax = std::max(yMax, p.y); }
    const pae::shape::RgbImage reference = pae::shape::renderClayView(data, view, frame, static_cast<float>(yMin), static_cast<float>(yMax));

    RenderSurface surface;
    surface.mesh = &mesh;
    surface.positions = &mesh.pos;
    const Rgb mine = renderSurface(surface, camera, ShadeMode::Clay, false);

    long unionCount = 0, intersection = 0;
    const size_t pixels = static_cast<size_t>(camera.width) * static_cast<size_t>(camera.height);
    auto isBackground = [](const std::vector<uint8_t>& px, size_t i) {
        return px[i * 3] == 28 && px[i * 3 + 1] == 28 && px[i * 3 + 2] == 31;
    };
    for (size_t i = 0; i < pixels; ++i) {
        const bool a = !isBackground(mine.pixels, i), b = !isBackground(reference.pixels, i);
        if (a && b) ++intersection;
        if (a || b) ++unionCount;
    }
    return unionCount ? static_cast<double>(intersection) / static_cast<double>(unionCount) : 0.0;
#else
    (void)mesh;
    (void)camera;
    return -1.0;
#endif
}

} // namespace pae::ht
