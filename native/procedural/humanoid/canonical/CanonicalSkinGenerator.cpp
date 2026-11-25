#include "CanonicalSkinGenerator.h"

#include "skeleton/Skeleton.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <unordered_map>

namespace pae::canonical {
namespace {

struct EdgeKey {
    uint32_t a;
    uint32_t b;
    bool operator<(const EdgeKey& other) const {
        return a < other.a || (a == other.a && b < other.b);
    }
};

EdgeKey edgeKey(uint32_t a, uint32_t b) {
    return a < b ? EdgeKey{a, b} : EdgeKey{b, a};
}

Vec3 interpolateBoundary(const MeshData& mesh, const BoundaryLoop& loop, float fraction) {
    if (loop.vertexIds.empty()) return {};
    if (loop.vertexIds.size() == 1) return mesh.vertices[loop.vertexIds.front()].position;
    const float target = std::clamp(fraction, 0.0f, 1.0f) * loop.totalLength;
    const auto upper = std::upper_bound(loop.distance.begin(), loop.distance.end(), target);
    size_t segment = static_cast<size_t>(upper - loop.distance.begin());
    if (segment == 0) return mesh.vertices[loop.vertexIds.front()].position;
    if (segment >= loop.vertexIds.size()) segment = 0;
    const size_t previous = segment - 1;
    const float d0 = loop.distance[previous];
    const float d1 = segment == 0 ? loop.totalLength : loop.distance[segment];
    const float span = d1 - d0;
    const float t = span > 1e-8f ? (target - d0) / span : 0.0f;
    return mesh.vertices[loop.vertexIds[previous]].position * (1.0f - t) +
           mesh.vertices[loop.vertexIds[segment]].position * t;
}

void recomputeNormals(MeshData& mesh) {
    for (Vertex& vertex : mesh.vertices) vertex.normal = {};
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        Vertex& a = mesh.vertices[mesh.indices[i]];
        Vertex& b = mesh.vertices[mesh.indices[i + 1]];
        Vertex& c = mesh.vertices[mesh.indices[i + 2]];
        const Vec3 face = (b.position - a.position).cross(c.position - a.position);
        a.normal = a.normal + face;
        b.normal = b.normal + face;
        c.normal = c.normal + face;
    }
    for (Vertex& vertex : mesh.vertices) {
        vertex.normal = vertex.normal.normalized(Vec3{0.0f, 1.0f, 0.0f});
    }
}

struct RenderKey {
    uint32_t canonical;
    uint32_t chart;
    uint8_t corner;
    uint16_t material;
    bool operator<(const RenderKey& other) const {
        if (canonical != other.canonical) return canonical < other.canonical;
        if (chart != other.chart) return chart < other.chart;
        if (corner != other.corner) return corner < other.corner;
        return material < other.material;
    }
};

} // namespace

std::vector<BoundaryLoop> extractBoundaryLoops(const MeshData& mesh) {
    std::map<EdgeKey, uint32_t> edgeCounts;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const uint32_t triangle[3] = {mesh.indices[i], mesh.indices[i + 1], mesh.indices[i + 2]};
        for (uint32_t e = 0; e < 3; ++e) {
            const uint32_t a = triangle[e];
            const uint32_t b = triangle[(e + 1) % 3];
            if (a >= mesh.vertices.size() || b >= mesh.vertices.size() || a == b) continue;
            ++edgeCounts[edgeKey(a, b)];
        }
    }
    std::map<uint32_t, uint32_t> outgoing;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const uint32_t triangle[3] = {mesh.indices[i], mesh.indices[i + 1], mesh.indices[i + 2]};
        for (uint32_t e = 0; e < 3; ++e) {
            const uint32_t a = triangle[e];
            const uint32_t b = triangle[(e + 1) % 3];
            if (a < mesh.vertices.size() && b < mesh.vertices.size() &&
                edgeCounts[edgeKey(a, b)] == 1) {
                outgoing[a] = b;
            }
        }
    }

    std::set<uint32_t> visited;
    std::vector<BoundaryLoop> loops;
    for (const auto& entry : outgoing) {
        const uint32_t start = entry.first;
        if (visited.count(start)) continue;
        BoundaryLoop loop;
        loop.anchor = start;
        loop.direction = 1;
        uint32_t current = start;
        for (size_t step = 0; step <= mesh.vertices.size(); ++step) {
            if (visited.count(current)) break;
            visited.insert(current);
            loop.vertexIds.push_back(current);
            const auto next = outgoing.find(current);
            if (next == outgoing.end() || next->second >= mesh.vertices.size()) break;
            current = next->second;
            if (current == start) break;
        }
        if (loop.vertexIds.size() >= 3) {
            loop.distance.resize(loop.vertexIds.size(), 0.0f);
            for (size_t i = 0; i < loop.vertexIds.size(); ++i) {
                const size_t next = (i + 1) % loop.vertexIds.size();
                loop.totalLength += (mesh.vertices[loop.vertexIds[next]].position -
                                     mesh.vertices[loop.vertexIds[i]].position).length();
                loop.distance[next] = next == 0 ? 0.0f : loop.totalLength;
            }
            loops.push_back(std::move(loop));
        }
    }
    return loops;
}

std::vector<Vec3> resampleBoundary(const MeshData& mesh, const BoundaryLoop& loop,
                                   uint32_t sampleCount) {
    std::vector<Vec3> result;
    if (sampleCount == 0 || loop.vertexIds.empty()) return result;
    result.reserve(sampleCount);
    for (uint32_t i = 0; i < sampleCount; ++i) {
        result.push_back(interpolateBoundary(mesh, loop, static_cast<float>(i) / sampleCount));
    }
    return result;
}

BoundaryLoop orientBoundary(const MeshData& mesh, const BoundaryLoop& loop, bool reverse) {
    BoundaryLoop result = loop;
    if (!reverse) return result;
    result.direction = -loop.direction;
    std::reverse(result.vertexIds.begin(), result.vertexIds.end());
    result.distance.assign(result.vertexIds.size(), 0.0f);
    result.totalLength = 0.0f;
    for (size_t i = 0; i < result.vertexIds.size(); ++i) {
        const size_t next = (i + 1) % result.vertexIds.size();
        result.totalLength += (mesh.vertices[result.vertexIds[next]].position -
                               mesh.vertices[result.vertexIds[i]].position).length();
        result.distance[next] = next == 0 ? 0.0f : result.totalLength;
    }
    return result;
}

std::vector<uint32_t> matchBoundaryVertices(const MeshData& mesh, const BoundaryLoop& reference,
                                            const BoundaryLoop& candidate,
                                            uint32_t commonSamples) {
    std::vector<uint32_t> mapping(candidate.vertexIds.size(), std::numeric_limits<uint32_t>::max());
    if (reference.vertexIds.empty() || candidate.vertexIds.empty()) return mapping;
    if (reference.vertexIds.size() == candidate.vertexIds.size()) {
        const uint32_t count = static_cast<uint32_t>(reference.vertexIds.size());
        float bestError = std::numeric_limits<float>::max();
        uint32_t bestShift = 0;
        bool bestReverse = false;
        for (uint32_t reverse = 0; reverse < 2; ++reverse) {
            for (uint32_t shift = 0; shift < count; ++shift) {
                float error = 0.0f;
                for (uint32_t i = 0; i < count; ++i) {
                    const uint32_t candidateIndex = reverse ? (shift + count - i) % count
                                                            : (i + shift) % count;
                    error += (mesh.vertices[reference.vertexIds[i]].position -
                              mesh.vertices[candidate.vertexIds[candidateIndex]].position).lengthSq();
                }
                if (error < bestError) {
                    bestError = error;
                    bestShift = shift;
                    bestReverse = reverse != 0;
                }
            }
        }
        for (uint32_t i = 0; i < count; ++i) {
            const uint32_t referenceIndex = bestReverse ? (bestShift + count - i) % count
                                                        : (i + count - bestShift) % count;
            mapping[i] = reference.vertexIds[referenceIndex];
        }
        return mapping;
    }
    const uint32_t samples = std::max({commonSamples,
                                       static_cast<uint32_t>(reference.vertexIds.size()),
                                       static_cast<uint32_t>(candidate.vertexIds.size()), 3u});
    const auto referenceSamples = resampleBoundary(mesh, reference, samples);
    const auto candidateSamples = resampleBoundary(mesh, candidate, samples);
    float bestError = std::numeric_limits<float>::max();
    uint32_t bestShift = 0;
    bool bestReverse = false;
    for (uint32_t reverse = 0; reverse < 2; ++reverse) {
        for (uint32_t shift = 0; shift < samples; ++shift) {
            float error = 0.0f;
            for (uint32_t i = 0; i < samples; ++i) {
                const uint32_t candidateIndex = reverse
                    ? (shift + samples - i) % samples : (i + shift) % samples;
                error += (referenceSamples[i] - candidateSamples[candidateIndex]).lengthSq();
            }
            if (error < bestError) {
                bestError = error;
                bestShift = shift;
                bestReverse = reverse != 0;
            }
        }
    }
    for (size_t i = 0; i < candidate.vertexIds.size(); ++i) {
        const float candidateFraction = static_cast<float>(i) / candidate.vertexIds.size();
        const float candidateParameter = candidateFraction * samples;
        const float referenceParameter = bestReverse
            ? bestShift - candidateParameter : candidateParameter - bestShift;
        float referenceFraction = referenceParameter / samples;
        referenceFraction -= std::floor(referenceFraction);
        const Vec3 target = interpolateBoundary(mesh, reference, referenceFraction);
        uint32_t best = 0;
        float bestDistance = std::numeric_limits<float>::max();
        for (size_t j = 0; j < reference.vertexIds.size(); ++j) {
            const float distance = (mesh.vertices[reference.vertexIds[j]].position - target).lengthSq();
            if (distance < bestDistance) {
                bestDistance = distance;
                best = static_cast<uint32_t>(j);
            }
        }
        mapping[i] = reference.vertexIds[best];
    }
    return mapping;
}

WeldResult weldBoundaryLoops(MeshData& mesh, const BoundaryLoop& reference,
                             const BoundaryLoop& candidate) {
    WeldResult result;
    if (reference.vertexIds.size() != candidate.vertexIds.size() ||
        reference.vertexIds.size() < 3) return result;
    const std::vector<uint32_t> mapping = matchBoundaryVertices(mesh, reference, candidate, 0);
    std::vector<uint32_t> remap(mesh.vertices.size());
    std::iota(remap.begin(), remap.end(), 0u);
    std::vector<char> candidateWeld(mesh.vertices.size(), 0);
    for (size_t i = 0; i < candidate.vertexIds.size(); ++i) {
        const uint32_t source = candidate.vertexIds[i];
        const uint32_t destination = mapping[i];
        if (source == destination || destination == std::numeric_limits<uint32_t>::max()) continue;
        remap[source] = destination;
        candidateWeld[source] = 1;
        ++result.weldedPairs;
        result.maxPositionError = std::fmax(result.maxPositionError,
            (mesh.vertices[source].position - mesh.vertices[destination].position).length());
    }
    for (size_t i = 0; i < remap.size(); ++i) {
        if (candidateWeld[i] && remap[i] < remap.size()) {
            mergeVertexWeights(mesh.vertices[remap[i]], mesh.vertices[i]);
        }
    }
    std::vector<uint32_t> indices;
    indices.reserve(mesh.indices.size());
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const uint32_t a = remap[mesh.indices[i]];
        const uint32_t b = remap[mesh.indices[i + 1]];
        const uint32_t c = remap[mesh.indices[i + 2]];
        if (a == b || b == c || c == a) continue;
        indices.insert(indices.end(), {a, b, c});
    }
    mesh.indices = std::move(indices);
    std::vector<uint32_t> compact(mesh.vertices.size(), std::numeric_limits<uint32_t>::max());
    std::vector<Vertex> vertices;
    vertices.reserve(mesh.vertices.size());
    for (uint32_t index : mesh.indices) {
        if (compact[index] != std::numeric_limits<uint32_t>::max()) continue;
        compact[index] = static_cast<uint32_t>(vertices.size());
        vertices.push_back(mesh.vertices[index]);
    }
    for (uint32_t& index : mesh.indices) index = compact[index];
    mesh.vertices = std::move(vertices);
    recomputeNormals(mesh);
    result.valid = result.weldedPairs == candidate.vertexIds.size() && result.maxPositionError < 1e-3f;
    return result;
}

void mergeVertexWeights(Vertex& destination, const Vertex& source) {
    std::array<float, BoneCount> weights{};
    const uint16_t destinationBones[4] = {destination.joints.x, destination.joints.y,
                                          destination.joints.z, destination.joints.w};
    const uint16_t sourceBones[4] = {source.joints.x, source.joints.y,
                                     source.joints.z, source.joints.w};
    const float destinationValues[4] = {destination.weights.x, destination.weights.y,
                                        destination.weights.z, destination.weights.w};
    const float sourceValues[4] = {source.weights.x, source.weights.y,
                                   source.weights.z, source.weights.w};
    for (int slot = 0; slot < 4; ++slot) {
        if (destinationBones[slot] < BoneCount && std::isfinite(destinationValues[slot]) &&
            destinationValues[slot] > 1e-7f) {
            weights[destinationBones[slot]] += destinationValues[slot] * 0.5f;
        }
        if (sourceBones[slot] < BoneCount && std::isfinite(sourceValues[slot]) &&
            sourceValues[slot] > 1e-7f) {
            weights[sourceBones[slot]] += sourceValues[slot] * 0.5f;
        }
    }
    std::array<uint16_t, 4> bones{};
    std::array<float, 4> values{};
    for (uint16_t bone = 0; bone < BoneCount; ++bone) {
        const float value = weights[bone];
        for (size_t slot = 0; slot < values.size(); ++slot) {
            if (value > values[slot] ||
                (value == values[slot] && value > 0.0f && bone < bones[slot])) {
                for (size_t shift = values.size() - 1; shift > slot; --shift) {
                    values[shift] = values[shift - 1];
                    bones[shift] = bones[shift - 1];
                }
                values[slot] = value;
                bones[slot] = bone;
                break;
            }
        }
    }
    float sum = values[0] + values[1] + values[2] + values[3];
    if (!(sum > 1e-8f)) {
        destination.joints = {0, 0, 0, 0};
        destination.weights = {1.0f, 0.0f, 0.0f, 0.0f};
        return;
    }
    destination.joints = {bones[0], bones[1], bones[2], bones[3]};
    destination.weights = {values[0] / sum, values[1] / sum,
                           values[2] / sum, values[3] / sum};
}

WeightReport validateWeights(const MeshData& mesh) {
    WeightReport report;
    report.vertices = mesh.vertices.size();
    for (const Vertex& vertex : mesh.vertices) {
        const uint16_t bones[4] = {vertex.joints.x, vertex.joints.y,
                                   vertex.joints.z, vertex.joints.w};
        const float values[4] = {vertex.weights.x, vertex.weights.y,
                                 vertex.weights.z, vertex.weights.w};
        bool valid = true;
        float sum = 0.0f;
        for (int slot = 0; slot < 4; ++slot) {
            valid = valid && bones[slot] < BoneCount && std::isfinite(values[slot]) && values[slot] >= 0.0f;
            sum += values[slot];
        }
        const float error = std::fabs(sum - 1.0f);
        report.maxNormalizationError = std::fmax(report.maxNormalizationError, error);
        if (!valid || error > 1e-3f) ++report.invalidVertices;
    }
    return report;
}

RenderResult buildRenderMesh(const CanonicalSkinResult& canonical, float) {
    RenderResult result;
    const MeshData& source = canonical.mesh;
    const size_t triangleCount = source.indices.size() / 3;
    if (triangleCount == 0 || source.vertices.empty()) return result;

    result.uv.charts = triangleCount;
    result.uv.atlasColumns = static_cast<size_t>(std::ceil(std::sqrt(static_cast<double>(triangleCount))));
    result.uv.atlasRows = (triangleCount + result.uv.atlasColumns - 1) / result.uv.atlasColumns;
    const float cellWidth = 1.0f / static_cast<float>(result.uv.atlasColumns);
    const float cellHeight = 1.0f / static_cast<float>(result.uv.atlasRows);
    constexpr float inset = 0.08f;

    result.mesh.vertices.reserve(source.indices.size());
    result.mesh.indices.reserve(source.indices.size());
    result.renderToCanonical.reserve(source.indices.size());
    result.uv.minUvArea = std::numeric_limits<float>::infinity();
    for (size_t triangle = 0; triangle < triangleCount; ++triangle) {
        const size_t column = triangle % result.uv.atlasColumns;
        const size_t row = triangle / result.uv.atlasColumns;
        const float x = static_cast<float>(column) * cellWidth;
        const float y = static_cast<float>(row) * cellHeight;
        const std::array<Vec2, 3> uv{{
            {(x + inset * cellWidth), (y + inset * cellHeight)},
            {(x + (1.0f - inset) * cellWidth), (y + inset * cellHeight)},
            {(x + inset * cellWidth), (y + (1.0f - inset) * cellHeight)},
        }};
        for (size_t corner = 0; corner < 3; ++corner) {
            const uint32_t sourceIndex = source.indices[triangle * 3 + corner];
            if (sourceIndex >= source.vertices.size()) return RenderResult{};
            Vertex vertex = source.vertices[sourceIndex];
            vertex.uv = uv[corner];
            if (!std::isfinite(vertex.uv.x) || !std::isfinite(vertex.uv.y)) {
                ++result.uv.invalidUvVertices;
            } else if (vertex.uv.x < 0.0f || vertex.uv.x > 1.0f ||
                       vertex.uv.y < 0.0f || vertex.uv.y > 1.0f) {
                ++result.uv.outOfRangeUvVertices;
            }
            result.mesh.indices.push_back(static_cast<uint32_t>(result.mesh.vertices.size()));
            result.mesh.vertices.push_back(vertex);
            result.renderToCanonical.push_back(sourceIndex);
        }
        const float area = std::fabs((uv[1].x - uv[0].x) * (uv[2].y - uv[0].y) -
                                     (uv[1].y - uv[0].y) * (uv[2].x - uv[0].x)) * 0.5f;
        if (area <= 1e-12f) ++result.uv.zeroAreaUvTriangles;
        result.uv.minUvArea = std::fmin(result.uv.minUvArea, area);
        result.uv.maxUvArea = std::fmax(result.uv.maxUvArea, area);
    }
    Aabb bounds;
    for (const Vertex& vertex : result.mesh.vertices) bounds.expand(vertex.position);
    result.mesh.bounds = bounds;
    result.mesh.subMeshes = source.subMeshes;
    result.uv.renderVertices = result.mesh.vertices.size();
    result.splitVertices = result.mesh.vertices.size() - source.vertices.size();
    result.valid = result.renderToCanonical.size() == result.mesh.vertices.size() &&
                   result.uv.invalidUvVertices == 0 &&
                   result.uv.outOfRangeUvVertices == 0 &&
                   result.uv.zeroAreaUvTriangles == 0 &&
                   validateWeights(result.mesh).invalidVertices == 0;
    return result;
}

} // namespace pae::canonical
