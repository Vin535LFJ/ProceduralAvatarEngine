#include "SdfFusion.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace pae::fusion {

namespace {

Vec3 hadamard(const Vec3& a, const Vec3& b) {
    return {a.x * b.x, a.y * b.y, a.z * b.z};
}

MeshData finalizeFusion(MeshData& mesh) {
    Aabb bounds;
    for (const Vertex& v : mesh.vertices) bounds.expand(v.position);
    mesh.bounds = bounds;
    if (mesh.subMeshes.empty()) {
        SubMesh sub;
        sub.indexCount = static_cast<uint32_t>(mesh.indices.size());
        mesh.subMeshes.push_back(sub);
    }
    return mesh;
}

} // namespace

float sdSphere(Vec3 p, Vec3 center, float radius) {
    return (p - center).length() - radius;
}

float sdEllipsoidApprox(Vec3 p, Vec3 center, Vec3 radii) {
    // Quadratic approximation: k*(|p/r| - 1) scaled by the smallest radius.
    const Vec3 q{(p.x - center.x) / radii.x, (p.y - center.y) / radii.y,
                 (p.z - center.z) / radii.z};
    const float k = std::min(std::min(radii.x, radii.y), radii.z);
    return k * (q.length() - 1.0f);
}

float sdCapsule(Vec3 p, Vec3 a, Vec3 b, float radius) {
    const Vec3 ab = b - a;
    const float lengthSq = ab.lengthSq();
    const float t = lengthSq > 1e-10f
        ? std::clamp((p - a).dot(ab) / lengthSq, 0.0f, 1.0f)
        : 0.0f;
    return (p - (a + ab * t)).length() - radius;
}

float sdSmoothUnion(float a, float b, float k) {
    // G2 (SPEC 6.2 note): negative-inside smooth union requires -k*h*(1-h).
    //
    // Derivation: with h = clamp(1/2 + (b-a)/(2k),0,1) and
    // f = b(1-h) + a*h + c*k*h*(1-h), inside the blend band a-b = k(1-2h)
    // and df/da = h - (1+c)(1-2h)/2, df/db = (1-h) + (1+c)(1-2h)/2.
    // Only c = -1 keeps both partials in [0,1] (the SDF distance bound);
    // c = +1 gives 3h-1 and 2-3h, which leave [0,1] when h < 1/3.
    // At coincident surfaces c = -1 yields f = -k/4 (inside -> the union
    // bulges outward), while c = +1 yields +k/4 (a carve, not a union).
    // AnatomyField.cpp already uses the correct sign; this dormant utility
    // was the only path with the inverted one. See
    // tests/unit/smooth_union_sign_tests.cpp for the dual-implementation test.
    const float h = std::clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
    return b * (1.0f - h) + a * h - k * h * (1.0f - h);
}

float sdSmoothSubtraction(float a, float b, float k) {
    // a minus b: solid only where a is solid and b is not — a smooth MAX of
    // a and -b (b is an SDF, so -b is negative outside b and positive inside).
    // A smooth max is -smin(-a,-b), so its smoothing term has the OPPOSITE
    // sign to the union in this h convention. At coincident surfaces a
    // subtraction must carve (f = +k/4, outside), hence +k*h*(1-h).
    const float nb = -b;
    const float h = std::clamp(0.5f - 0.5f * (nb - a) / k, 0.0f, 1.0f);
    return nb * (1.0f - h) + a * h + k * h * (1.0f - h);
}

MeshData extractSurfaceNets(const FusionVolume& volume, const SdfField& sdf,
                           std::vector<std::array<uint32_t, 4>>* quadIndices,
                           bool discardDegenerateQuads) {
    MeshData mesh;
    // M13: per-axis resolution — a body-wide X and a thin Z are no longer
    // forced onto the same lattice density.
    const int nx = std::max(2, volume.resolution[0]);
    const int ny = std::max(2, volume.resolution[1]);
    const int nz = std::max(2, volume.resolution[2]);
    const int grid[3] = {nx, ny, nz};
    const Vec3 origin = volume.center - volume.size * 0.5f;
    const Vec3 cellSize{volume.size.x / static_cast<float>(nx),
                        volume.size.y / static_cast<float>(ny),
                        volume.size.z / static_cast<float>(nz)};
    const int extent[3] = {nx + 1, ny + 1, nz + 1};

    auto sampleIndex = [&extent](int x, int y, int z) {
        return static_cast<size_t>((x * extent[1] + y) * extent[2] + z);
    };
    const size_t sampleCount = static_cast<size_t>(extent[0]) * extent[1] * extent[2];

    // Corner samples over the (nx+1)x(ny+1)x(nz+1) lattice.
    std::vector<float> samples(sampleCount);
    for (int x = 0; x < extent[0]; ++x) {
        for (int y = 0; y < extent[1]; ++y) {
            for (int z = 0; z < extent[2]; ++z) {
                samples[sampleIndex(x, y, z)] =
                    sdf(origin + hadamard(Vec3{static_cast<float>(x), static_cast<float>(y),
                                               static_cast<float>(z)},
                                          cellSize));
            }
        }
    }
    auto cornerSample = [&](int x, int y, int z) { return samples[sampleIndex(x, y, z)]; };

    // One dual vertex per cell whose 8 corners straddle the isosurface.
    const size_t cellCount = static_cast<size_t>(nx) * ny * nz;
    std::vector<int> cellVertex(cellCount, -1);
    auto cellIndex = [grid](int x, int y, int z) {
        return static_cast<size_t>((x * grid[1] + y) * grid[2] + z);
    };
    std::vector<Vec3> positions;
    const int cornerOffset[8][3] = {
        {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
        {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
    const int edgePair[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7}};

    for (int x = 0; x < nx; ++x) {
        for (int y = 0; y < ny; ++y) {
            for (int z = 0; z < nz; ++z) {
                float corner[8];
                bool anyNeg = false, anyPos = false;
                for (int c = 0; c < 8; ++c) {
                    corner[c] = cornerSample(x + cornerOffset[c][0],
                                             y + cornerOffset[c][1],
                                             z + cornerOffset[c][2]);
                    anyNeg = anyNeg || corner[c] < 0.0f;
                    anyPos = anyPos || corner[c] >= 0.0f;
                }
                if (!(anyNeg && anyPos)) continue;

                // Vertex = average of the zero crossings on the 12 cube edges.
                Vec3 acc{0.0f, 0.0f, 0.0f};
                int crossings = 0;
                for (const auto& edge : edgePair) {
                    const float sa = corner[edge[0]];
                    const float sb = corner[edge[1]];
                    if ((sa < 0.0f) == (sb < 0.0f)) continue;
                    const float denom = sa - sb;
                    const float t = std::fabs(denom) > 1e-10f ? sa / denom : 0.5f;
                    const int ax = x + cornerOffset[edge[0]][0];
                    const int ay = y + cornerOffset[edge[0]][1];
                    const int az = z + cornerOffset[edge[0]][2];
                    const int bx = x + cornerOffset[edge[1]][0];
                    const int by = y + cornerOffset[edge[1]][1];
                    const int bz = z + cornerOffset[edge[1]][2];
                    const Vec3 pa{static_cast<float>(ax), static_cast<float>(ay), static_cast<float>(az)};
                    const Vec3 pb{static_cast<float>(bx), static_cast<float>(by), static_cast<float>(bz)};
                    acc = acc + hadamard(pa + (pb - pa) * t, cellSize);
                    ++crossings;
                }
                if (crossings == 0) continue;
                acc = acc * (1.0f / static_cast<float>(crossings));
                cellVertex[cellIndex(x, y, z)] = static_cast<int>(positions.size());
                positions.push_back(origin + acc);
            }
        }
    }
    if (positions.empty()) return mesh;

    // Shared dual vertices: one mesh vertex per straddling cell.
    auto gradient = [&](Vec3 p) {
        const float e = std::min(std::min(cellSize.x, cellSize.y), cellSize.z) * 0.5f;
        const float gx = sdf(p + Vec3{e, 0, 0}) - sdf(p - Vec3{e, 0, 0});
        const float gy = sdf(p + Vec3{0, e, 0}) - sdf(p - Vec3{0, e, 0});
        const float gz = sdf(p + Vec3{0, 0, e}) - sdf(p - Vec3{0, 0, e});
        return Vec3{gx, gy, gz};
    };
    mesh.vertices.reserve(positions.size());
    for (const Vec3& p : positions) {
        Vertex vertex;
        vertex.position = p;
        vertex.normal = gradient(p).normalized({0.0f, 1.0f, 0.0f});
        mesh.vertices.push_back(vertex);
    }

    // Emit quads on lattice edges that straddle the surface. Quads reference
    // the shared dual vertices, then a BFS pass orients every quad so the
    // whole patch has one consistent outward winding.
    struct QuadRecord { uint32_t v[4]; bool alternate = false; };
    std::vector<QuadRecord> quads;

    auto cellVertexAt = [&](int x, int y, int z) -> int {
        if (x < 0 || y < 0 || z < 0 || x >= nx || y >= ny || z >= nz) return -1;
        return cellVertex[cellIndex(x, y, z)];
    };

    const int axisOffset[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    for (int axis = 0; axis < 3; ++axis) {
        const int u = (axis + 1) % 3;
        const int v = (axis + 2) % 3;
        int pos[3];
        for (pos[0] = 0; pos[0] < nx; ++pos[0]) {
            for (pos[1] = 0; pos[1] < ny; ++pos[1]) {
                for (pos[2] = 0; pos[2] < nz; ++pos[2]) {
                    const float s0 = cornerSample(pos[0], pos[1], pos[2]);
                    const float s1 = cornerSample(pos[0] + axisOffset[axis][0],
                                                  pos[1] + axisOffset[axis][1],
                                                  pos[2] + axisOffset[axis][2]);
                    if ((s0 < 0.0f) == (s1 < 0.0f)) continue;

                    int cellA[3], cellB[3], cellC[3], cellD[3];
                    for (int k = 0; k < 3; ++k) {
                        const int base = pos[k];
                        const int du = axisOffset[u][k];
                        const int dv = axisOffset[v][k];
                        // The four cells sharing this lattice edge all sit at
                        // base minus the two transverse offsets.
                        cellA[k] = base - du - dv;
                        cellB[k] = base - du;
                        cellC[k] = base;
                        cellD[k] = base - dv;
                    }
                    const int ia = cellVertexAt(cellA[0], cellA[1], cellA[2]);
                    const int ib = cellVertexAt(cellB[0], cellB[1], cellB[2]);
                    const int ic = cellVertexAt(cellC[0], cellC[1], cellC[2]);
                    const int id = cellVertexAt(cellD[0], cellD[1], cellD[2]);
                    if (ia < 0 || ib < 0 || ic < 0 || id < 0) continue;
                    if (discardDegenerateQuads &&
                        (ia == ib || ia == ic || ia == id || ib == ic || ib == id || ic == id)) {
                        continue;
                    }
                    if (!discardDegenerateQuads) {
                        quads.push_back({{static_cast<uint32_t>(ia), static_cast<uint32_t>(ib),
                                          static_cast<uint32_t>(ic), static_cast<uint32_t>(id)}});
                        continue;
                    }
                    const Vec3 qa = mesh.vertices[ia].position;
                    const Vec3 qb = mesh.vertices[ib].position;
                    const Vec3 qc = mesh.vertices[ic].position;
                    const Vec3 qd = mesh.vertices[id].position;
                    const float quadScaleSq = std::max({
                        (qb - qa).lengthSq(), (qc - qb).lengthSq(),
                        (qd - qc).lengthSq(), (qa - qd).lengthSq()});
                    const float minimumAreaSq = 5e-16f * quadScaleSq * quadScaleSq;
                    const float ac0 = (qb - qa).cross(qc - qa).lengthSq();
                    const float ac1 = (qd - qa).cross(qc - qa).lengthSq();
                    const float bd0 = (qc - qb).cross(qd - qb).lengthSq();
                    const float bd1 = (qa - qb).cross(qd - qb).lengthSq();
                    const bool validAc = ac0 >= minimumAreaSq && ac1 >= minimumAreaSq;
                    const bool validBd = bd0 >= minimumAreaSq && bd1 >= minimumAreaSq;
                    if (!validAc && !validBd) continue;
                    const bool alternate = validBd &&
                        (!validAc || std::fmin(bd0, bd1) > std::fmin(ac0, ac1));
                    quads.push_back({{static_cast<uint32_t>(ia), static_cast<uint32_t>(ib),
                                      static_cast<uint32_t>(ic), static_cast<uint32_t>(id)}, alternate});
                }
            }
        }
    }
    if (quads.empty()) return mesh;

    // Orient every quad against the SDF gradient (surface normals and the
    // gradient are parallel on smooth regions, so this yields one consistent
    // outward winding per connected patch).
    for (const QuadRecord& quad : quads) {
        const Vec3& a = mesh.vertices[quad.v[0]].position;
        const Vec3& b = mesh.vertices[quad.v[1]].position;
        const Vec3& c = mesh.vertices[quad.v[2]].position;
        const Vec3 faceNormal = (b - a).cross(c - a);
        const Vec3 grad = gradient((a + b + c) * (1.0f / 3.0f));
        if (faceNormal.dot(grad) >= 0.0f) {
            if (quad.alternate) {
                mesh.indices.insert(mesh.indices.end(), {quad.v[0], quad.v[1], quad.v[3],
                                                         quad.v[1], quad.v[2], quad.v[3]});
            } else {
                mesh.indices.insert(mesh.indices.end(), {quad.v[0], quad.v[1], quad.v[2],
                                                         quad.v[0], quad.v[2], quad.v[3]});
            }
        } else {
            if (quad.alternate) {
                mesh.indices.insert(mesh.indices.end(), {quad.v[0], quad.v[3], quad.v[1],
                                                         quad.v[1], quad.v[3], quad.v[2]});
            } else {
                mesh.indices.insert(mesh.indices.end(), {quad.v[0], quad.v[2], quad.v[1],
                                                         quad.v[0], quad.v[3], quad.v[2]});
            }
        }
        if (quadIndices) {
            quadIndices->push_back({quad.v[0], quad.v[1], quad.v[2], quad.v[3]});
        }
    }
    return finalizeFusion(mesh);
}

} // namespace pae::fusion
