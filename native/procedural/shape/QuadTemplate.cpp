#include "procedural/shape/QuadTemplate.h"

#include <cmath>
#include <cstdio>
#include <functional>

namespace pae::shape {
namespace {

constexpr float kPi = 3.14159265358979323846f;

// Uniform cubic B-spline basis on one segment.
void cubicBasis(float s, float w[4]) {
    const float s2 = s * s;
    const float s3 = s2 * s;
    w[0] = (1.0f - s) * (1.0f - s) * (1.0f - s) / 6.0f;
    w[1] = (3.0f * s3 - 6.0f * s2 + 4.0f) / 6.0f;
    w[2] = (-3.0f * s3 + 3.0f * s2 + 3.0f * s + 1.0f) / 6.0f;
    w[3] = s3 / 6.0f;
}

struct GridVertex {
    Vec3 position;
    float u = 0.0f;
    float v = 0.0f;
};

// Control lattice of the R3 surface.
//
// Vertical:   CLAMPED. The first three and last three control rows are
//             duplicated at yBottom / yTop. That duplication is what makes the
//             surface actually PASS THROUGH the end planes - without it the
//             surface stops short of them (the previous version reached only
//             ~7 cm below the chest landmark, which silently shortened the
//             whole template).
// Horizontal: PERIODIC. The control columns wrap around the body and the
//             column count equals the ring vertex count, so the ring
//             vertices land on integer spline parameters.
ControlSurface buildControlLatticeInternal(const QuadTemplateSpec& spec,
                                   const anatomy::HumanAnatomyParameters& params,
                                   const BodyJoints& joints, float yBottom, float yTop) {
    ControlSurface lattice;
    lattice.rows = spec.rings + 3;
    lattice.cols = spec.segments;
    lattice.points.assign(static_cast<size_t>(lattice.rows),
                           std::vector<Vec3>(static_cast<size_t>(lattice.cols)));
    for (int r = 0; r < lattice.rows; ++r) {
        float rowT = 0.0f;
        if (r <= 2) {
            rowT = 0.0f;
        } else if (r >= lattice.rows - 3) {
            rowT = 1.0f;
        } else {
            rowT = static_cast<float>(r - 2) / static_cast<float>(lattice.rows - 5);
        }
        const float y = lerp(yBottom, yTop, rowT);
        const EnvelopeSection section = envelopeAt(params, joints, y);
        for (int c = 0; c < lattice.cols; ++c) {
            const float angle = 2.0f * kPi * static_cast<float>(c) / static_cast<float>(lattice.cols);
            lattice.points[static_cast<size_t>(r)][static_cast<size_t>(c)] =
                Vec3{section.halfWidth * std::cos(angle), y, section.halfDepth * std::sin(angle)};
        }
    }
    return lattice;
}

Vec3 evaluateLatticeInternal(const ControlSurface& lattice, float v, float u) {
    const int rows = lattice.rows;
    const int cols = lattice.cols;
    if (rows < 4 || cols < 3) return Vec3{0.0f, 0.0f, 0.0f};

    // Clamped uniform cubic B-spline in the vertical direction.
    const float tv = v * static_cast<float>(rows - 3);
    int iv = static_cast<int>(std::floor(tv));
    if (iv < 0) iv = 0;
    if (iv > rows - 4) iv = rows - 4;
    float wv[4];
    cubicBasis(tv - static_cast<float>(iv), wv);

    // Periodic uniform cubic B-spline around the body. Evaluating at tu and
    // tu + cols gives the same point, so the ring closes without a seam.
    const float tu = u * static_cast<float>(cols);
    const int iu = static_cast<int>(std::floor(tu));
    float wu[4];
    cubicBasis(tu - static_cast<float>(iu), wu);

    Vec3 p{0.0f, 0.0f, 0.0f};
    for (int a = 0; a < 4; ++a) {
        const int row = iv + a;
        for (int b = 0; b < 4; ++b) {
            const int col = ((iu + b) % cols + cols) % cols;
            const Vec3& cp = lattice.points[static_cast<size_t>(row)][static_cast<size_t>(col)];
            const float w = wv[a] * wu[b];
            p.x += cp.x * w;
            p.y += cp.y * w;
            p.z += cp.z * w;
        }
    }
    return p;
}

// Builds the mesh (periodic ring grid + optional triangle-fan caps) from a
// ring grid. The ring is NOT seam-duplicated: a duplicated seam vertex would
// leave two coincident but distinct vertices and the surface would not be
// watertight. The price is one UV seam, which is irrelevant for topology.
//
// T2 (SPEC 10.1): the top ring is the shoulder-root crop boundary and stays
// OPEN by default. It is the explicit attachment ring for a future upper arm,
// and its boundary edges are registered as a crop so the topology audit can
// tell them apart from an accidental crack.
TemplateMesh assembleTemplate(const QuadTemplateSpec& spec,
                               const std::vector<GridVertex>& grid,
                               float yBottom, float yTop) {
    TemplateMesh out;
    const int rings = spec.rings;
    const int segments = spec.segments;
    if (grid.size() != static_cast<size_t>(rings * segments)) {
        out.error = "template grid size mismatch";
        return out;
    }

    out.mesh.vertices.reserve(grid.size() + 2);
    for (const GridVertex& g : grid) {
        Vertex v;
        v.position = g.position;
        v.uv = Vec2{g.u, g.v};
        v.joints = U16Vec4{0, 0, 0, 0};
        v.weights = Vec4{1.0f, 0.0f, 0.0f, 0.0f};
        v.color = Vec4{0.78f, 0.78f, 0.80f, 1.0f};
        out.mesh.vertices.push_back(v);
    }
    out.mesh.indices.reserve(static_cast<size_t>((rings - 1) * segments * 6 + segments * 6));
    out.quads.reserve(static_cast<size_t>((rings - 1) * segments));
    for (int i = 0; i + 1 < rings; ++i) {
        for (int j = 0; j < segments; ++j) {
            const int jn = (j + 1) % segments;
            const uint32_t a = static_cast<uint32_t>(i * segments + j);
            const uint32_t b = static_cast<uint32_t>(i * segments + jn);
            const uint32_t c = static_cast<uint32_t>((i + 1) * segments + jn);
            const uint32_t d = static_cast<uint32_t>((i + 1) * segments + j);
            out.quads.push_back({a, b, c, d});
            out.mesh.indices.push_back(a);
            out.mesh.indices.push_back(c);
            out.mesh.indices.push_back(b);
            out.mesh.indices.push_back(a);
            out.mesh.indices.push_back(d);
            out.mesh.indices.push_back(c);
        }
    }

    // Bottom cap: pelvis pole, outward normal points down (-Y).
    if (spec.capBottom) {
        const uint32_t bottomCenter = static_cast<uint32_t>(out.mesh.vertices.size());
        {
            Vertex v;
            v.position = Vec3{0.0f, yBottom, 0.0f};
            v.uv = Vec2{0.5f, 0.0f};
            v.weights = Vec4{1.0f, 0.0f, 0.0f, 0.0f};
            v.color = Vec4{0.78f, 0.78f, 0.80f, 1.0f};
            out.mesh.vertices.push_back(v);
        }
        for (int j = 0; j < segments; ++j) {
            const uint32_t v0 = static_cast<uint32_t>(j);
            const uint32_t v1 = static_cast<uint32_t>((j + 1) % segments);
            out.mesh.indices.push_back(bottomCenter);
            out.mesh.indices.push_back(v1);
            out.mesh.indices.push_back(v0);
        }
    }

    // Top: T2 shoulder-root crop boundary.
    //
    // The top ring is LEFT OPEN. It is the explicit boundary where the upper
    // arm attaches; the boundary edges it contributes are registered as a crop
    // so the topology audit can tell them apart from an accidental crack.
    if (spec.capTop) {
        const uint32_t topCenter = static_cast<uint32_t>(out.mesh.vertices.size());
        {
            Vertex v;
            v.position = Vec3{0.0f, yTop, 0.0f};
            v.uv = Vec2{0.5f, 1.0f};
            v.weights = Vec4{1.0f, 0.0f, 0.0f, 0.0f};
            v.color = Vec4{0.78f, 0.78f, 0.80f, 1.0f};
            out.mesh.vertices.push_back(v);
        }
        const uint32_t ringOffset = static_cast<uint32_t>((rings - 1) * segments);
        for (int j = 0; j < segments; ++j) {
            const uint32_t v0 = ringOffset + static_cast<uint32_t>(j);
            const uint32_t v1 = ringOffset + static_cast<uint32_t>((j + 1) % segments);
            out.mesh.indices.push_back(topCenter);
            out.mesh.indices.push_back(v0);
            out.mesh.indices.push_back(v1);
        }
    } else {
        // The crop records the ACTUAL height of the boundary ring, not the
        // nominal top plane: a route whose surface does not reach the nominal
        // plane would otherwise register a boundary that is not there.
        float sumY = 0.0f;
        for (int j = 0; j < segments; ++j) {
            sumY += grid[static_cast<size_t>((rings - 1) * segments + j)].position.y;
        }
        CropBoundary crop;
        crop.label = "shoulder-root";
        crop.ringIndex = rings - 1;
        crop.vertexCount = segments;
        crop.edgeCount = segments;
        crop.loopCount = 1;
        crop.closed = true;
        crop.y = sumY / static_cast<float>(segments);
        out.cropBoundaries.push_back(crop);
    }

    computeVertexNormals(out.mesh);
    for (const Vertex& v : out.mesh.vertices) out.mesh.bounds.expand(v.position);
    out.valid = true;
    return out;
}

// Marches from `origin` along `dir` until the field turns positive, then
// refines the crossing by bisection. Returns false when no crossing is found.
bool marchOutward(const std::function<float(Vec3)>& field, const Vec3& origin,
                  const Vec3& dir, float maxDistance, Vec3& hit, std::string& error) {
    constexpr float kStep = 0.006f;
    Vec3 inside = origin;
    bool haveInside = field(origin) < 0.0f;
    if (!haveInside) {
        // The axis point may sit outside the body (e.g. above the rib cage):
        // step inward first to find a point that is definitely inside.
        for (float t = kStep; t <= 0.45f; t += kStep) {
            const Vec3 probe = origin - dir * t;
            if (field(probe) < 0.0f) {
                inside = probe;
                haveInside = true;
                break;
            }
        }
        if (!haveInside) {
            error = "no interior point found for the template ray";
            return false;
        }
    }

    Vec3 previous = inside;
    Vec3 current = inside;
    bool crossed = false;
    for (float t = kStep; t <= maxDistance; t += kStep) {
        current = inside + dir * t;
        if (field(current) >= 0.0f) {
            crossed = true;
            break;
        }
        previous = current;
    }
    if (!crossed) {
        error = "template ray did not reach the surface";
        return false;
    }
    Vec3 lo = previous;
    Vec3 hi = current;
    for (int it = 0; it < 18; ++it) {
        const Vec3 mid = (lo + hi) * 0.5f;
        if (field(mid) < 0.0f) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    hit = (lo + hi) * 0.5f;
    return true;
}

} // namespace

TemplateTopology templateTopology(const QuadTemplateSpec& spec) {
    TemplateTopology t;
    t.rings = spec.rings;
    t.segments = spec.segments;
    t.vertexCount = spec.rings * spec.segments + (spec.capBottom ? 1 : 0) +
                    (spec.capTop ? 1 : 0);
    t.quadCount = (spec.rings - 1) * spec.segments;
    t.triangleCount = t.quadCount * 2 + (spec.capBottom ? spec.segments : 0) +
                      (spec.capTop ? spec.segments : 0);
    return t;
}

EnvelopeSection envelopeAt(const anatomy::HumanAnatomyParameters& params,
                           const BodyJoints& joints, float y) {
    const float volumeScale = 0.92f + 0.08f * params.ribCageVolume;

    struct Key {
        float y;
        float halfWidth;
        float halfDepth;
    };
    const Key keys[4] = {
        {joints.hips.y,
         0.170f * params.pelvisWidth,
         0.115f * params.pelvisWidth},
        {joints.spine.y,
         0.150f * params.waistWidth * (1.0f - 0.22f * params.waistCurvature),
         0.100f * params.waistDepth},
        {joints.chest.y,
         0.185f * params.chestWidth * volumeScale,
         0.125f * params.chestDepth * volumeScale},
        {joints.chest.y + 0.05f,
         (joints.leftShoulder.x + 0.086f) * params.shoulderWidth,
         0.115f * params.chestDepth * volumeScale},
    };

    if (y <= keys[0].y) return EnvelopeSection{keys[0].halfWidth, keys[0].halfDepth};
    for (int i = 0; i < 3; ++i) {
        if (y <= keys[i + 1].y) {
            const float span = keys[i + 1].y - keys[i].y;
            const float t = span > 1e-6f ? (y - keys[i].y) / span : 0.0f;
            return EnvelopeSection{lerp(keys[i].halfWidth, keys[i + 1].halfWidth, t),
                                   lerp(keys[i].halfDepth, keys[i + 1].halfDepth, t)};
        }
    }
    return EnvelopeSection{keys[3].halfWidth, keys[3].halfDepth};
}

TemplateMesh projectTemplateOntoField(const QuadTemplateSpec& spec,
                                       const anatomy::HumanAnatomyParameters& params,
                                       const BodyJoints& joints) {
    TemplateMesh out;
    if (spec.rings < 2 || spec.segments < 3) {
        out.error = "invalid template spec";
        return out;
    }
    const std::function<float(Vec3)> field = anatomy::makeUpperBodyField(params, joints);
    const float yBottom = joints.hips.y + spec.yBottomOffset;
    const float yTop = joints.chest.y + spec.yTopOffset;

    std::vector<GridVertex> grid(static_cast<size_t>(spec.rings * spec.segments));
    for (int i = 0; i < spec.rings; ++i) {
        const float v = spec.rings > 1 ? static_cast<float>(i) / static_cast<float>(spec.rings - 1) : 0.0f;
        const float y = lerp(yBottom, yTop, v);
        for (int j = 0; j < spec.segments; ++j) {
            const float u = static_cast<float>(j) / static_cast<float>(spec.segments);
            const float angle = 2.0f * kPi * u;
            const Vec3 dir{std::cos(angle), 0.0f, std::sin(angle)};
            const Vec3 origin{0.0f, y, 0.0f};
            Vec3 hit{};
            if (!marchOutward(field, origin, dir, 0.55f, hit, out.error)) {
                out.error = "R2 projection failed at u=" + std::to_string(u) +
                            " v=" + std::to_string(v) + ": " + out.error;
                return out;
            }
            GridVertex& g = grid[static_cast<size_t>(i * spec.segments + j)];
            g.position = hit;
            g.u = u;
            g.v = v;
        }
    }
    return assembleTemplate(spec, grid, yBottom, yTop);
}

ControlSurface buildControlSurface(const QuadTemplateSpec& spec,
                                   const anatomy::HumanAnatomyParameters& params,
                                   const BodyJoints& joints) {
    ControlSurface surface = buildControlLatticeInternal(spec, params, joints,
                                                          joints.hips.y + spec.yBottomOffset,
                                                          joints.chest.y + spec.yTopOffset);
    return surface;
}

Vec3 evaluateControlSurface(const ControlSurface& surface, float v, float u) {
    return evaluateLatticeInternal(surface, v, u);
}

TemplateMesh buildControlSurfaceTemplate(const QuadTemplateSpec& spec,
                                         const anatomy::HumanAnatomyParameters& params,
                                         const BodyJoints& joints) {
    TemplateMesh out;
    if (spec.rings < 2 || spec.segments < 3) {
        out.error = "invalid template spec";
        return out;
    }
    const float yBottom = joints.hips.y + spec.yBottomOffset;
    const float yTop = joints.chest.y + spec.yTopOffset;

    const ControlSurface lattice = buildControlLatticeInternal(spec, params, joints, yBottom, yTop);

    std::vector<GridVertex> grid(static_cast<size_t>(spec.rings * spec.segments));
    for (int i = 0; i < spec.rings; ++i) {
        const float v = spec.rings > 1 ? static_cast<float>(i) / static_cast<float>(spec.rings - 1) : 0.0f;
        for (int j = 0; j < spec.segments; ++j) {
            const float u = static_cast<float>(j) / static_cast<float>(spec.segments);
            GridVertex& g = grid[static_cast<size_t>(i * spec.segments + j)];
            g.position = evaluateLatticeInternal(lattice, v, u);
            g.u = u;
            g.v = v;
        }
    }
    return assembleTemplate(spec, grid, yBottom, yTop);
}

float torsoRadiusAt(const std::function<float(Vec3)>& field, float y, float u,
                    float maxDistance) {
    const float angle = 2.0f * kPi * u;
    const Vec3 dir{std::cos(angle), 0.0f, std::sin(angle)};
    const Vec3 origin{0.0f, y, 0.0f};
    Vec3 hit{};
    std::string error;
    if (!marchOutward(field, origin, dir, maxDistance, hit, error)) return -1.0f;
    return std::hypot(hit.x, hit.z);
}

float torsoRadiusAt(const anatomy::HumanAnatomyParameters& params,
                    const BodyJoints& joints, float y, float u,
                    float maxDistance) {
    const std::function<float(Vec3)> field = anatomy::makeUpperBodyField(params, joints);
    return torsoRadiusAt(field, y, u, maxDistance);
}

void computeVertexNormals(MeshData& mesh) {
    for (Vertex& v : mesh.vertices) v.normal = Vec3{0.0f, 0.0f, 0.0f};
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const Vec3& a = mesh.vertices[mesh.indices[i]].position;
        const Vec3& b = mesh.vertices[mesh.indices[i + 1]].position;
        const Vec3& c = mesh.vertices[mesh.indices[i + 2]].position;
        const Vec3 n = (b - a).cross(c - a);
        for (int k = 0; k < 3; ++k) {
            Vertex& v = mesh.vertices[mesh.indices[i + static_cast<size_t>(k)]];
            v.normal = v.normal + n;
        }
    }
    for (Vertex& v : mesh.vertices) {
        const float len = v.normal.length();
        v.normal = len > 1e-8f ? v.normal * (1.0f / len) : Vec3{0.0f, 1.0f, 0.0f};
    }
}

std::string writeObj(const MeshData& mesh, const std::string& path) {
    std::FILE* file = std::fopen(path.c_str(), "wb");
    if (!file) return "cannot open " + path;
    for (const Vertex& v : mesh.vertices) {
        std::fprintf(file, "v %.6f %.6f %.6f\n", v.position.x, v.position.y, v.position.z);
    }
    for (const Vertex& v : mesh.vertices) {
        std::fprintf(file, "vn %.6f %.6f %.6f\n", v.normal.x, v.normal.y, v.normal.z);
    }
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const std::uint32_t a = mesh.indices[i] + 1;
        const std::uint32_t b = mesh.indices[i + 1] + 1;
        const std::uint32_t c = mesh.indices[i + 2] + 1;
        std::fprintf(file, "f %u//%u %u//%u %u//%u\n", a, a, b, b, c, c);
    }
    std::fclose(file);
    return std::string();
}

} // namespace pae::shape
