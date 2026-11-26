#include "ht_rig.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <set>
#include <unordered_map>

namespace pae::ht {

namespace {

void compressTopN(std::vector<std::vector<std::pair<int, double>>>& lists, int vertexCount, int n, SkinWeights& out,
                  const std::vector<uint8_t>* fallback = nullptr) {
    out.vertexCount = vertexCount;
    out.maxInfluences = n;
    out.bone.assign(static_cast<size_t>(vertexCount), {0, 0, 0, 0});
    out.w.assign(static_cast<size_t>(vertexCount), {0.0f, 0.0f, 0.0f, 0.0f});
    out.count.assign(static_cast<size_t>(vertexCount), 0);
    out.fallback.assign(static_cast<size_t>(vertexCount), static_cast<uint8_t>(WeightFallback::Source));
    if (fallback)
        for (int v = 0; v < vertexCount && v < static_cast<int>(fallback->size()); ++v)
            out.fallback[static_cast<size_t>(v)] = (*fallback)[static_cast<size_t>(v)];
    double influences = 0.0;
    double massLost = 0.0;
    for (int v = 0; v < vertexCount; ++v) {
        std::vector<std::pair<int, double>>& list = lists[static_cast<size_t>(v)];
        // Drop numerical dust before anything else.
        std::vector<std::pair<int, double>> kept;
        double total = 0.0;
        for (const auto& p : list) {
            if (!(p.second > 1e-6)) continue;
            kept.push_back(p);
            total += p.second;
        }
        std::sort(kept.begin(), kept.end(), [](const std::pair<int, double>& a, const std::pair<int, double>& b) {
            return a.second > b.second;
        });
        influences += static_cast<double>(kept.size());
        double keptMass = 0.0;
        for (size_t i = 0; i < kept.size() && i < static_cast<size_t>(n); ++i) keptMass += kept[i].second;
        if (total > 0.0) massLost += (total - keptMass) / total;
        if (keptMass <= 0.0) {
            out.count[static_cast<size_t>(v)] = 0;
            continue;
        }
        for (size_t i = 0; i < kept.size() && i < static_cast<size_t>(n); ++i) {
            out.bone[static_cast<size_t>(v)][i] = static_cast<uint16_t>(kept[i].first);
            out.w[static_cast<size_t>(v)][i] = static_cast<float>(kept[i].second / keptMass);
        }
        out.count[static_cast<size_t>(v)] = static_cast<uint8_t>(std::min(kept.size(), static_cast<size_t>(n)));
    }
    out.meanInfluencesBeforeCompression = vertexCount ? influences / vertexCount : 0.0;
    out.massLostByCompression = vertexCount ? massLost / vertexCount : 0.0;
}

bool startsWith(const std::string& s, const char* prefix) {
    return s.compare(0, std::char_traits<char>::length(prefix), prefix) == 0;
}

// Defined below readMhWeights, which calls it on its result.
void fillUnweighted(SkinWeights& out, const Rig& rig, const RigFile& file,
                    const std::vector<std::vector<std::pair<int, double>>>& dropped, const std::vector<D3>* restPositions);

} // namespace

// --- names, roles, regions ----------------------------------------------------

const char* roleName(Role r) {
    switch (r) {
        case Role::Root: return "root";
        case Role::Pelvis: return "pelvis";
        case Role::Spine: return "spine";
        case Role::Neck: return "neck";
        case Role::Head: return "head";
        case Role::Clavicle: return "clavicle";
        case Role::Shoulder: return "shoulder";
        case Role::UpperArm: return "upper-arm";
        case Role::Forearm: return "forearm";
        case Role::Hand: return "hand";
        case Role::Finger: return "finger";
        case Role::Thigh: return "thigh";
        case Role::Shin: return "shin";
        case Role::Foot: return "foot";
        case Role::Toe: return "toe";
        case Role::Breast: return "breast";
        default: return "face";
    }
}

Role roleOfBoneName(const std::string& name) {
    if (name == "root") return Role::Root;
    if (startsWith(name, "pelvis")) return Role::Pelvis;
    if (startsWith(name, "spine")) return Role::Spine;
    if (startsWith(name, "neck")) return Role::Neck;
    if (startsWith(name, "head")) return Role::Head;
    if (startsWith(name, "clavicle")) return Role::Clavicle;
    if (startsWith(name, "shoulder")) return Role::Shoulder;
    if (startsWith(name, "upperarm")) return Role::UpperArm;
    if (startsWith(name, "lowerarm")) return Role::Forearm;
    if (startsWith(name, "wrist") || startsWith(name, "metacarpal")) return Role::Hand;
    if (startsWith(name, "finger")) return Role::Finger;
    if (startsWith(name, "upperleg")) return Role::Thigh;
    if (startsWith(name, "lowerleg")) return Role::Shin;
    if (startsWith(name, "foot")) return Role::Foot;
    if (startsWith(name, "toe")) return Role::Toe;
    if (startsWith(name, "breast")) return Role::Breast;
    return Role::Face;
}

bool isFacialRole(Role r) { return r == Role::Face || r == Role::Breast; }

const char* regionName(Region r) {
    switch (r) {
        case Region::Head: return "head";
        case Region::Neck: return "neck";
        case Region::Thorax: return "thorax";
        case Region::Abdomen: return "abdomen";
        case Region::Pelvis: return "pelvis";
        case Region::ShoulderGirdle: return "shoulder-girdle";
        case Region::UpperArm: return "upper-arm";
        case Region::Forearm: return "forearm";
        case Region::Hand: return "hand";
        case Region::Thigh: return "thigh";
        case Region::LowerLeg: return "lower-leg";
        case Region::Foot: return "foot";
        default: return "unknown";
    }
}

Region regionOfRole(Role r) {
    switch (r) {
        case Role::Root:
        case Role::Pelvis: return Region::Pelvis;
        case Role::Spine: return Region::Abdomen;  // refined by height in labelRegions
        case Role::Neck: return Region::Neck;
        case Role::Head: return Region::Head;
        case Role::Clavicle:
        case Role::Shoulder: return Region::ShoulderGirdle;
        case Role::UpperArm: return Region::UpperArm;
        case Role::Forearm: return Region::Forearm;
        case Role::Hand:
        case Role::Finger: return Region::Hand;
        case Role::Thigh: return Region::Thigh;
        case Role::Shin: return Region::LowerLeg;
        case Role::Foot:
        case Role::Toe: return Region::Foot;
        default: return Region::Unknown;
    }
}

int Rig::find(const std::string& boneName) const {
    for (size_t i = 0; i < bones.size(); ++i)
        if (bones[i].name == boneName) return static_cast<int>(i);
    return -1;
}

std::vector<int> Rig::ancestors(int bone) const {
    std::vector<int> out;
    int guard = 0;
    int p = bones[static_cast<size_t>(bone)].parent;
    while (p >= 0 && guard++ < static_cast<int>(bones.size()) + 1) {
        out.push_back(p);
        p = bones[static_cast<size_t>(p)].parent;
    }
    return out;
}

// --- .mhskel -------------------------------------------------------------------

bool readRigFile(const std::string& path, RigFile& out, std::string& error) {
    std::string text;
    if (!readFile(path, text)) {
        error = "cannot open " + path;
        return false;
    }
    JsonValue root;
    if (!parseJson(text, root, error)) return false;
    out = RigFile{};
    out.path = path;
    out.sha256 = sha256HexFile(path);
    if (const JsonValue* v = root.find("name"); v && v->isString()) out.name = v->text;
    if (const JsonValue* v = root.find("license"); v && v->isString()) out.license = v->text;
    if (const JsonValue* v = root.find("description"); v && v->isString()) out.description = v->text;
    if (const JsonValue* v = root.find("weights_file"); v && v->isString()) out.weightsFile = v->text;
    if (const JsonValue* v = root.find("version"); v && v->isNumber()) out.version = static_cast<long>(v->number);
    const JsonValue* bones = root.find("bones");
    const JsonValue* joints = root.find("joints");
    if (!bones || !bones->isObject() || !joints || !joints->isObject()) {
        error = "rig file lacks bones/joints objects";
        return false;
    }
    for (const auto& m : bones->members) {
        RigFileBone b;
        b.name = m.first;
        for (const auto& f : m.second.members) {
            if (f.first == "head" && f.second.isString()) b.headJoint = f.second.text;
            else if (f.first == "tail" && f.second.isString()) b.tailJoint = f.second.text;
            else if (f.first == "parent" && f.second.isString()) b.parent = f.second.text;
            else if (f.first == "rotation_plane" && f.second.isString()) b.planeJoints[0] = f.second.text;
        }
        out.bones.push_back(b);
    }
    std::unordered_map<std::string, std::array<std::string, 3>> planes;
    if (const JsonValue* p = root.find("planes"); p && p->isObject()) {
        for (const auto& m : p->members) {
            std::array<std::string, 3> names;
            if (m.second.isArray() && m.second.items.size() == 3)
                for (int i = 0; i < 3; ++i) names[static_cast<size_t>(i)] = m.second.items[static_cast<size_t>(i)].text;
            planes[m.first] = names;
            ++out.planeCount;
        }
    }
    for (RigFileBone& b : out.bones)
        if (!b.planeJoints[0].empty() && planes.count(b.planeJoints[0])) b.planeJoints = planes[b.planeJoints[0]];
    for (const auto& m : joints->members) {
        RigFileJoint j;
        j.name = m.first;
        if (m.second.isNumber()) {
            j.objVertexIndices.push_back(static_cast<long>(m.second.number));
        } else if (m.second.isArray()) {
            for (const JsonValue& item : m.second.items)
                if (item.isNumber()) j.objVertexIndices.push_back(static_cast<long>(item.number));
        }
        out.joints.push_back(j);
    }
    return true;
}

bool resolveJoints(const RigFile& file, const std::vector<D3>& positions, std::map<std::string, D3>& out,
                   int& unresolvedCount) {
    out.clear();
    unresolvedCount = 0;
    const long count = static_cast<long>(positions.size());
    for (const RigFileJoint& j : file.joints) {
        D3 sum;
        long used = 0;
        for (long idx : j.objVertexIndices) {
            const long id = idx - 1;  // 1-based OBJ index
            if (id < 0 || id >= count) continue;
            sum += positions[static_cast<size_t>(id)];
            ++used;
        }
        if (used == 0) {
            ++unresolvedCount;
            continue;
        }
        out[j.name] = sum * (1.0 / static_cast<double>(used));
    }
    return unresolvedCount == 0;
}

std::vector<std::string> coreBoneNames() {
    std::vector<std::string> names = {"root", "spine01", "spine03", "spine05", "neck01", "head"};
    for (const char* side : {".L", ".R"}) {
        for (const char* stem : {"clavicle", "upperarm01", "lowerarm01", "wrist", "upperleg01", "lowerleg01", "foot"}) {
            names.push_back(std::string(stem) + side);
        }
    }
    return names;
}

bool buildRigV2(const RigFile& file, const std::map<std::string, D3>& joints, bool coreOnly, Rig& out) {
    out = Rig{};
    out.name = coreOnly ? "PAEHumanRigV2-core" : "PAEHumanRigV2-deform";
    out.source = file.path;
    out.license = file.license;
    out.formatVersion = 2;
    const std::vector<std::string> core = coreBoneNames();
    std::set<std::string> keep(core.begin(), core.end());
    std::unordered_map<std::string, int> newIndex;
    std::unordered_map<std::string, std::string> fileParent;
    for (const RigFileBone& b : file.bones) fileParent[b.name] = b.parent;
    for (const RigFileBone& b : file.bones) {
        const Role role = roleOfBoneName(b.name);
        if (isFacialRole(role)) continue;
        if (coreOnly && !keep.count(b.name)) continue;
        auto head = joints.find(b.headJoint), tail = joints.find(b.tailJoint);
        if (head == joints.end() || tail == joints.end()) return false;
        RigBone bone;
        bone.name = b.name;
        bone.head = head->second;
        bone.tail = tail->second;
        bone.role = role;
        bone.headJoint = b.headJoint;
        bone.tailJoint = b.tailJoint;
        bone.planeNormal = D3{};
        if (!b.planeJoints[0].empty()) {
            const auto p0 = joints.find(b.planeJoints[0]), p1 = joints.find(b.planeJoints[1]), p2 = joints.find(b.planeJoints[2]);
            if (p0 != joints.end() && p1 != joints.end() && p2 != joints.end())
                bone.planeNormal = (p1->second - p0->second).cross(p2->second - p0->second).normalized();
        }
        newIndex[b.name] = static_cast<int>(out.bones.size());
        out.bones.push_back(bone);
    }
    // Parents: nearest kept ancestor in the source hierarchy.
    for (RigBone& bone : out.bones) {
        std::string p = fileParent[bone.name];
        int guard = 0;
        while (!p.empty() && !newIndex.count(p) && guard++ < static_cast<int>(file.bones.size()) + 1) p = fileParent[p];
        bone.parent = newIndex.count(p) ? newIndex[p] : -1;
    }
    return !out.bones.empty();
}

std::vector<double> boneRadii(const PolyMesh& body, const Rig& rig) {
    // Radius of a bone = median surface distance of the vertices that are
    // closest (plain distance) to that bone. The median over the bone's own
    // neighbourhood is robust for long thin segments (shin, forearm), where a
    // "k nearest vertices" estimate would be biased towards the joint ends.
    const size_t vc = body.pos.size(), bc = rig.bones.size();
    std::vector<double> radii(bc, 0.0);
    std::vector<int> nearest(vc, 0);
    std::vector<double> nearestDistance(vc, 0.0);
    std::vector<std::vector<double>> own(bc);
    for (size_t v = 0; v < vc; ++v) {
        double best = 1e300;
        for (size_t b = 0; b < bc; ++b) {
            const double d = distanceToSegment(body.pos[v], rig.bones[b].head, rig.bones[b].tail);
            if (d < best) { best = d; nearest[v] = static_cast<int>(b); }
        }
        nearestDistance[v] = best;
        own[static_cast<size_t>(nearest[v])].push_back(best);
    }
    for (size_t b = 0; b < bc; ++b) {
        if (own[b].size() >= 8) {
            std::vector<double> d = own[b];
            std::sort(d.begin(), d.end());
            radii[b] = d[d.size() / 2];
        } else {
            // Bones that own no vertex of their own (e.g. short twist bones
            // inside a larger bone) fall back to their 16 nearest vertices.
            std::vector<double> d(vc);
            for (size_t v = 0; v < vc; ++v) d[v] = distanceToSegment(body.pos[v], rig.bones[b].head, rig.bones[b].tail);
            const size_t k = std::min<size_t>(16, vc);
            std::partial_sort(d.begin(), d.begin() + static_cast<std::ptrdiff_t>(k), d.end());
            for (size_t i = 0; i < k; ++i) radii[b] += d[i];
            radii[b] /= static_cast<double>(k);
        }
        radii[b] = std::max(radii[b], 1e-4);
    }
    return radii;
}

std::vector<Region> labelRegions(const PolyMesh& body, const Rig& rig) {
    const std::vector<double> radii = boneRadii(body, rig);
    // Central-column landmarks: hip-joint height and the base of the neck.
    double hipY = 0.0, neckY = 1e30;
    int hipCount = 0, neckCount = 0;
    for (const RigBone& b : rig.bones) {
        if (b.role == Role::Thigh && b.name.find("01") != std::string::npos) { hipY += b.head.y; ++hipCount; }
        if (b.role == Role::Neck) { neckY = std::min(neckY, b.head.y); ++neckCount; }
    }
    if (hipCount) hipY /= hipCount;
    if (!neckCount) neckY = hipY + 1.0;
    // Torso zones by height fraction between hip joint and neck base. The
    // source rig has short "joint" bones (neck02, spine04/05) whose plain
    // neighbourhood would otherwise swallow the whole upper trunk.
    auto torsoZone = [&](double y) {
        if (y >= neckY) return Region::Neck;
        const double f = neckY != hipY ? (y - hipY) / (neckY - hipY) : 1.0;
        return f < 0.28 ? Region::Pelvis : (f < 0.50 ? Region::Abdomen : Region::Thorax);
    };
    std::vector<Region> labels(body.pos.size(), Region::Unknown);
    for (size_t v = 0; v < body.pos.size(); ++v) {
        double best = 1e300;
        size_t bestBone = 0;
        for (size_t b = 0; b < rig.bones.size(); ++b) {
            const double q = distanceToSegment(body.pos[v], rig.bones[b].head, rig.bones[b].tail) / radii[b];
            if (q < best) { best = q; bestBone = b; }
        }
        const Role role = rig.bones[bestBone].role;
        switch (role) {
            case Role::Root:
            case Role::Pelvis:
            case Role::Spine:
            case Role::Neck:
                labels[v] = torsoZone(body.pos[v].y);
                break;
            case Role::Head:
                labels[v] = body.pos[v].y >= neckY ? Region::Head : torsoZone(body.pos[v].y);
                break;
            default:
                labels[v] = regionOfRole(role);
                break;
        }
    }
    return labels;
}

std::vector<long> nearestBoneCounts(const PolyMesh& body, const Rig& rig) {
    const std::vector<double> radii = boneRadii(body, rig);
    std::vector<long> counts(rig.bones.size(), 0);
    for (size_t v = 0; v < body.pos.size(); ++v) {
        double best = 1e300;
        size_t bestBone = 0;
        for (size_t b = 0; b < rig.bones.size(); ++b) {
            const double q = distanceToSegment(body.pos[v], rig.bones[b].head, rig.bones[b].tail) / radii[b];
            if (q < best) { best = q; bestBone = b; }
        }
        ++counts[bestBone];
    }
    return counts;
}

// --- weights -------------------------------------------------------------------

bool readMhWeights(const std::string& path, const Rig& rig, int vertexCount, SkinWeights& out, std::string& error,
                   const RigFile* hierarchy, const std::vector<D3>* restPositions) {
    std::string text;
    if (!readFile(path, text)) {
        error = "cannot open " + path;
        return false;
    }
    JsonValue root;
    if (!parseJson(text, root, error)) return false;
    const JsonValue* weights = root.find("weights");
    if (!weights || !weights->isObject()) {
        error = "weights file lacks weights object";
        return false;
    }
    out = SkinWeights{};
    out.source = path;
    out.generator = "makehuman-mhw";
    if (const JsonValue* v = root.find("license"); v && v->isString()) out.license = v->text;
    // Index base: 0 is not a legal 1-based index; V is not a legal 0-based one.
    long minIdx = 1, maxIdx = 0;
    bool any = false;
    for (const auto& group : weights->members)
        for (const JsonValue& pair : group.second.items) {
            if (!pair.isArray() || pair.items.size() < 2 || !pair.items[0].isNumber()) continue;
            const long idx = static_cast<long>(pair.items[0].number);
            minIdx = std::min(minIdx, idx);
            maxIdx = std::max(maxIdx, idx);
            any = true;
        }
    if (any && minIdx == 0) out.detectedIndexBase = 0;
    else if (any && maxIdx == vertexCount) out.detectedIndexBase = 1;
    else out.detectedIndexBase = 0;  // fallback, reported through detectedIndexBase
    const int base = out.detectedIndexBase;

    std::vector<std::vector<std::pair<int, double>>> lists(static_cast<size_t>(vertexCount));
    // Entries on bones the rig does not keep are remembered per vertex (source
    // bone index into hierarchy) so the fallback can still use their mass.
    std::unordered_map<std::string, int> sourceBoneIndex;
    std::vector<std::vector<std::pair<int, double>>> dropped;
    if (hierarchy) {
        dropped.assign(static_cast<size_t>(vertexCount), {});
        for (size_t i = 0; i < hierarchy->bones.size(); ++i) sourceBoneIndex[hierarchy->bones[static_cast<size_t>(i)].name] = static_cast<int>(i);
    }
    for (const auto& group : weights->members) {
        const int bone = rig.find(group.first);
        for (const JsonValue& pair : group.second.items) {
            if (!pair.isArray() || pair.items.size() < 2 || !pair.items[0].isNumber() || !pair.items[1].isNumber()) continue;
            ++out.entriesRead;
            const long id = static_cast<long>(pair.items[0].number) - base;
            if (id < 0 || id >= vertexCount) { ++out.outOfRangeEntries; continue; }
            if (bone < 0) {
                ++out.unknownBoneEntries;
                const auto source = sourceBoneIndex.find(group.first);
                if (source != sourceBoneIndex.end()) dropped[static_cast<size_t>(id)].push_back({source->second, pair.items[1].number});
                continue;
            }
            lists[static_cast<size_t>(id)].push_back({bone, pair.items[1].number});
        }
    }
    compressTopN(lists, vertexCount, 4, out);
    if (hierarchy) fillUnweighted(out, rig, *hierarchy, dropped, restPositions);
    return true;
}

namespace {

// Nearest bone of `rig` found when walking up the source hierarchy from
// `sourceBone` (itself included). -1 when the chain ends without one.
int nearestKeptAncestor(const Rig& rig, const RigFile& file, const std::unordered_map<std::string, int>& indexOf, int sourceBone) {
    int current = sourceBone;
    for (int guard = 0; guard <= static_cast<int>(file.bones.size()); ++guard) {
        const RigFileBone& bone = file.bones[static_cast<size_t>(current)];
        const int kept = rig.find(bone.name);
        if (kept >= 0) return kept;
        if (bone.parent.empty()) return -1;
        const auto parent = indexOf.find(bone.parent);
        if (parent == indexOf.end()) return -1;
        current = parent->second;
    }
    return -1;  // hierarchy cycle (rejected by the rig tests anyway)
}

void fillUnweighted(SkinWeights& out, const Rig& rig, const RigFile& file,
                    const std::vector<std::vector<std::pair<int, double>>>& dropped, const std::vector<D3>* restPositions) {
    std::unordered_map<std::string, int> indexOf;
    for (size_t i = 0; i < file.bones.size(); ++i) indexOf[file.bones[static_cast<size_t>(i)].name] = static_cast<int>(i);
    std::vector<int> ancestor(file.bones.size(), -2);  // -2 = not resolved yet
    for (int v = 0; v < out.vertexCount; ++v) {
        const size_t index = static_cast<size_t>(v);
        if (out.count[index] != 0) continue;  // a kept source weight exists: never touched
        std::vector<std::pair<int, double>> merged;
        double total = 0.0;
        for (const auto& entry : dropped[static_cast<size_t>(v)]) {
            if (ancestor[static_cast<size_t>(entry.first)] == -2)
                ancestor[static_cast<size_t>(entry.first)] = nearestKeptAncestor(rig, file, indexOf, entry.first);
            const int kept = ancestor[static_cast<size_t>(entry.first)];
            if (kept < 0) continue;
            const auto found = std::find_if(merged.begin(), merged.end(), [kept](const std::pair<int, double>& p) { return p.first == kept; });
            if (found == merged.end()) merged.push_back({kept, entry.second});
            else found->second += entry.second;
            total += entry.second;
        }
        if (total > 0.0) {
            // Same treatment as a source list: strongest first, Top-N, renormalised.
            std::sort(merged.begin(), merged.end(), [](const std::pair<int, double>& a, const std::pair<int, double>& b) { return a.second > b.second; });
            const size_t n = std::min(merged.size(), static_cast<size_t>(out.maxInfluences));
            double keptMass = 0.0;
            for (size_t k = 0; k < n; ++k) keptMass += merged[k].second;
            for (size_t k = 0; k < n; ++k) {
                out.bone[index][k] = static_cast<uint16_t>(merged[k].first);
                out.w[index][k] = static_cast<float>(merged[k].second / keptMass);
            }
            out.count[index] = static_cast<uint8_t>(n);
            out.fallback[index] = static_cast<uint8_t>(WeightFallback::KeptAncestor);
        } else if (restPositions && static_cast<int>(restPositions->size()) > v) {
            int best = 0;
            double bestDistance = 1e300;
            for (size_t b = 0; b < rig.bones.size(); ++b) {
                const double d = distanceToSegment((*restPositions)[static_cast<size_t>(v)], rig.bones[b].head, rig.bones[b].tail);
                if (d < bestDistance) { bestDistance = d; best = static_cast<int>(b); }
            }
            out.bone[index][0] = static_cast<uint16_t>(best);
            out.w[index][0] = 1.0f;
            out.count[index] = 1;
            out.fallback[index] = static_cast<uint8_t>(WeightFallback::NearestBone);
        }
    }
}

} // namespace

const char* weightFallbackName(WeightFallback rule) {
    switch (rule) {
        case WeightFallback::KeptAncestor: return "kept-ancestor";
        case WeightFallback::NearestBone: return "nearest-bone";
        default: return "source";
    }
}

long countFallback(const SkinWeights& sw, WeightFallback rule, int vertexLimit) {
    const int n = vertexLimit > 0 ? std::min(vertexLimit, sw.vertexCount) : sw.vertexCount;
    long count = 0;
    for (int v = 0; v < n && v < static_cast<int>(sw.fallback.size()); ++v)
        count += sw.fallback[static_cast<size_t>(v)] == static_cast<uint8_t>(rule) ? 1 : 0;
    return count;
}

namespace {

void finishWeights(std::vector<std::vector<std::pair<int, double>>>& lists, int vertexCount,
                   const char* generator, SkinWeights& out) {
    for (int v = 0; v < vertexCount; ++v) {
        double sum = 0.0;
        for (const auto& p : lists[static_cast<size_t>(v)]) sum += p.second;
        if (sum > 0.0)
            for (auto& p : lists[static_cast<size_t>(v)]) p.second /= sum;
    }
    compressTopN(lists, vertexCount, 4, out);
    out.generator = generator;
    out.source = "PAE host generator";
}

} // namespace

SkinWeights generateDistanceWeights(const PolyMesh& body, const Topology& topo, const Rig& rig, int smoothingIterations) {
    const double t0 = nowMs();
    const std::vector<double> radii = boneRadii(body, rig);
    const int vc = static_cast<int>(body.pos.size()), bc = static_cast<int>(rig.bones.size());
    std::vector<std::vector<std::pair<int, double>>> lists(static_cast<size_t>(vc));
    for (int v = 0; v < vc; ++v) {
        std::vector<double> d(static_cast<size_t>(bc));
        double dmin = 1e300;
        int bmin = 0;
        for (int b = 0; b < bc; ++b) {
            d[static_cast<size_t>(b)] = distanceToSegment(body.pos[static_cast<size_t>(v)], rig.bones[static_cast<size_t>(b)].head,
                                                          rig.bones[static_cast<size_t>(b)].tail);
            if (d[static_cast<size_t>(b)] < dmin) { dmin = d[static_cast<size_t>(b)]; bmin = b; }
        }
        // Region-aware cut: only bones within one local bone radius of the
        // closest one can influence the vertex (keeps hands off thighs).
        const double cut = dmin + radii[static_cast<size_t>(bmin)];
        for (int b = 0; b < bc; ++b) {
            if (d[static_cast<size_t>(b)] > cut) continue;
            const double eps = 0.25 * radii[static_cast<size_t>(b)];
            lists[static_cast<size_t>(v)].push_back({b, 1.0 / (d[static_cast<size_t>(b)] * d[static_cast<size_t>(b)] + eps * eps)});
        }
    }
    for (int it = 0; it < smoothingIterations; ++it) {
        std::vector<std::vector<std::pair<int, double>>> next(static_cast<size_t>(vc));
        for (int v = 0; v < vc; ++v) {
            const auto& nb = topo.vertexNeighbours[static_cast<size_t>(v)];
            std::vector<std::pair<int, double>> acc = lists[static_cast<size_t>(v)];
            auto add = [&](int bone, double w) {
                for (auto& p : acc)
                    if (p.first == bone) { p.second += w; return; }
                acc.push_back({bone, w});
            };
            for (int n : nb)
                for (const auto& p : lists[static_cast<size_t>(n)]) add(p.first, p.second);
            const double norm = 1.0 / static_cast<double>(nb.size() + 1);
            for (auto& p : acc) p.second *= norm;
            next[static_cast<size_t>(v)] = std::move(acc);
        }
        lists.swap(next);
    }
    SkinWeights out;
    finishWeights(lists, vc, "pae-distance", out);
    out.generationMs = nowMs() - t0;
    return out;
}

SkinWeights generateHeatWeights(const PolyMesh& body, const Topology& topo, const Rig& rig, double diffusionTime,
                                int iterations, double power) {
    const double t0 = nowMs();
    const std::vector<double> radii = boneRadii(body, rig);
    const int vc = static_cast<int>(body.pos.size()), bc = static_cast<int>(rig.bones.size());
    std::vector<double> heat(static_cast<size_t>(vc) * static_cast<size_t>(bc), 0.0), source(heat.size(), 0.0);
    std::vector<int> nearest(static_cast<size_t>(vc), 0);
    for (int v = 0; v < vc; ++v) {
        double best = 1e300;
        for (int b = 0; b < bc; ++b) {
            const double q = distanceToSegment(body.pos[static_cast<size_t>(v)], rig.bones[static_cast<size_t>(b)].head,
                                               rig.bones[static_cast<size_t>(b)].tail) / radii[static_cast<size_t>(b)];
            if (q < best) { best = q; nearest[static_cast<size_t>(v)] = b; }
        }
        source[static_cast<size_t>(v) * static_cast<size_t>(bc) + static_cast<size_t>(nearest[static_cast<size_t>(v)])] = 1.0;
    }
    // Jacobi iteration for (I + t L) h = e  with the uniform graph Laplacian.
    for (int it = 0; it < iterations; ++it) {
        std::vector<double> next(heat.size(), 0.0);
        for (int v = 0; v < vc; ++v) {
            const auto& nb = topo.vertexNeighbours[static_cast<size_t>(v)];
            const double denom = 1.0 + diffusionTime * static_cast<double>(nb.size());
            for (int b = 0; b < bc; ++b) {
                double sum = 0.0;
                for (int n : nb) sum += heat[static_cast<size_t>(n) * static_cast<size_t>(bc) + static_cast<size_t>(b)];
                next[static_cast<size_t>(v) * static_cast<size_t>(bc) + static_cast<size_t>(b)] =
                    (source[static_cast<size_t>(v) * static_cast<size_t>(bc) + static_cast<size_t>(b)] + diffusionTime * sum) / denom;
            }
        }
        heat.swap(next);
    }
    std::vector<double> maxHeat(static_cast<size_t>(bc), 0.0);
    for (int b = 0; b < bc; ++b)
        for (int v = 0; v < vc; ++v)
            maxHeat[static_cast<size_t>(b)] = std::max(maxHeat[static_cast<size_t>(b)], heat[static_cast<size_t>(v) * static_cast<size_t>(bc) + static_cast<size_t>(b)]);
    std::vector<std::vector<std::pair<int, double>>> lists(static_cast<size_t>(vc));
    for (int v = 0; v < vc; ++v)
        for (int b = 0; b < bc; ++b) {
            const double h = heat[static_cast<size_t>(v) * static_cast<size_t>(bc) + static_cast<size_t>(b)];
            if (maxHeat[static_cast<size_t>(b)] <= 0.0 || h <= 1e-5 * maxHeat[static_cast<size_t>(b)]) continue;
            lists[static_cast<size_t>(v)].push_back({b, std::pow(h / maxHeat[static_cast<size_t>(b)], power)});
        }
    SkinWeights out;
    finishWeights(lists, vc, "pae-heat", out);
    out.generationMs = nowMs() - t0;
    return out;
}

void extendWeightsToAux(SkinWeights& sw, const std::vector<D3>& fullPositions, const Rig& rig) {
    const int bodyCount = sw.vertexCount;
    const int fullCount = static_cast<int>(fullPositions.size());
    if (fullCount <= bodyCount) return;
    sw.bone.resize(static_cast<size_t>(fullCount), {0, 0, 0, 0});
    sw.w.resize(static_cast<size_t>(fullCount), {0.0f, 0.0f, 0.0f, 0.0f});
    sw.count.resize(static_cast<size_t>(fullCount), 0);
    // Auxiliary vertices get their nearest bone, i.e. the same rule as the
    // NearestBone fallback; flag them so the template stays auditable.
    if (sw.fallback.size() < static_cast<size_t>(bodyCount)) sw.fallback.assign(static_cast<size_t>(bodyCount), 0);
    sw.fallback.resize(static_cast<size_t>(fullCount), static_cast<uint8_t>(WeightFallback::NearestBone));
    for (int v = bodyCount; v < fullCount; ++v) {
        double best = 1e300;
        int bestBone = 0;
        for (size_t b = 0; b < rig.bones.size(); ++b) {
            const double d = distanceToSegment(fullPositions[static_cast<size_t>(v)], rig.bones[b].head, rig.bones[b].tail);
            if (d < best) { best = d; bestBone = static_cast<int>(b); }
        }
        sw.bone[static_cast<size_t>(v)][0] = static_cast<uint16_t>(bestBone);
        sw.w[static_cast<size_t>(v)][0] = 1.0f;
        sw.count[static_cast<size_t>(v)] = 1;
    }
    sw.vertexCount = fullCount;
}

SkinWeights remapToRig(const SkinWeights& src, const Rig& from, const Rig& to) {
    SkinWeights out = src;
    out.generator = "remap:" + src.generator;
    out.maxInfluences = src.maxInfluences;
    std::vector<std::vector<std::pair<int, double>>> lists(static_cast<size_t>(src.vertexCount));
    std::vector<int> mapTo(static_cast<size_t>(from.bones.size()), -1);
    for (size_t b = 0; b < from.bones.size(); ++b) {
        int target = to.find(from.bones[b].name);
        if (target < 0)
            for (int a : from.ancestors(static_cast<int>(b))) {
                target = to.find(from.bones[static_cast<size_t>(a)].name);
                if (target >= 0) break;
            }
        mapTo[b] = target;
    }
    for (int v = 0; v < src.vertexCount; ++v)
        for (int k = 0; k < src.count[static_cast<size_t>(v)]; ++k) {
            const int bone = mapTo[src.bone[static_cast<size_t>(v)][static_cast<size_t>(k)]];
            if (bone >= 0) lists[static_cast<size_t>(v)].push_back({bone, src.w[static_cast<size_t>(v)][static_cast<size_t>(k)]});
        }
    compressTopN(lists, src.vertexCount, src.maxInfluences, out, &src.fallback);
    return out;
}

WeightStats weightStats(const SkinWeights& sw, int vertexLimit) {
    WeightStats s;
    s.minSum = 1e30;
    const int n = vertexLimit > 0 ? std::min(vertexLimit, sw.vertexCount) : sw.vertexCount;
    for (int v = 0; v < n; ++v) {
        const uint8_t n = sw.count[static_cast<size_t>(v)];
        s.meanInfluences += n;
        s.maxInfluences = std::max(s.maxInfluences, static_cast<double>(n));
        double sum = 0.0;
        for (int k = 0; k < n; ++k) sum += sw.w[static_cast<size_t>(v)][static_cast<size_t>(k)];
        if (n == 0) ++s.verticesWithoutWeight;
        else { s.minSum = std::min(s.minSum, sum); s.maxSum = std::max(s.maxSum, sum); }
    }
    if (sw.vertexCount) s.meanInfluences /= sw.vertexCount;
    if (s.minSum > 1e30) s.minSum = 0.0;
    return s;
}

namespace {

int dominantBone(const SkinWeights& sw, int v) {
    const uint8_t n = sw.count[static_cast<size_t>(v)];
    if (n == 0) return -1;
    int best = -1;
    float bestW = -1.0f;
    for (int k = 0; k < n; ++k)
        if (sw.w[static_cast<size_t>(v)][static_cast<size_t>(k)] > bestW) {
            bestW = sw.w[static_cast<size_t>(v)][static_cast<size_t>(k)];
            best = sw.bone[static_cast<size_t>(v)][static_cast<size_t>(k)];
        }
    return best;
}

} // namespace

double weightL1Difference(const SkinWeights& a, const Rig& rigA, const SkinWeights& b, const Rig& rigB) {
    double total = 0.0;
    const int n = std::min(a.vertexCount, b.vertexCount);
    for (int v = 0; v < n; ++v) {
        std::vector<std::pair<std::string, double>> va, vb;
        for (int k = 0; k < a.count[static_cast<size_t>(v)]; ++k)
            va.push_back({rigA.bones[a.bone[static_cast<size_t>(v)][static_cast<size_t>(k)]].name, a.w[static_cast<size_t>(v)][static_cast<size_t>(k)]});
        for (int k = 0; k < b.count[static_cast<size_t>(v)]; ++k)
            vb.push_back({rigB.bones[b.bone[static_cast<size_t>(v)][static_cast<size_t>(k)]].name, b.w[static_cast<size_t>(v)][static_cast<size_t>(k)]});
        std::map<std::string, double> merged;
        for (const auto& p : va) merged[p.first] += p.second;
        for (const auto& p : vb) merged[p.first] -= p.second;
        for (const auto& p : merged) total += std::fabs(p.second);
    }
    return n ? total / static_cast<double>(n) : 0.0;
}

double dominantBoneAgreement(const SkinWeights& a, const Rig& rigA, const SkinWeights& b, const Rig& rigB) {
    int agree = 0;
    const int n = std::min(a.vertexCount, b.vertexCount);
    for (int v = 0; v < n; ++v) {
        const int da = dominantBone(a, v), db = dominantBone(b, v);
        if (da >= 0 && db >= 0 && rigA.bones[static_cast<size_t>(da)].name == rigB.bones[static_cast<size_t>(db)].name) ++agree;
    }
    return n ? static_cast<double>(agree) / static_cast<double>(n) : 0.0;
}

// --- skinning ------------------------------------------------------------------

std::vector<Affine> poseGlobals(const Rig& rig, const std::vector<BoneRotation>& rotations) {
    std::vector<Affine> globals(rig.bones.size());
    std::vector<int> state(rig.bones.size(), 0);  // 0 = open, 1 = in progress, 2 = done
    std::vector<Affine> local(rig.bones.size());
    for (size_t b = 0; b < rig.bones.size(); ++b) {
        const RigBone& bone = rig.bones[b];
        Affine l = Affine::translation(bone.head);
        if (bone.parent >= 0) l.t = bone.head - rig.bones[static_cast<size_t>(bone.parent)].head;
        for (const BoneRotation& r : rotations)
            if (r.bone == static_cast<int>(b) && r.angleDeg != 0.0)
                l = l * Affine::rotation(r.axis, r.angleDeg * 3.14159265358979323846 / 180.0);
        local[b] = l;
    }
    // Iterative evaluation in dependency order (parents may be listed after children).
    for (size_t b = 0; b < rig.bones.size(); ++b) {
        std::vector<int> chain;
        int cur = static_cast<int>(b);
        while (cur >= 0 && state[static_cast<size_t>(cur)] != 2) {
            if (state[static_cast<size_t>(cur)] == 1) break;  // cycle guard
            state[static_cast<size_t>(cur)] = 1;
            chain.push_back(cur);
            cur = rig.bones[static_cast<size_t>(cur)].parent;
        }
        const Affine parentGlobal = cur >= 0 ? globals[static_cast<size_t>(cur)] : Affine::identity();
        Affine g = parentGlobal;
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            g = g * local[static_cast<size_t>(*it)];
            globals[static_cast<size_t>(*it)] = g;
            state[static_cast<size_t>(*it)] = 2;
        }
    }
    return globals;
}

std::vector<Affine> skinMatrices(const Rig& rig, const std::vector<Affine>& globals) {
    std::vector<Affine> skin(rig.bones.size());
    for (size_t b = 0; b < rig.bones.size(); ++b)
        skin[b] = globals[b] * Affine::translation(-rig.bones[b].head);  // rest global is T(head)
    return skin;
}

std::vector<D3> lbsSkin(const std::vector<D3>& rest, const SkinWeights& sw, const std::vector<Affine>& skin) {
    std::vector<D3> out(rest.size());
    for (size_t v = 0; v < rest.size() && v < static_cast<size_t>(sw.vertexCount); ++v) {
        D3 sum;
        const uint8_t n = sw.count[v];
        if (n == 0) { out[v] = rest[v]; continue; }
        for (int k = 0; k < n; ++k) {
            const double w = sw.w[v][static_cast<size_t>(k)];
            if (w == 0.0) continue;
            sum += skin[sw.bone[v][static_cast<size_t>(k)]].apply(rest[v]) * w;
        }
        out[v] = sum;
    }
    return out;
}

} // namespace pae::ht
