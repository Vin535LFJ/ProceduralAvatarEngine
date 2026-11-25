#include "procedural/character/TorsoShoulderPatchGenerator.h"
#include "procedural/character/AnatomicalLandmarks.h"
#include "procedural/shape/v2/CharacterDefinition.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

using namespace pae;
using namespace pae::character;

namespace {

shape::v2::CharacterDefinition variant(const std::string& name) {
    auto definition = shape::v2::defaultCharacterDefinition();
    if (name == "ribcage") definition.torso.ribcageWidthScale = 1.15f;
    else if (name == "depth") definition.torso.chestDepthScale = 1.15f;
    else if (name == "waist") definition.torso.waistPinch = 1.0f;
    else if (name == "girdle") definition.torso.shoulderGirdleMass = 1.15f;
    else if (name == "clavicle") definition.torso.clavicleRelief = 0.03f;
    else if (name == "axilla") definition.torso.axillaDepth = 0.06f;
    else if (name == "leftwidth") definition.shoulders.leftWidthScale = 1.15f;
    else if (name == "deltoid") definition.shoulders.deltoidVolume = 1.60f;
    else if (name == "leftradius") definition.upperArms.leftRadiusScale = 1.40f;
    else if (name == "rightdelta") definition.asymmetry.rightUpperArmRadiusDelta = 0.40f;
    else if (name == "table-ribcage") definition.parameters.setValue("thorax-ribcage-volume", 1.15f);
    else if (name == "table-depth") definition.parameters.setValue("thorax-depth-ratio", 1.15f);
    else if (name == "table-waist") definition.parameters.setValue("waist-pinch", 1.0f);
    else if (name == "table-girdle") definition.parameters.setValue("shoulder-girdle-mass", 1.15f);
    else if (name == "table-axilla") definition.parameters.setValue("axilla-depth", 0.06f);
    else if (name == "acromion") {
        definition.landmarks.placed.push_back({"left-acromion", {0.30f, 1.40f, 0.0f}});
    }
    return definition;
}

TorsoShoulderResult generate(const shape::v2::CharacterDefinition& definition, uint32_t detail) {
    const auto request = makeGeometryBuildRequest(definition, detail, 2);
    return TorsoShoulderPatchGenerator::generate(definition, request);
}

uint64_t meshHash(const TorsoShoulderResult& result) {
    uint64_t hash = 1469598103934665603ull;
    const auto hashBytes = [&hash](const void* data, size_t size) {
        const auto* bytes = static_cast<const unsigned char*>(data);
        for (size_t i = 0; i < size; ++i) {
            hash ^= bytes[i];
            hash *= 1099511628211ull;
        }
    };
    for (const Vertex& vertex : result.mesh.vertices) {
        hashBytes(&vertex.position.x, sizeof(float) * 3);
        const uint16_t joints[4] = {vertex.joints.x, vertex.joints.y, vertex.joints.z, vertex.joints.w};
        const float weights[4] = {vertex.weights.x, vertex.weights.y, vertex.weights.z, vertex.weights.w};
        hashBytes(joints, sizeof(joints));
        hashBytes(weights, sizeof(weights));
    }
    hashBytes(result.mesh.indices.data(), sizeof(uint32_t) * result.mesh.indices.size());
    hashBytes(result.stableIds.data(), sizeof(uint64_t) * result.stableIds.size());
    return hash;
}

void compare(const std::string& name, const TorsoShoulderResult& base,
             const TorsoShoulderResult& changed) {
    float maxAll = 0.0f;
    float maxTorso = 0.0f;
    float maxLeft = 0.0f;
    float maxRight = 0.0f;
    const size_t count = std::min(base.mesh.vertices.size(), changed.mesh.vertices.size());
    for (size_t i = 0; i < count; ++i) {
        const float delta = (base.mesh.vertices[i].position - changed.mesh.vertices[i].position).length();
        maxAll = std::fmax(maxAll, delta);
        if (i < 352) maxTorso = std::fmax(maxTorso, delta);
        const RegionId region = changed.vertexRegions[i];
        if (region == RegionId::ShoulderGirdleLeft || region == RegionId::AxillaLeft ||
            region == RegionId::UpperArmRootLeft || region == RegionId::BridgeLeft) {
            maxLeft = std::fmax(maxLeft, delta);
        }
        if (region == RegionId::ShoulderGirdleRight || region == RegionId::AxillaRight ||
            region == RegionId::UpperArmRootRight || region == RegionId::BridgeRight) {
            maxRight = std::fmax(maxRight, delta);
        }
    }
    std::printf("DELTA %-10s verts=%zu tris=%zu max_all=%.6f max_torso=%.6f max_left=%.6f max_right=%.6f\n",
                name.c_str(), changed.mesh.vertices.size(), changed.mesh.indices.size() / 3,
                maxAll, maxTorso, maxLeft, maxRight);
}

} // namespace

int main() {
    const auto definition = shape::v2::defaultCharacterDefinition();
    const auto state = resolveCharacterState(definition);
    const auto& a = state.anthropometry;
    std::printf("ANTH stature=%.6f shoulder=%.6f biacromial=%.6f chest=%.6f chest_depth=%.6f waist=%.6f waist_depth=%.6f hip=%.6f upper_arm=%.6f\n",
                a.heightM, a.shoulderBreadthM, definition.proportions.biacromialM(), a.chestWidthM, a.chestDepthM,
                a.waistWidthM, a.waistDepthM, a.hipWidthM, a.upperArmLengthM);
    std::printf("JOINTS hips=%.6f spine=%.6f chest=%.6f neck=%.6f left_shoulder=(%.6f,%.6f,%.6f) left_elbow=(%.6f,%.6f,%.6f)\n",
                state.joints.hips.y, state.joints.spine.y, state.joints.chest.y, state.joints.neck.y,
                state.joints.leftShoulder.x, state.joints.leftShoulder.y, state.joints.leftShoulder.z,
                state.joints.leftElbow.x, state.joints.leftElbow.y, state.joints.leftElbow.z);
    for (const auto& point : state.landmarks.points) {
        std::printf("LANDMARK %-24s (%.6f,%.6f,%.6f) confidence=%s bone=%u\n",
                    point.name, point.position.x, point.position.y, point.position.z,
                    shape::v2::confidenceName(point.confidence), point.boneId);
    }

    const TorsoShoulderResult base = generate(definition, 1);
    std::printf("BASE valid=%d verts=%zu tris=%zu gen_ms=%.3f y_bounds=(%.6f,%.6f) mesh_fnv64=%016llx\n",
                base.valid, base.mesh.vertices.size(), base.mesh.indices.size() / 3,
                base.timings.totalMs, base.mesh.bounds.min.y, base.mesh.bounds.max.y,
                static_cast<unsigned long long>(meshHash(base)));
    for (const uint32_t ring : {0u, 3u, 5u, 6u, 8u, 10u}) {
        float minX = 1e9f, maxX = -1e9f, minZ = 1e9f, maxZ = -1e9f;
        float minY = 1e9f, maxY = -1e9f, sumY = 0.0f;
        for (uint32_t s = 0; s < 32; ++s) {
            const Vec3& p = base.mesh.vertices[ring * 32 + s].position;
            minX = std::fmin(minX, p.x); maxX = std::fmax(maxX, p.x);
            minZ = std::fmin(minZ, p.z); maxZ = std::fmax(maxZ, p.z);
            minY = std::fmin(minY, p.y); maxY = std::fmax(maxY, p.y); sumY += p.y;
        }
        std::printf("RING %u width=%.6f depth=%.6f y_min=%.6f y_max=%.6f y_mean=%.6f\n",
                    ring, maxX - minX, maxZ - minZ, minY, maxY, sumY / 32.0f);
    }
    std::printf("TOP_CAP center_index=353 position=(%.6f,%.6f,%.6f)\n",
                base.mesh.vertices[353].position.x, base.mesh.vertices[353].position.y,
                base.mesh.vertices[353].position.z);
    for (const std::string name : {"ribcage", "depth", "waist", "girdle", "clavicle", "axilla",
                                   "leftwidth", "deltoid", "leftradius", "rightdelta", "acromion",
                                   "table-ribcage", "table-depth", "table-waist", "table-girdle",
                                   "table-axilla"}) {
        compare(name, base, generate(variant(name), 1));
    }
    compare("detail0", base, generate(definition, 0));
    compare("detail2", base, generate(definition, 2));

    const auto requestBase = makeGeometryBuildRequest(definition, 1, 2);
    const auto requestClavicle = makeGeometryBuildRequest(variant("clavicle"), 1, 2);
    const auto requestRightRadius = makeGeometryBuildRequest(variant("rightdelta"), 1, 2);
    std::printf("CACHE base=%s clavicle=%s acromion_override=%s right_radius_delta=%s\n",
                requestBase.cacheKey().c_str(), requestClavicle.cacheKey().c_str(),
                requestRightRadius.cacheKey().c_str(),
                makeGeometryBuildRequest(variant("acromion"), 1, 2).cacheKey().c_str());
    return 0;
}
