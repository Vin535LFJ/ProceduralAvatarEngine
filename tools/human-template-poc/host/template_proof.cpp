#include "core/math/Math.h"
#include "procedural/MeshData.h"
#include "procedural/MeshTopology.h"
#include "procedural/humanoid/canonical/CanonicalSkinGenerator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
using namespace pae;

namespace {

struct Json {
    enum class Kind { Null, Number, String, Array, Object } kind = Kind::Null;
    double number = 0.0;
    std::string string;
    std::vector<Json> array;
    std::map<std::string, Json> object;
    const Json& at(const std::string& key) const { return object.at(key); }
    double num() const { return number; }
    const std::string& str() const { return string; }
};

class JsonParser {
public:
    explicit JsonParser(std::string text) : text_(std::move(text)) {}
    Json parse() {
        skip();
        return value();
    }
private:
    std::string text_;
    size_t pos_ = 0;
    void skip() {
        while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) ++pos_;
    }
    bool take(char c) {
        skip();
        if (pos_ < text_.size() && text_[pos_] == c) { ++pos_; return true; }
        return false;
    }
    Json value() {
        skip();
        if (pos_ >= text_.size()) throw std::runtime_error("unexpected JSON end");
        const char c = text_[pos_];
        if (c == '{') return object();
        if (c == '[') return array();
        if (c == '"') return string();
        if (c == 't' && text_.compare(pos_, 4, "true") == 0) { pos_ += 4; return {}; }
        if (c == 'f' && text_.compare(pos_, 5, "false") == 0) { pos_ += 5; return {}; }
        if (c == 'n' && text_.compare(pos_, 4, "null") == 0) { pos_ += 4; return {}; }
        char* end = nullptr;
        const double n = std::strtod(text_.c_str() + pos_, &end);
        if (end == text_.c_str() + pos_) throw std::runtime_error("bad JSON number");
        pos_ = static_cast<size_t>(end - text_.c_str());
        Json result;
        result.kind = Json::Kind::Number;
        result.number = n;
        return result;
    }
    Json object() {
        Json result;
        result.kind = Json::Kind::Object;
        ++pos_;
        if (take('}')) return result;
        do {
            Json key = string();
            if (!take(':')) throw std::runtime_error("missing JSON colon");
            result.object[key.string] = value();
        } while (take(','));
        if (!take('}')) throw std::runtime_error("missing JSON object end");
        return result;
    }
    Json array() {
        Json result;
        result.kind = Json::Kind::Array;
        ++pos_;
        if (take(']')) return result;
        do {
            result.array.push_back(value());
        } while (take(','));
        if (!take(']')) throw std::runtime_error("missing JSON array end");
        return result;
    }
    Json string() {
        skip();
        if (pos_ >= text_.size() || text_[pos_] != '"') throw std::runtime_error("bad JSON string");
        ++pos_;
        Json result;
        result.kind = Json::Kind::String;
        while (pos_ < text_.size()) {
            const char c = text_[pos_++];
            if (c == '"') return result;
            if (c == '\\') {
                if (pos_ >= text_.size()) throw std::runtime_error("bad JSON escape");
                const char e = text_[pos_++];
                if (e == 'n') result.string.push_back('\n');
                else if (e == 't') result.string.push_back('\t');
                else if (e == 'r') result.string.push_back('\r');
                else result.string.push_back(e);
            } else {
                result.string.push_back(c);
            }
        }
        throw std::runtime_error("unterminated JSON string");
    }
};

Json loadJson(const fs::path& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open " + path.string());
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return JsonParser(buffer.str()).parse();
}

uint64_t stableVertexId(uint32_t originalIndex) {
    uint64_t hash = 1469598103934665603ull;
    hash ^= 0x50414548ull;
    hash *= 1099511628211ull;
    hash ^= originalIndex;
    hash *= 1099511628211ull;
    return hash;
}

void writeString(std::ostream& out, const std::string& value) {
    const uint32_t size = static_cast<uint32_t>(value.size());
    out.write(reinterpret_cast<const char*>(&size), sizeof(size));
    out.write(value.data(), size);
}

struct Quad {
    std::array<uint32_t, 4> v{};
    std::array<uint32_t, 4> vt{};
    std::string group;
};

struct ObjMesh {
    std::vector<Vec3> vertices;
    std::vector<Vec2> uvs;
    std::vector<Quad> quads;
    std::map<std::string, size_t> groupFaceCounts;
};

int32_t objIndex(const std::string& token, size_t count) {
    const int64_t raw = std::stoll(token);
    if (raw == 0) throw std::runtime_error("OBJ index zero");
    const int64_t resolved = raw > 0 ? raw - 1 : static_cast<int64_t>(count) + raw;
    if (resolved < 0 || resolved >= static_cast<int64_t>(count)) throw std::runtime_error("OBJ index out of range");
    return static_cast<int32_t>(resolved);
}

ObjMesh loadObj(const fs::path& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open " + path.string());
    ObjMesh mesh;
    std::string line;
    std::string group;
    while (std::getline(in, line)) {
        std::istringstream stream(line);
        std::string kind;
        if (!(stream >> kind)) continue;
        if (kind == "v") {
            Vec3 p;
            stream >> p.x >> p.y >> p.z;
            mesh.vertices.push_back(p * 0.1f);
        } else if (kind == "vt") {
            Vec2 uv;
            stream >> uv.x >> uv.y;
            mesh.uvs.push_back(uv);
        } else if (kind == "g") {
            stream >> group;
        } else if (kind == "f") {
            Quad q;
            q.vt.fill(std::numeric_limits<uint32_t>::max());
            q.group = group;
            std::string token;
            size_t corner = 0;
            while (stream >> token && corner < 4) {
                const size_t slash = token.find('/');
                const std::string v = token.substr(0, slash);
                q.v[corner] = static_cast<uint32_t>(objIndex(v, mesh.vertices.size()));
                if (slash != std::string::npos) {
                    const size_t second = token.find('/', slash + 1);
                    const std::string vt = token.substr(slash + 1, second == std::string::npos
                        ? std::string::npos : second - slash - 1);
                    if (!vt.empty()) q.vt[corner] = static_cast<uint32_t>(objIndex(vt, mesh.uvs.size()));
                }
                ++corner;
            }
            if (corner != 4) throw std::runtime_error("proof currently expects quad-only base mesh");
            std::string extra;
            if (stream >> extra) throw std::runtime_error("base mesh contains a non-quad polygon");
            mesh.quads.push_back(q);
            ++mesh.groupFaceCounts[group];
        }
    }
    return mesh;
}

struct Delta {
    uint32_t original = 0;
    uint32_t body = 0;
    Vec3 value;
};

struct ShapeTarget {
    std::string name;
    std::vector<Delta> deltas;
};

ShapeTarget loadTarget(const fs::path& path, const std::vector<int32_t>& originalToBody) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open " + path.string());
    ShapeTarget target;
    target.name = path.filename().string();
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream stream(line);
        uint32_t original;
        Vec3 delta;
        if (!(stream >> original >> delta.x >> delta.y >> delta.z)) continue;
        if (original >= originalToBody.size() || originalToBody[original] < 0) continue;
        target.deltas.push_back({original, static_cast<uint32_t>(originalToBody[original]), delta * 0.1f});
    }
    return target;
}

void recomputeNormals(MeshData& mesh) {
    for (Vertex& vertex : mesh.vertices) vertex.normal = {};
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        Vertex& a = mesh.vertices[mesh.indices[i]];
        Vertex& b = mesh.vertices[mesh.indices[i + 1]];
        Vertex& c = mesh.vertices[mesh.indices[i + 2]];
        const Vec3 n = (b.position - a.position).cross(c.position - a.position);
        a.normal = a.normal + n;
        b.normal = b.normal + n;
        c.normal = c.normal + n;
    }
    for (Vertex& vertex : mesh.vertices) vertex.normal = vertex.normal.normalized(Vec3{0, 1, 0});
}

struct Quality {
    TopologyReport topology;
    size_t sameDirectionEdges = 0;
    size_t duplicateFaces = 0;
    size_t slivers = 0;
    size_t edges = 0;
    uint32_t minValence = 0;
    uint32_t maxValence = 0;
    float minEdgeLength = 1e9f;
    float maxEdgeLength = 0.0f;
    float signedVolume = 0.0f;
    float minAngleDeg = 180.0f;
    float maxAspect = 1.0f;
    size_t selfIntersections = 0;
};

Quality measureQuality(const MeshData& mesh, bool intersections) {
    Quality q;
    q.topology = analyzeTopology(mesh);
    std::map<std::pair<uint32_t, uint32_t>, std::pair<uint32_t, uint32_t>> edges;
    std::set<std::array<uint32_t, 3>> faces;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const uint32_t ids[3] = {mesh.indices[i], mesh.indices[i + 1], mesh.indices[i + 2]};
        std::array<uint32_t, 3> sorted = {ids[0], ids[1], ids[2]};
        std::sort(sorted.begin(), sorted.end());
        if (!faces.insert(sorted).second) ++q.duplicateFaces;
        const float lengths[3] = {
            (mesh.vertices[ids[1]].position - mesh.vertices[ids[0]].position).length(),
            (mesh.vertices[ids[2]].position - mesh.vertices[ids[1]].position).length(),
            (mesh.vertices[ids[0]].position - mesh.vertices[ids[2]].position).length()
        };
        const float mn = std::min({lengths[0], lengths[1], lengths[2]});
        const float mx = std::max({lengths[0], lengths[1], lengths[2]});
        q.minEdgeLength = std::min(q.minEdgeLength, mn);
        q.maxEdgeLength = std::max(q.maxEdgeLength, mx);
        q.maxAspect = std::max(q.maxAspect, mx / std::max(mn, 1e-9f));
        if (mn <= 1e-7f || mx / mn > 10.0f) ++q.slivers;
        q.signedVolume += mesh.vertices[ids[0]].position.dot(
            mesh.vertices[ids[1]].position.cross(mesh.vertices[ids[2]].position)) / 6.0f;
        for (size_t corner = 0; corner < 3; ++corner) {
            const Vec3 a = mesh.vertices[ids[corner]].position;
            const Vec3 b = mesh.vertices[ids[(corner + 1) % 3]].position;
            const Vec3 c = mesh.vertices[ids[(corner + 2) % 3]].position;
            const float angle = std::acos(std::clamp((b - a).normalized().dot((c - a).normalized()), -1.0f, 1.0f));
            q.minAngleDeg = std::min(q.minAngleDeg, angle * 57.2957795f);
            const uint32_t lo = std::min(ids[corner], ids[(corner + 1) % 3]);
            const uint32_t hi = std::max(ids[corner], ids[(corner + 1) % 3]);
            auto& count = edges[{lo, hi}];
            if (ids[corner] < ids[(corner + 1) % 3]) ++count.first;
            else ++count.second;
        }
    }
    q.edges = edges.size();
    q.minValence = std::numeric_limits<uint32_t>::max();
    for (const auto& entry : edges) {
        const uint32_t incidence = entry.second.first + entry.second.second;
        q.minValence = std::min(q.minValence, incidence);
        q.maxValence = std::max(q.maxValence, incidence);
        if (entry.second.first == 2 || entry.second.second == 2) ++q.sameDirectionEdges;
    }
    if (intersections) q.selfIntersections = canonical::detectSelfIntersections(mesh).intersectingPairs;
    return q;
}

struct Bone {
    std::string name;
    std::string parent;
    std::string headJoint;
    std::string tailJoint;
    int parentIndex = -1;
    Vec3 head;
    Vec3 tail;
    Mat4 restGlobal;
    Mat4 actionGlobal;
    Mat4 palette;
};

struct Rig {
    std::vector<Bone> bones;
    std::unordered_map<std::string, int> boneByName;
};

Rig loadRig(const fs::path& path, const ObjMesh& obj) {
    const Json root = loadJson(path);
    Rig rig;
    const auto& joints = root.at("joints").object;
    std::unordered_map<std::string, Vec3> jointPositions;
    for (const auto& item : joints) {
        Vec3 sum{};
        const auto& ids = item.second.array;
        for (const Json& id : ids) {
            const size_t index = static_cast<size_t>(id.num());
            if (index >= obj.vertices.size()) throw std::runtime_error("rig joint index out of range");
            sum = sum + obj.vertices[index];
        }
        if (!ids.empty()) jointPositions[item.first] = sum * (1.0f / static_cast<float>(ids.size()));
    }
    for (const auto& item : root.at("bones").object) {
        Bone bone;
        bone.name = item.first;
        bone.parent = item.second.at("parent").str();
        bone.headJoint = item.second.at("head").str();
        bone.tailJoint = item.second.at("tail").str();
        bone.head = jointPositions.at(bone.headJoint);
        bone.tail = jointPositions.at(bone.tailJoint);
        rig.boneByName[bone.name] = static_cast<int>(rig.bones.size());
        rig.bones.push_back(std::move(bone));
    }
    for (Bone& bone : rig.bones) {
        if (!bone.parent.empty()) bone.parentIndex = rig.boneByName.at(bone.parent);
    }
    std::vector<char> visited(rig.bones.size(), 0);
    std::function<void(int)> rest = [&](int index) {
        if (visited[index]) return;
        visited[index] = 1;
        Bone& bone = rig.bones[index];
        const Vec3 parentHead = bone.parentIndex >= 0 ? rig.bones[bone.parentIndex].head : Vec3{};
        if (bone.parentIndex >= 0) rest(bone.parentIndex);
        bone.restGlobal = bone.parentIndex >= 0
            ? rig.bones[bone.parentIndex].restGlobal * Mat4::translation(bone.head - parentHead)
            : Mat4::translation(bone.head);
    };
    for (size_t i = 0; i < rig.bones.size(); ++i) rest(static_cast<int>(i));
    return rig;
}

Quat axisAngle(const Vec3& axis, float degrees) {
    const float half = degrees * 0.00872664625997f;
    const Vec3 n = axis.normalized();
    const float s = std::sin(half);
    return Quat{n.x * s, n.y * s, n.z * s, std::cos(half)};
}

void prepareAction(Rig& rig, const std::unordered_map<int, Quat>& rotations) {
    std::vector<char> visited(rig.bones.size(), 0);
    std::function<void(int)> action = [&](int index) {
        if (visited[index]) return;
        visited[index] = 1;
        Bone& bone = rig.bones[index];
        if (bone.parentIndex >= 0) action(bone.parentIndex);
        const Vec3 parentHead = bone.parentIndex >= 0 ? rig.bones[bone.parentIndex].head : Vec3{};
        const auto found = rotations.find(index);
        const Quat rotation = found == rotations.end() ? Quat{} : found->second;
        const Mat4 local = Mat4::translation(bone.head - parentHead) * rotation.toMat4();
        bone.actionGlobal = bone.parentIndex >= 0
            ? rig.bones[bone.parentIndex].actionGlobal * local
            : local;
        bone.palette = bone.actionGlobal * Mat4::translation(bone.head * -1.0f);
    };
    for (size_t i = 0; i < rig.bones.size(); ++i) action(static_cast<int>(i));
}

struct Influence {
    uint16_t bone = 0;
    float weight = 0.0f;
};

std::vector<std::array<Influence, 4>> loadWeights(const fs::path& path, const Rig& rig,
                                                  const std::vector<int32_t>& originalToBody,
                                                  size_t bodyVertexCount) {
    const Json root = loadJson(path);
    std::vector<std::map<int, float>> accumulated(bodyVertexCount);
    for (const auto& group : root.at("weights").object) {
        const auto bone = rig.boneByName.find(group.first);
        if (bone == rig.boneByName.end()) continue;
        for (const Json& pair : group.second.array) {
            const size_t original = static_cast<size_t>(pair.array[0].num());
            const float weight = static_cast<float>(pair.array[1].num());
            if (original >= originalToBody.size() || originalToBody[original] < 0) continue;
            accumulated[static_cast<size_t>(originalToBody[original])][bone->second] += weight;
        }
    }
    std::vector<std::array<Influence, 4>> result(bodyVertexCount);
    size_t invalid = 0;
    size_t maxSourceInfluences = 0;
    float maxError = 0.0f;
    double maxCompressionL1 = 0.0;
    double meanCompressionL1 = 0.0;
    for (size_t vertex = 0; vertex < bodyVertexCount; ++vertex) {
        std::vector<Influence> influences;
        float sum = 0.0f;
        for (const auto& item : accumulated[vertex]) {
            if (item.second <= 0.0f) continue;
            influences.push_back({static_cast<uint16_t>(item.first), item.second});
            sum += item.second;
        }
        if (!(sum > 1e-8f)) {
            result[vertex][0] = {0, 1.0f};
            ++invalid;
            continue;
        }
        std::sort(influences.begin(), influences.end(), [](const Influence& a, const Influence& b) {
            return a.weight > b.weight;
        });
        maxSourceInfluences = std::max(maxSourceInfluences, influences.size());
        if (influences.size() > 4) influences.resize(4);
        float topSum = 0.0f;
        for (const Influence& influence : influences) topSum += influence.weight;
        for (size_t i = 0; i < influences.size(); ++i) {
            result[vertex][i] = {influences[i].bone, influences[i].weight / topSum};
        }
        float normalized = 0.0f;
        for (const Influence& influence : result[vertex]) normalized += influence.weight;
        maxError = std::max(maxError, std::fabs(normalized - 1.0f));
        std::map<int, float> compressed;
        for (const Influence& influence : result[vertex]) {
            if (influence.weight > 0.0f) compressed[influence.bone] = influence.weight;
        }
        double compressionL1 = 0.0;
        for (const auto& item : accumulated[vertex]) {
            if (item.second <= 0.0f) continue;
            const float fullWeight = item.second / sum;
            const auto found = compressed.find(item.first);
            compressionL1 += std::fabs(fullWeight - (found == compressed.end() ? 0.0f : found->second));
        }
        maxCompressionL1 = std::max(maxCompressionL1, compressionL1);
        meanCompressionL1 += compressionL1;
    }
    meanCompressionL1 /= static_cast<double>(bodyVertexCount - invalid);
    std::cout << "WEIGHTS invalid=" << invalid << " max_normalization_error=" << maxError
              << " max_top4_l1=" << maxCompressionL1
              << " mean_top4_l1=" << meanCompressionL1
              << " max_source_influences=" << maxSourceInfluences << "\n";
    return result;
}

std::vector<Vec3> skin(const std::vector<Vec3>& base, const Rig& rig,
                       const std::vector<std::array<Influence, 4>>& weights) {
    std::vector<Vec3> result(base.size());
    for (size_t i = 0; i < base.size(); ++i) {
        Vec3 sum{};
        for (const Influence& influence : weights[i]) {
            const Vec4 q = rig.bones[influence.bone].palette.transform(Vec4{base[i], 1.0f});
            sum = sum + Vec3{q.x, q.y, q.z} * influence.weight;
        }
        result[i] = sum;
    }
    return result;
}

Vec3 posedPoint(const Bone& bone, const Vec3& restPoint) {
    const Vec4 q = bone.palette.transform(Vec4{restPoint, 1.0f});
    return Vec3{q.x, q.y, q.z};
}

// True when `bone` is `ancestor` itself or one of its descendants.
bool isInSubtree(const Rig& rig, int bone, int ancestor) {
    for (int guard = 0; bone >= 0 && guard < static_cast<int>(rig.bones.size()); ++guard) {
        if (bone == ancestor) return true;
        bone = rig.bones[static_cast<size_t>(bone)].parentIndex;
    }
    return false;
}

void exportObj(const fs::path& path, const ObjMesh& obj, const std::vector<Quad>& bodyQuads,
               const std::vector<Vec3>& positions) {
    std::ofstream out(path);
    out << "# PAE Human Template Proof - CC0 source asset converted by PAE host tool\n";
    for (const Vec3& p : positions) out << "v " << p.x << " " << p.y << " " << p.z << "\n";
    for (const Vec2& uv : obj.uvs) out << "vt " << uv.x << " " << uv.y << "\n";
    out << "g body\n";
    for (const Quad& q : bodyQuads) {
        out << "f";
        for (size_t i = 0; i < 4; ++i) {
            out << " " << (q.v[i] + 1);
            if (q.vt[i] != std::numeric_limits<uint32_t>::max()) out << "/" << (q.vt[i] + 1);
        }
        out << "\n";
    }
}

void writeTemplate(const fs::path& path, const MeshData& mesh, const Rig& rig,
                   const std::vector<std::array<Influence, 4>>& weights,
                   const std::vector<ShapeTarget>& targets,
                   const std::vector<uint64_t>& stableIds) {
    std::ofstream out(path, std::ios::binary);
    out.write("PAEHUM01", 8);
    const uint32_t version = 1;
    const uint32_t vertices = static_cast<uint32_t>(mesh.vertices.size());
    const uint32_t triangles = static_cast<uint32_t>(mesh.indices.size() / 3);
    const uint32_t bones = static_cast<uint32_t>(rig.bones.size());
    const uint32_t targetCount = static_cast<uint32_t>(targets.size());
    out.write(reinterpret_cast<const char*>(&version), sizeof(version));
    out.write(reinterpret_cast<const char*>(&vertices), sizeof(vertices));
    out.write(reinterpret_cast<const char*>(&triangles), sizeof(triangles));
    out.write(reinterpret_cast<const char*>(&bones), sizeof(bones));
    out.write(reinterpret_cast<const char*>(&targetCount), sizeof(targetCount));
    for (uint64_t id : stableIds) out.write(reinterpret_cast<const char*>(&id), sizeof(id));
    writeString(out, "a8bc2d54ff0ac92e78ff71431b1023eda42bf482");
    writeString(out, "8e761e6624b8f54536409135d1636da63b32486a90d4897f84e121d144f6fb4c");
    writeString(out, "99f179bce0aa850b45d4191a1d0d234c5851f881c057439470ded3bddf729a24");
    writeString(out, "0f3641d651ae3d00ad6b4ccee43142edb109d3bd909d27d9e4139ef1beed8625");
    writeString(out, "base.obj + targets: CC0-1.0 (LICENSE.md section C, per-file CC0 header). "
                     "default.mhskel + default_weights.mhw: CC0 declared inside the files only, "
                     "not enumerated in LICENSE.md, PENDING owner confirmation. AGPL code not copied");
    for (const Vertex& vertex : mesh.vertices) {
        out.write(reinterpret_cast<const char*>(&vertex.position), sizeof(Vec3));
    }
    out.write(reinterpret_cast<const char*>(mesh.indices.data()),
              static_cast<std::streamsize>(mesh.indices.size() * sizeof(uint32_t)));
    for (const Bone& bone : rig.bones) {
        const uint32_t nameLength = static_cast<uint32_t>(bone.name.size());
        const int32_t parent = bone.parentIndex;
        out.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
        out.write(bone.name.data(), nameLength);
        out.write(reinterpret_cast<const char*>(&parent), sizeof(parent));
        out.write(reinterpret_cast<const char*>(&bone.head), sizeof(Vec3));
        out.write(reinterpret_cast<const char*>(&bone.tail), sizeof(Vec3));
    }
    for (const auto& vertex : weights) {
        for (const Influence& influence : vertex) {
            out.write(reinterpret_cast<const char*>(&influence), sizeof(Influence));
        }
    }
    for (const ShapeTarget& target : targets) {
        const uint32_t nameLength = static_cast<uint32_t>(target.name.size());
        const uint32_t deltaCount = static_cast<uint32_t>(target.deltas.size());
        out.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
        out.write(target.name.data(), nameLength);
        out.write(reinterpret_cast<const char*>(&deltaCount), sizeof(deltaCount));
        for (const Delta& delta : target.deltas) {
            out.write(reinterpret_cast<const char*>(&delta.body), sizeof(delta.body));
            out.write(reinterpret_cast<const char*>(&delta.value), sizeof(Vec3));
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 3) {
            std::cerr << "usage: template_proof <makehuman-data-root> <output-dir>\n";
            return 2;
        }
        const fs::path data = argv[1];
        const fs::path out = argv[2];
        fs::create_directories(out);
        const fs::path objPath = data / "makehuman/data/3dobjs/base.obj";
        const ObjMesh obj = loadObj(objPath);
        std::vector<int32_t> originalToBody(obj.vertices.size(), -1);
        std::vector<Quad> bodyQuads;
        std::vector<Vec3> originalBodyPositions;
        std::vector<uint32_t> bodyOriginalIndices;
        for (const Quad& quad : obj.quads) {
            if (quad.group != "body") continue;
            bodyQuads.push_back(quad);
            for (uint32_t index : quad.v) {
                if (originalToBody[index] < 0) {
                    originalToBody[index] = static_cast<int32_t>(originalBodyPositions.size());
                    originalBodyPositions.push_back(obj.vertices[index]);
                    bodyOriginalIndices.push_back(index);
                }
            }
        }
        std::vector<uint64_t> stableIds;
        stableIds.reserve(bodyOriginalIndices.size());
        for (uint32_t original : bodyOriginalIndices) stableIds.push_back(stableVertexId(original));
        MeshData baseMesh;
        baseMesh.vertices.reserve(originalBodyPositions.size());
        for (const Vec3& p : originalBodyPositions) {
            Vertex vertex;
            vertex.position = p;
            baseMesh.vertices.push_back(vertex);
        }
        for (const Quad& quad : bodyQuads) {
            const uint32_t ids[4] = {
                static_cast<uint32_t>(originalToBody[quad.v[0]]),
                static_cast<uint32_t>(originalToBody[quad.v[1]]),
                static_cast<uint32_t>(originalToBody[quad.v[2]]),
                static_cast<uint32_t>(originalToBody[quad.v[3]])
            };
            baseMesh.indices.insert(baseMesh.indices.end(), {ids[0], ids[1], ids[2],
                                                             ids[0], ids[2], ids[3]});
        }
        recomputeNormals(baseMesh);
        const Quality baseQuality = measureQuality(baseMesh, true);
        size_t bodyUvCorners = 0;
        std::set<std::pair<uint32_t, uint32_t>> quadEdges;
        std::vector<uint32_t> quadValence(baseMesh.vertices.size(), 0);
        for (const Quad& quad : bodyQuads) {
            for (uint32_t uv : quad.vt) {
                if (uv != std::numeric_limits<uint32_t>::max()) ++bodyUvCorners;
            }
            for (size_t corner = 0; corner < 4; ++corner) {
                const uint32_t a = quad.v[corner];
                const uint32_t b = quad.v[(corner + 1) % 4];
                quadEdges.insert({std::min(a, b), std::max(a, b)});
                ++quadValence[a];
                ++quadValence[b];
            }
        }
        const auto valenceRange = std::minmax_element(quadValence.begin(), quadValence.end());
        std::map<uint32_t, size_t> valenceHistogram;
        for (uint32_t valence : quadValence) ++valenceHistogram[valence];
        std::ostringstream valenceText;
        bool firstValence = true;
        for (const auto& entry : valenceHistogram) {
            if (!firstValence) valenceText << ",";
            valenceText << entry.first << ":" << entry.second;
            firstValence = false;
        }
        std::cout << "BASE vertices=" << obj.vertices.size() << " body_vertices=" << baseMesh.vertices.size()
                  << " quads=" << bodyQuads.size() << " triangles=" << baseMesh.indices.size() / 3
                  << " ngons=0" << " quad_edges=" << quadEdges.size()
                  << " quad_valence=" << *valenceRange.first << "-" << *valenceRange.second
                  << " quad_valence_hist=" << valenceText.str()
                  << " uvs=" << obj.uvs.size() << " body_uv_corners=" << bodyUvCorners
                  << " groups=" << obj.groupFaceCounts.size()
                  << " triangle_edges=" << baseQuality.edges
                  << " edge_length_m=" << baseQuality.minEdgeLength << "-" << baseQuality.maxEdgeLength
                  << " signed_volume_m3=" << baseQuality.signedVolume
                  << " components=" << baseQuality.topology.components
                  << " boundary=" << baseQuality.topology.boundaryEdges
                  << " nonmanifold=" << baseQuality.topology.nonManifoldEdges
                  << " same_direction=" << baseQuality.sameDirectionEdges
                  << " duplicates=" << baseQuality.duplicateFaces
                  << " slivers=" << baseQuality.slivers
                  << " min_angle=" << baseQuality.minAngleDeg
                  << " max_aspect=" << baseQuality.maxAspect
                  << " self_intersections=" << baseQuality.selfIntersections << "\n";

        const std::vector<std::string> targetNames = {
            "torso/torso-scale-horiz-incr.target",
            "torso/torso-scale-depth-incr.target",
            "measure/measure-waist-circ-incr.target",
            "measure/measure-shoulder-dist-incr.target",
            "hip/hip-scale-horiz-incr.target",
            "armslegs/l-upperarm-shoulder-muscle-incr.target",
            "armslegs/r-upperarm-shoulder-muscle-incr.target",
            "measure/measure-upperarm-circ-incr.target"
        };
        std::vector<ShapeTarget> targets;
        for (const std::string& name : targetNames) {
            targets.push_back(loadTarget(data / "makehuman/data/targets" / name, originalToBody));
        }
        for (const ShapeTarget& target : targets) {
            MeshData changed = baseMesh;
            float maxDelta = 0.0f;
            Vec3 mn{1e9f, 1e9f, 1e9f};
            Vec3 mx{-1e9f, -1e9f, -1e9f};
            Vec3 centroid{};
            for (const Delta& delta : target.deltas) {
                changed.vertices[delta.body].position = changed.vertices[delta.body].position + delta.value;
                maxDelta = std::max(maxDelta, delta.value.length());
                const Vec3& p = baseMesh.vertices[delta.body].position;
                mn.x = std::min(mn.x, p.x); mn.y = std::min(mn.y, p.y); mn.z = std::min(mn.z, p.z);
                mx.x = std::max(mx.x, p.x); mx.y = std::max(mx.y, p.y); mx.z = std::max(mx.z, p.z);
                centroid = centroid + p;
            }
            if (!target.deltas.empty()) centroid = centroid * (1.0f / static_cast<float>(target.deltas.size()));
            recomputeNormals(changed);
            const Quality quality = measureQuality(changed, true);
            std::cout << "TARGET " << target.name << " deltas=" << target.deltas.size()
                      << " max_delta=" << maxDelta
                      << " bbox=(" << mn.x << "," << mn.y << "," << mn.z << ")-("
                      << mx.x << "," << mx.y << "," << mx.z << ")"
                      << " centroid=(" << centroid.x << "," << centroid.y << "," << centroid.z << ")"
                      << " boundary=" << quality.topology.boundaryEdges
                      << " nonmanifold=" << quality.topology.nonManifoldEdges
                      << " same_direction=" << quality.sameDirectionEdges
                      << " degen=" << quality.topology.degenerateTriangles
                      << " slivers=" << quality.slivers
                      << " self_intersections=" << quality.selfIntersections << "\n";
            std::vector<Vec3> targetPositions = obj.vertices;
            for (size_t i = 0; i < changed.vertices.size(); ++i) {
                targetPositions[bodyOriginalIndices[i]] = changed.vertices[i].position;
            }
            exportObj(out / (target.name + ".obj"), obj, bodyQuads, targetPositions);
        }
        MeshData combined = baseMesh;
        for (const ShapeTarget& target : targets) {
            for (const Delta& delta : target.deltas) {
                combined.vertices[delta.body].position = combined.vertices[delta.body].position + delta.value;
            }
        }
        recomputeNormals(combined);
        const Quality combinedQuality = measureQuality(combined, true);
        std::cout << "COMBINED targets=" << targets.size()
                  << " boundary=" << combinedQuality.topology.boundaryEdges
                  << " nonmanifold=" << combinedQuality.topology.nonManifoldEdges
                  << " same_direction=" << combinedQuality.sameDirectionEdges
                  << " degen=" << combinedQuality.topology.degenerateTriangles
                  << " slivers=" << combinedQuality.slivers
                  << " self_intersections=" << combinedQuality.selfIntersections << "\n";
        std::vector<Vec3> combinedPositions = obj.vertices;
        for (size_t i = 0; i < combined.vertices.size(); ++i) {
            combinedPositions[bodyOriginalIndices[i]] = combined.vertices[i].position;
        }
        exportObj(out / "combined-targets.obj", obj, bodyQuads, combinedPositions);
        MeshData zero = baseMesh;
        float zeroError = 0.0f;
        for (const ShapeTarget& target : targets) {
            for (const Delta& delta : target.deltas) {
                zero.vertices[delta.body].position = zero.vertices[delta.body].position + delta.value * 0.0f;
                zeroError = std::max(zeroError,
                    (zero.vertices[delta.body].position - baseMesh.vertices[delta.body].position).length());
            }
        }
        std::cout << "ZERO_RESET max_error=" << zeroError << " stable_ids=" << stableIds.size() << "\n";

        Rig rig = loadRig(data / "makehuman/data/rigs/default.mhskel", obj);
        const auto weights = loadWeights(data / "makehuman/data/rigs/default_weights.mhw", rig,
                                         originalToBody, baseMesh.vertices.size());
        std::cout << "RIG bones=" << rig.bones.size() << " weighted_vertices=" << weights.size() << "\n";
        std::ofstream weightsCsv(out / "weights.csv");
        weightsCsv << "body_index,original_index,bone0,weight0,bone1,weight1,bone2,weight2,bone3,weight3\n";
        for (size_t i = 0; i < weights.size(); ++i) {
            weightsCsv << i << "," << bodyOriginalIndices[i];
            for (const Influence& influence : weights[i]) {
                weightsCsv << "," << influence.bone << "," << influence.weight;
            }
            weightsCsv << "\n";
        }
        const std::vector<Vec3> basePositions = originalBodyPositions;
        // World frame of the source mesh: +X = the character's left, +Y = up,
        // +Z = forward, metres. Each action carries a direction assertion, so
        // a joint that bends the wrong way fails the proof instead of passing
        // on "the numbers are finite".
        struct Expect {
            const char* joint;  // bone whose HEAD joint is tracked
            const char* ref;    // reference bone (nullptr = world origin / no displacement to subtract)
            Vec3 dir;           // world direction
            float minMove;      // metres; negative values only bound the opposite drift
            bool positional = false;  // false: displacement of the joint minus the reference's displacement
                                      // true:  posed position of the joint minus the reference's posed position
        };
        struct ActionSpec {
            const char* name;
            std::vector<std::pair<const char*, Quat>> rotations;
            std::vector<Expect> expect;
            const char* still;  // mirror-side joint that must not move (nullptr = none)
            float mirrorX;      // body vertices beyond this x (sign gives the side) form the mirror limb
            float twistDeg;     // 0 = none; else the twist about +Y through the rotated bone's head joint
            const char* approachA;      // distance between these two joints must SHRINK by
            const char* approachB;      // approachMin metres (nullptr = none)
            float approachMin;
        };
        // elbow_bend: the rest forearm already points forward-down and outwards
        // (elbow -> wrist = (0.52, -0.46, 0.72)), so flexion about +X by -85 deg
        // lifts the hand up in front of the shoulder. +85 deg (the first
        // version) hyperextends the elbow: the wrist ends 9 cm BEHIND the elbow
        // and 17 cm below it, further away from the shoulder, which the
        // "approach" and "in front of the elbow" checks reject.
        const std::vector<ActionSpec> actions = {
            {"left_abduct", {{"upperarm01.L", axisAngle({0, 0, 1}, 75.0f)}},
             {{"wrist.L", nullptr, {0, 1, 0}, 0.15f}, {"wrist.L", nullptr, {1, 0, 0}, 0.03f}},
             "wrist.R", -0.05f, 0.0f, nullptr, nullptr, 0.0f},
            {"right_abduct", {{"upperarm01.R", axisAngle({0, 0, 1}, -75.0f)}},
             {{"wrist.R", nullptr, {0, 1, 0}, 0.15f}, {"wrist.R", nullptr, {-1, 0, 0}, 0.03f}},
             "wrist.L", 0.05f, 0.0f, nullptr, nullptr, 0.0f},
            {"both_abduct", {{"upperarm01.L", axisAngle({0, 0, 1}, 75.0f)},
                             {"upperarm01.R", axisAngle({0, 0, 1}, -75.0f)}},
             {{"wrist.L", nullptr, {0, 1, 0}, 0.15f}, {"wrist.R", nullptr, {0, 1, 0}, 0.15f}},
             nullptr, 0.0f, 0.0f, nullptr, nullptr, 0.0f},
            {"elbow_bend", {{"lowerarm01.L", axisAngle({1, 0, 0}, -85.0f)}},
             {{"wrist.L", "lowerarm01.L", {0, 0, 1}, -0.02f, true}},
             "wrist.R", -0.05f, 0.0f, "wrist.L", "upperarm01.L", 0.08f},
            {"hip_bend", {{"upperleg01.L", axisAngle({1, 0, 0}, -50.0f)}},
             {{"foot.L", nullptr, {0, 0, 1}, 0.15f}, {"foot.L", nullptr, {0, 1, 0}, 0.05f}},
             "foot.R", -0.05f, 0.0f, nullptr, nullptr, 0.0f},
            {"knee_bend", {{"lowerleg01.L", axisAngle({1, 0, 0}, 60.0f)}},
             {{"foot.L", nullptr, {0, 0, -1}, 0.10f}, {"foot.L", nullptr, {0, 1, 0}, 0.05f}},
             "foot.R", -0.05f, 0.0f, "foot.L", "upperleg01.L", 0.08f},
            {"wrist_rotate", {{"wrist.L", axisAngle({0, 1, 0}, 45.0f)}}, {}, "wrist.R", -0.05f, 45.0f,
             nullptr, nullptr, 0.0f},
            {"ankle_rotate", {{"foot.L", axisAngle({1, 0, 0}, 30.0f)}},
             {{"toe1-1.L", nullptr, {0, -1, 0}, 0.02f}}, "foot.R", -0.05f, 0.0f, nullptr, nullptr, 0.0f},
        };
        // The MakeHuman weight file carries a few stray weights across the midline
        // (<= 1 % of a vertex on the mirror limb's bone), so mirror-limb skin may
        // bleed by fractions of a millimetre. That is reported, not hidden; the
        // mirror joint itself must stay exactly still.
        constexpr float kMirrorJointMax = 1e-5f;
        constexpr float kMirrorLimbMax = 3e-3f;
        exportObj(out / "base.obj", obj, bodyQuads, obj.vertices);
        int directionFailures = 0;
        for (const ActionSpec& spec : actions) {
            std::unordered_map<int, Quat> rotations;
            for (const auto& rotation : spec.rotations) rotations[rig.boneByName.at(rotation.first)] = rotation.second;
            prepareAction(rig, rotations);
            const std::vector<Vec3> posed = skin(basePositions, rig, weights);
            float maxDisplacement = 0.0f;
            bool finite = true;
            for (size_t i = 0; i < posed.size(); ++i) {
                finite = finite && posed[i].finite();
                maxDisplacement = std::max(maxDisplacement, (posed[i] - basePositions[i]).length());
            }
            bool direction = finite;
            // The joint a bone turns about is its own head: it must not drift.
            float pivotMove = 0.0f;
            for (const auto& rotation : spec.rotations) {
                const Bone& bone = rig.bones[static_cast<size_t>(rig.boneByName.at(rotation.first))];
                pivotMove = std::max(pivotMove, (posedPoint(bone, bone.head) - bone.head).length());
            }
            direction = direction && pivotMove < 1e-5f;
            std::ostringstream detail;
            for (const Expect& expect : spec.expect) {
                const Bone& bone = rig.bones[static_cast<size_t>(rig.boneByName.at(expect.joint))];
                Vec3 delta = posedPoint(bone, bone.head) - bone.head;
                if (expect.positional) delta = posedPoint(bone, bone.head);
                if (expect.ref) {
                    const Bone& reference = rig.bones[static_cast<size_t>(rig.boneByName.at(expect.ref))];
                    delta = expect.positional ? delta - posedPoint(reference, reference.head)
                                              : delta - (posedPoint(reference, reference.head) - reference.head);
                }
                const float along = delta.dot(expect.dir);
                direction = direction && along >= expect.minMove;
                detail << " " << expect.joint << (expect.positional ? "_rel=(" : "_d=(") << delta.x << "," << delta.y << ","
                       << delta.z << ")along=" << along;
            }
            float approach = 0.0f;
            if (spec.approachA) {
                const Bone& a = rig.bones[static_cast<size_t>(rig.boneByName.at(spec.approachA))];
                const Bone& b = rig.bones[static_cast<size_t>(rig.boneByName.at(spec.approachB))];
                approach = (a.head - b.head).length() - (posedPoint(a, a.head) - posedPoint(b, b.head)).length();
                direction = direction && approach >= spec.approachMin;
            }
            float twistDeg = 0.0f, twistHeight = 0.0f, twistRadius = 0.0f;
            int twisted = 0;
            if (spec.twistDeg != 0.0f) {
                // Vertices bound entirely to the rotated bone's subtree turn about
                // the vertical axis through its head joint: height and radius hold,
                // and the signed angle is taken anti-clockwise seen from +Y.
                const int pivot = rig.boneByName.at(spec.rotations.front().first);
                const Vec3 centre = rig.bones[static_cast<size_t>(pivot)].head;
                double sinSum = 0.0, cosSum = 0.0;
                for (size_t i = 0; i < posed.size(); ++i) {
                    double bound = 0.0;
                    for (const Influence& influence : weights[i])
                        if (isInSubtree(rig, influence.bone, pivot)) bound += influence.weight;
                    if (bound < 0.999) continue;
                    const Vec3 r0 = basePositions[i] - centre, r1 = posed[i] - centre;
                    sinSum += static_cast<double>(r0.z) * r1.x - static_cast<double>(r0.x) * r1.z;
                    cosSum += static_cast<double>(r0.x) * r1.x + static_cast<double>(r0.z) * r1.z;
                    twistHeight = std::max(twistHeight, std::fabs(r1.y - r0.y));
                    twistRadius = std::max(twistRadius, std::fabs(std::hypot(r1.x, r1.z) - std::hypot(r0.x, r0.z)));
                    ++twisted;
                }
                twistDeg = static_cast<float>(std::atan2(sinSum, cosSum) * 57.2957795);
                direction = direction && twisted > 100 && twistHeight < 1e-4f && twistRadius < 1e-4f &&
                            std::fabs(twistDeg - spec.twistDeg) < 3.0f;
            }
            float stillMove = 0.0f, mirrorLimbMax = 0.0f;
            if (spec.still) {
                const Bone& bone = rig.bones[static_cast<size_t>(rig.boneByName.at(spec.still))];
                stillMove = (posedPoint(bone, bone.head) - bone.head).length();
                direction = direction && stillMove < kMirrorJointMax;
                for (size_t i = 0; i < posed.size(); ++i) {
                    const bool otherSide = spec.mirrorX < 0.0f ? basePositions[i].x < spec.mirrorX : basePositions[i].x > spec.mirrorX;
                    if (otherSide) mirrorLimbMax = std::max(mirrorLimbMax, (posed[i] - basePositions[i]).length());
                }
                direction = direction && mirrorLimbMax < kMirrorLimbMax;
            }
            directionFailures += direction ? 0 : 1;
            std::cout << "ACTION " << spec.name << " finite=" << finite << " max_displacement=" << maxDisplacement
                      << " direction=" << (direction ? "PASS" : "FAIL") << detail.str()
                      << " approach=" << approach
                      << " pivot_move=" << pivotMove << " twist_deg=" << twistDeg << " twist_vertices=" << twisted
                      << " twist_height_drift=" << twistHeight << " twist_radius_drift=" << twistRadius
                      << " mirror_joint_move=" << stillMove << " mirror_limb_max=" << mirrorLimbMax << "\n";
            std::vector<Vec3> exportPositions = obj.vertices;
            for (size_t i = 0; i < basePositions.size(); ++i) {
                exportPositions[bodyOriginalIndices[i]] = posed[i];
            }
            exportObj(out / (std::string(spec.name) + ".obj"), obj, bodyQuads, exportPositions);
        }
        std::cout << "DIRECTION_CHECK actions=" << actions.size() << " failed=" << directionFailures << "\n";
        writeTemplate(out / "base.paehuman", baseMesh, rig, weights, targets, stableIds);
        return directionFailures ? 3 : 0;
    } catch (const std::exception& error) {
        std::cerr << "ERROR " << error.what() << "\n";
        return 1;
    }
}
