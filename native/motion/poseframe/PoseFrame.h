#pragma once

#include "core/math/Math.h"

#include <array>
#include <cstdint>

namespace pae {

enum class CoordinateSpace : uint8_t {
    NormalizedImage = 0,
    PixelImage = 1,
    ProviderWorld = 2,
    AvatarLocal = 3,
    EngineWorld = 4
};

enum LandmarkId : uint16_t {
    LkNose = 0,
    LkLeftShoulder = 11,
    LkRightShoulder = 12,
    LkLeftElbow = 13,
    LkRightElbow = 14,
    LkLeftWrist = 15,
    LkRightWrist = 16,
    LkLeftHip = 23,
    LkRightHip = 24,
    LkLeftKnee = 25,
    LkRightKnee = 26,
    LkLeftAnkle = 27,
    LkRightAnkle = 28,
    LkLeftHeel = 29,
    LkRightHeel = 30,
    LkLeftFootIndex = 31,
    LkRightFootIndex = 32,
    LandmarkCount = 33
};

struct PoseLandmark {
    uint16_t id = 0;
    Vec3 position{};
    float visibility = 1.0f;
    float presence = 1.0f;
    bool valid = false;
};

struct PoseFrame {
    int64_t timestampNs = 0;
    CoordinateSpace coordinateSpace = CoordinateSpace::NormalizedImage;
    uint32_t landmarkCount = 0;
    std::array<PoseLandmark, LandmarkCount> landmarks{};
    float poseConfidence = 0.0f;
    bool hasPerson = false;
    uint32_t imageWidth = 0;
    uint32_t imageHeight = 0;
    int rotationDegrees = 0;
    bool mirrored = false;
    uint64_t revision = 0;

    const PoseLandmark* find(uint16_t id) const {
        for (uint32_t i = 0; i < landmarkCount; ++i) {
            if (landmarks[i].id == id && landmarks[i].valid) return &landmarks[i];
        }
        return nullptr;
    }
};

} // namespace pae
