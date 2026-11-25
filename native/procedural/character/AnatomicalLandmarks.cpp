#include "AnatomicalLandmarks.h"

namespace pae::character {
namespace {

void add(std::vector<LandmarkPoint>& points, LandmarkId id, const char* name,
         const Vec3& position, LandmarkSide side, uint16_t boneId,
         shape::v2::ValueConfidence confidence = shape::v2::ValueConfidence::Derived) {
    points.push_back({id, name, position, side, confidence, boneId});
}

} // namespace

const LandmarkPoint* AnatomicalLandmarkSet::find(LandmarkId id) const {
    for (const LandmarkPoint& point : points) {
        if (point.id == id) return &point;
    }
    return nullptr;
}

AnatomicalLandmarkSet resolveLandmarks(const shape::v2::CharacterDefinition& definition,
                                       const Anthropometry& anthropometry,
                                       const BodyJoints& joints,
                                       bool usedLegacyJoints,
                                       LandmarkResolverReport* report) {
    (void)definition;
    AnatomicalLandmarkSet set;
    set.points.reserve(static_cast<size_t>(LandmarkId::Count));

    const float chestY = joints.chest.y;
    const float costalY = joints.spine.y;
    add(set.points, LandmarkId::HipCenter, "hip-center",
        {0.0f, joints.hips.y, 0.0f}, LandmarkSide::Center, BoneHips);
    add(set.points, LandmarkId::WaistCenter, "waist-center",
        {0.0f, (joints.hips.y + costalY) * 0.5f, 0.0f}, LandmarkSide::Center, BoneSpine);
    add(set.points, LandmarkId::CostalMargin, "costal-margin",
        {0.0f, costalY, anthropometry.chestDepthM * 0.28f}, LandmarkSide::Center, BoneSpine);
    add(set.points, LandmarkId::ChestCenter, "chest-center",
        {0.0f, chestY, anthropometry.chestDepthM * 0.34f}, LandmarkSide::Center, BoneChest);
    add(set.points, LandmarkId::NeckBase, "neck-base",
        {0.0f, joints.neck.y, 0.0f}, LandmarkSide::Center, BoneChest);

    for (int sideIndex = 1; sideIndex <= 2; ++sideIndex) {
        const bool left = sideIndex == 1;
        const float side = left ? 1.0f : -1.0f;
        const Vec3 shoulder = left ? joints.leftShoulder : joints.rightShoulder;
        const Vec3 elbow = left ? joints.leftElbow : joints.rightElbow;
        const uint16_t shoulderBone = left ? BoneLeftShoulder : BoneRightShoulder;
        const uint16_t upperArmBone = left ? BoneLeftUpperArm : BoneRightUpperArm;
        const LandmarkSide landmarkSide = left ? LandmarkSide::Left : LandmarkSide::Right;

        add(set.points, left ? LandmarkId::LeftAcromion : LandmarkId::RightAcromion,
            left ? "left-acromion" : "right-acromion",
            shoulder + Vec3{side * 0.022f, 0.012f, 0.0f}, landmarkSide, shoulderBone);
        add(set.points, left ? LandmarkId::LeftShoulderJoint : LandmarkId::RightShoulderJoint,
            left ? "left-shoulder-joint" : "right-shoulder-joint",
            shoulder, landmarkSide, shoulderBone);
        add(set.points, left ? LandmarkId::LeftAxilla : LandmarkId::RightAxilla,
            left ? "left-axilla" : "right-axilla",
            shoulder + Vec3{side * 0.026f, -0.055f, 0.004f}, landmarkSide, upperArmBone);
        add(set.points, left ? LandmarkId::LeftElbow : LandmarkId::RightElbow,
            left ? "left-elbow" : "right-elbow",
            elbow, landmarkSide, upperArmBone);
    }

    // Explicit landmarks have highest priority. The catalogue name is the
    // stable key; resolved positions never write back to CharacterDefinition.
    size_t explicitOverrides = 0;
    for (LandmarkPoint& landmark : set.points) {
        for (const shape::v2::LandmarkPoint& explicitPoint : definition.landmarks.placed) {
            if (explicitPoint.name == landmark.name) {
                landmark.position = explicitPoint.position;
                landmark.confidence = shape::v2::ValueConfidence::Measured;
                ++explicitOverrides;
                break;
            }
        }
    }

    if (report) {
        report->valid = set.points.size() == static_cast<size_t>(LandmarkId::Count);
        report->usedLegacyJoints = usedLegacyJoints;
        if (usedLegacyJoints) {
            report->notes.push_back("landmarks resolved from legacy BodyJoints adapter");
        }
        if (explicitOverrides > 0) {
            report->notes.push_back("explicit landmark overrides applied: " +
                                    std::to_string(explicitOverrides));
        }
        if (!report->valid) report->notes.push_back("landmark catalogue incomplete");
    }
    return set;
}

} // namespace pae::character