#include "ht_topology.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>

namespace pae::ht {

namespace {

struct UnionFind {
    std::vector<int> parent;
    explicit UnionFind(int n) : parent(static_cast<size_t>(n)) {
        for (int i = 0; i < n; ++i) parent[static_cast<size_t>(i)] = i;
    }
    int find(int x) {
        while (parent[static_cast<size_t>(x)] != x) {
            parent[static_cast<size_t>(x)] = parent[static_cast<size_t>(parent[static_cast<size_t>(x)])];
            x = parent[static_cast<size_t>(x)];
        }
        return x;
    }
    void unite(int a, int b) {
        const int ra = find(a), rb = find(b);
        if (ra != rb) parent[static_cast<size_t>(ra)] = rb;
    }
};

double triangleQualityAspect(const D3& a, const D3& b, const D3& c, double& minAngleDeg) {
    const double la = (b - c).length(), lb = (a - c).length(), lc = (a - b).length();
    const double area2 = (b - a).cross(c - a).length();
    minAngleDeg = 180.0;
    if (area2 <= 1e-18) return 1e9;
    const double angles[3] = {
        std::acos(std::max(-1.0, std::min(1.0, ((b - a).normalized()).dot((c - a).normalized())))),
        std::acos(std::max(-1.0, std::min(1.0, ((a - b).normalized()).dot((c - b).normalized())))),
        std::acos(std::max(-1.0, std::min(1.0, ((a - c).normalized()).dot((b - c).normalized()))))};
    for (double a : angles) minAngleDeg = std::min(minAngleDeg, a * 180.0 / 3.14159265358979323846);
    return (la * la + lb * lb + lc * lc) / (2.0 * 1.7320508075688772 * area2);
}

} // namespace

std::map<std::pair<int, int>, int> edgeLookup(const Topology& topo) {
    std::map<std::pair<int, int>, int> lookup;
    for (size_t i = 0; i < topo.edges.size(); ++i)
        lookup[{topo.edges[i][0], topo.edges[i][1]}] = static_cast<int>(i);
    return lookup;
}

int Topology::edgeIndex(int a, int b, const std::map<std::pair<int, int>, int>& lookup) const {
    if (a > b) std::swap(a, b);
    const auto it = lookup.find({a, b});
    return it == lookup.end() ? -1 : it->second;
}

void buildTopology(const PolyMesh& mesh, Topology& topo) {
    topo = Topology{};
    topo.vertexCount = static_cast<int>(mesh.pos.size());
    topo.faceCount = static_cast<int>(mesh.faces.size());
    std::map<std::pair<int, int>, int> index;
    std::vector<std::set<int>> neighbours(mesh.pos.size());
    topo.vertexFaces.assign(mesh.pos.size(), {});
    for (size_t f = 0; f < mesh.faces.size(); ++f) {
        const PolyFace& face = mesh.faces[f];
        const size_t n = face.v.size();
        for (size_t k = 0; k < n; ++k) {
            int a = face.v[k], b = face.v[(k + 1) % n];
            topo.vertexFaces[static_cast<size_t>(a)].push_back(static_cast<int>(f));
            neighbours[static_cast<size_t>(a)].insert(b);
            neighbours[static_cast<size_t>(b)].insert(a);
            if (a > b) std::swap(a, b);
            const auto it = index.find({a, b});
            if (it == index.end()) {
                index[{a, b}] = static_cast<int>(topo.edges.size());
                topo.edges.push_back({a, b, 1});
                topo.edgeFaces.push_back({static_cast<int>(f)});
            } else {
                ++topo.edges[static_cast<size_t>(it->second)][2];
                topo.edgeFaces[static_cast<size_t>(it->second)].push_back(static_cast<int>(f));
            }
        }
    }
    topo.vertexNeighbours.resize(mesh.pos.size());
    for (size_t v = 0; v < mesh.pos.size(); ++v)
        topo.vertexNeighbours[v].assign(neighbours[v].begin(), neighbours[v].end());

    UnionFind uf(topo.vertexCount);
    for (const auto& e : topo.edges) uf.unite(e[0], e[1]);
    std::map<int, int> componentIds;
    topo.componentOfVertex.resize(mesh.pos.size());
    for (int v = 0; v < topo.vertexCount; ++v) {
        const int root = uf.find(v);
        auto it = componentIds.find(root);
        if (it == componentIds.end()) {
            const int id = static_cast<int>(componentIds.size());
            componentIds[root] = id;
            topo.componentOfVertex[static_cast<size_t>(v)] = id;
        } else {
            topo.componentOfVertex[static_cast<size_t>(v)] = it->second;
        }
    }
    topo.componentCount = static_cast<int>(componentIds.size());
    for (const auto& n : topo.vertexNeighbours)
        if (n.empty()) ++topo.isolatedVertexCount;
}

D3 faceNormal(const PolyMesh& mesh, int face) {
    // Newell's method: valid for any planar or mildly non-planar polygon.
    const PolyFace& f = mesh.faces[static_cast<size_t>(face)];
    D3 n;
    for (size_t i = 0; i < f.v.size(); ++i) {
        const D3& a = mesh.pos[static_cast<size_t>(f.v[i])];
        const D3& b = mesh.pos[static_cast<size_t>(f.v[(i + 1) % f.v.size()])];
        n.x += (a.y - b.y) * (a.z + b.z);
        n.y += (a.z - b.z) * (a.x + b.x);
        n.z += (a.x - b.x) * (a.y + b.y);
    }
    return n.normalized();
}

double faceArea(const PolyMesh& mesh, int face) {
    const PolyFace& f = mesh.faces[static_cast<size_t>(face)];
    double sum = 0.0;
    const D3& a = mesh.pos[static_cast<size_t>(f.v[0])];
    for (size_t k = 1; k + 1 < f.v.size(); ++k)
        sum += (mesh.pos[static_cast<size_t>(f.v[k])] - a).cross(mesh.pos[static_cast<size_t>(f.v[k + 1])] - a).length() * 0.5;
    return sum;
}

Stats summarize(std::vector<double> values) {
    Stats s;
    s.count = static_cast<long>(values.size());
    if (values.empty()) return s;
    std::sort(values.begin(), values.end());
    s.min = values.front();
    s.max = values.back();
    double sum = 0.0;
    for (double v : values) sum += v;
    s.mean = sum / static_cast<double>(values.size());
    auto at = [&](double q) {
        const double pos = q * static_cast<double>(values.size() - 1);
        const size_t i = static_cast<size_t>(pos);
        const size_t j = std::min(i + 1, values.size() - 1);
        return values[i] + (values[j] - values[i]) * (pos - static_cast<double>(i));
    };
    s.p01 = at(0.01);
    s.p50 = at(0.50);
    s.p99 = at(0.99);
    return s;
}

TopologyAudit auditTopology(const PolyMesh& mesh, const Topology& topo) {
    TopologyAudit a;
    const double kPi = 3.14159265358979323846;
    a.vertexCount = topo.vertexCount;
    a.faceCount = topo.faceCount;
    a.edgeCount = static_cast<long>(topo.edges.size());
    a.componentCount = topo.componentCount;
    a.isolatedVertexCount = topo.isolatedVertexCount;

    std::vector<double> edgeLengths, cornerAngles, triAngles, aspects, dihedrals;
    std::set<std::vector<int>> faceSignatures;
    std::set<std::pair<int, int>> uvPairs;
    std::set<std::string> materials;

    // Per-face pass.
    std::vector<D3> normals(static_cast<size_t>(topo.faceCount));
    for (size_t f = 0; f < mesh.faces.size(); ++f) {
        const PolyFace& face = mesh.faces[f];
        const size_t n = face.v.size();
        if (n == 3) ++a.triCount;
        else if (n == 4) ++a.quadCount;
        else ++a.ngonCount;
        a.triangulatedCount += static_cast<long>(n - 2);
        a.uvCornerCount += static_cast<long>(n);
        for (size_t k = 0; k < n; ++k) {
            uvPairs.insert({face.v[k], face.vt[k]});
            edgeLengths.push_back((mesh.pos[static_cast<size_t>(face.v[k])] -
                                   mesh.pos[static_cast<size_t>(face.v[(k + 1) % n])]).length());
        }
        if (!mesh.useMtl.empty() && !mesh.useMtl[f].empty()) {
            ++a.materialFaceCount;
            materials.insert(mesh.useMtl[f]);
        }
        std::vector<int> sorted = face.v;
        std::sort(sorted.begin(), sorted.end());
        if (!faceSignatures.insert(sorted).second) ++a.duplicateFaceCount;
        std::vector<int> unique = sorted;
        unique.erase(std::unique(unique.begin(), unique.end()), unique.end());
        if (unique.size() != n) ++a.degenerateFaceCount;
        const double area = faceArea(mesh, static_cast<int>(f));
        if (area < 1e-14) ++a.zeroAreaFaceCount;
        a.surfaceArea += area;
        normals[f] = faceNormal(mesh, static_cast<int>(f));
        // Signed volume through a fan from vertex 0.
        const D3& p0 = mesh.pos[static_cast<size_t>(face.v[0])];
        for (size_t k = 1; k + 1 < n; ++k) {
            const D3& p1 = mesh.pos[static_cast<size_t>(face.v[k])];
            const D3& p2 = mesh.pos[static_cast<size_t>(face.v[k + 1])];
            a.signedVolume += p0.dot(p1.cross(p2)) / 6.0;
        }
        // Corner angles of the polygon.
        for (size_t k = 0; k < n; ++k) {
            const D3& p = mesh.pos[static_cast<size_t>(face.v[k])];
            const D3& prev = mesh.pos[static_cast<size_t>(face.v[(k + n - 1) % n])];
            const D3& next = mesh.pos[static_cast<size_t>(face.v[(k + 1) % n])];
            const double c = std::max(-1.0, std::min(1.0, (prev - p).normalized().dot((next - p).normalized())));
            cornerAngles.push_back(std::acos(c) * 180.0 / kPi);
        }
        // Triangle split: quads along the shorter diagonal.
        if (n >= 3) {
            std::vector<std::array<size_t, 3>> tris;
            if (n == 4) {
                const double d02 = (mesh.pos[static_cast<size_t>(face.v[0])] - mesh.pos[static_cast<size_t>(face.v[2])]).lengthSq();
                const double d13 = (mesh.pos[static_cast<size_t>(face.v[1])] - mesh.pos[static_cast<size_t>(face.v[3])]).lengthSq();
                if (d02 <= d13) tris = {{{0, 1, 2}}, {{0, 2, 3}}};
                else tris = {{{1, 2, 3}}, {{1, 3, 0}}};
            } else {
                for (size_t k = 1; k + 1 < n; ++k) tris.push_back({{0, k, k + 1}});
            }
            for (const auto& t : tris) {
                double minAngle = 180.0;
                const double q = triangleQualityAspect(mesh.pos[static_cast<size_t>(face.v[t[0]])],
                                                       mesh.pos[static_cast<size_t>(face.v[t[1]])],
                                                       mesh.pos[static_cast<size_t>(face.v[t[2]])], minAngle);
                aspects.push_back(q);
                triAngles.push_back(minAngle);
            }
        }
    }
    for (const auto& m : materials) a.materialNames += (a.materialNames.empty() ? "" : ",") + m;
    a.uniqueUvVertexPairCount = static_cast<long>(uvPairs.size());

    // Edge pass.
    UnionFind loopFind(topo.vertexCount);
    for (size_t e = 0; e < topo.edges.size(); ++e) {
        const int uses = topo.edges[e][2];
        if (uses == 1) {
            ++a.boundaryEdgeCount;
            loopFind.unite(topo.edges[e][0], topo.edges[e][1]);
        } else if (uses > 2) {
            ++a.nonManifoldEdgeCount;
        }
        if (uses == 2) {
            const int f0 = topo.edgeFaces[e][0], f1 = topo.edgeFaces[e][1];
            // Direction of traversal in each face.
            auto direction = [&](int fi) {
                const PolyFace& face = mesh.faces[static_cast<size_t>(fi)];
                const size_t n = face.v.size();
                for (size_t k = 0; k < n; ++k) {
                    const int a0 = face.v[k], b0 = face.v[(k + 1) % n];
                    if ((a0 == topo.edges[e][0] && b0 == topo.edges[e][1])) return 1;
                    if ((a0 == topo.edges[e][1] && b0 == topo.edges[e][0])) return -1;
                }
                return 0;
            };
            if (direction(f0) == direction(f1)) ++a.inconsistentWindingEdgeCount;
            const double d = std::max(-1.0, std::min(1.0, normals[static_cast<size_t>(f0)].dot(normals[static_cast<size_t>(f1)])));
            const double deg = std::acos(d) * 180.0 / kPi;
            dihedrals.push_back(deg);
            if (deg > 60.0) ++a.dihedralOver60;
            if (deg > 90.0) ++a.dihedralOver90;
            // UV seam: the two faces reference different vt for a shared vertex.
            auto vtOf = [&](int fi, int vertex) {
                const PolyFace& face = mesh.faces[static_cast<size_t>(fi)];
                for (size_t k = 0; k < face.v.size(); ++k)
                    if (face.v[k] == vertex) return face.vt[k];
                return -2;
            };
            if (vtOf(f0, topo.edges[e][0]) != vtOf(f1, topo.edges[e][0]) ||
                vtOf(f0, topo.edges[e][1]) != vtOf(f1, topo.edges[e][1]))
                ++a.uvSeamEdgeCount;
        }
    }
    std::set<int> loopRoots;
    for (size_t e = 0; e < topo.edges.size(); ++e)
        if (topo.edges[e][2] == 1) loopRoots.insert(loopFind.find(topo.edges[e][0]));
    // Every boundary loop is a cycle of boundary edges, so the number of
    // union-find roots among boundary edges equals the number of loops.
    a.boundaryLoopCount = static_cast<int>(loopRoots.size());

    a.valenceMin = std::numeric_limits<int>::max();
    a.valenceMax = 0;
    for (const auto& n : topo.vertexNeighbours) {
        if (n.empty()) continue;
        a.valenceMin = std::min(a.valenceMin, static_cast<int>(n.size()));
        a.valenceMax = std::max(a.valenceMax, static_cast<int>(n.size()));
        ++a.valenceHistogram[static_cast<int>(n.size())];
    }
    a.edgeLength = summarize(edgeLengths);
    a.minCornerAngleDeg = summarize(cornerAngles);
    a.triangleMinAngleDeg = summarize(triAngles);
    a.aspectRatio = summarize(aspects);
    a.dihedralDeg = summarize(dihedrals);
    a.eulerChi = a.vertexCount - a.edgeCount + a.faceCount;
    a.outwardWinding = a.signedVolume > 0.0 && a.boundaryEdgeCount == 0;
    return a;
}

DeformationQuality compareSurfaces(const PolyMesh& rest, const std::vector<D3>& deformed, const Topology& topo) {
    DeformationQuality q;
    PolyMesh moved = rest;
    moved.pos = deformed;
    const TopologyAudit audit = auditTopology(moved, topo);
    q.minAngleDeg = audit.triangleMinAngleDeg.min;
    q.maxAspect = audit.aspectRatio.max;
    q.signedVolume = audit.signedVolume;
    const double restVolume = auditTopology(rest, topo).signedVolume;
    q.volumeRatio = restVolume != 0.0 ? audit.signedVolume / restVolume : 1.0;
    std::vector<double> ratios, stretch;
    for (size_t f = 0; f < rest.faces.size(); ++f) {
        const double d = faceNormal(rest, static_cast<int>(f)).dot(faceNormal(moved, static_cast<int>(f)));
        q.minNormalDot = std::min(q.minNormalDot, d);
        if (d < 0.0) ++q.flippedFaces;
        const size_t n = rest.faces[f].v.size();
        for (size_t k = 0; k < n; ++k) {
            const int a = rest.faces[f].v[k], b = rest.faces[f].v[(k + 1) % n];
            const double r0 = (rest.pos[static_cast<size_t>(a)] - rest.pos[static_cast<size_t>(b)]).length();
            const double r1 = (deformed[static_cast<size_t>(a)] - deformed[static_cast<size_t>(b)]).length();
            if (r0 <= 1e-15) continue;
            const double ratio = r1 / r0;
            q.minEdgeRatio = std::min(q.minEdgeRatio, ratio);
            q.maxEdgeRatio = std::max(q.maxEdgeRatio, ratio);
            if (ratio < 0.5) ++q.edgesBelow50pct;
            stretch.push_back(std::max(ratio, 1.0 / ratio));
        }
    }
    // 99th percentile of the worst stretch-or-squash factor per edge instance.
    std::sort(stretch.begin(), stretch.end());
    if (!stretch.empty())
        q.edgeRatioP99 = stretch[std::min(stretch.size() - 1, static_cast<size_t>(0.99 * static_cast<double>(stretch.size())))];
    return q;
}

} // namespace pae::ht
