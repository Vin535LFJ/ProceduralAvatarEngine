#include "AnatomicalLandmarks.h"

namespace pae::character {
namespace {

void add(std::vector<LandmarkPoint>& points, LandmarkId id, const char* name,
         const Vec3& position, LandmarkSide side, uint16_t boneId,
         shape::v2::ValueConfidence confidence = shape::v2::ValueConfidence::Derived,
         const char* provenance = "derived") {
    points.push_back({id, name, position, side, confidence, provenance, boneId});
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
    AnatomicalLandmarkSet set;
    set.points.reserve(static_cast<size_t>(LandmarkId::Count));

    const float yBottom = joints.hips.y + 0.010f;
    const float yNeck = joints.neck.y;
    const auto targetY = [yBottom, yNeck](float s) {
        return yBottom + (yNeck - yBottom) * s;
    };
    const bool r2 = definition.schemaVersion >= 3;
    const float costalY = r2 ? targetY(0.62f) : joints.spine.y;
    const float chestY = r2 ? targetY(0.80f) : joints.chest.y;

    add(set.points, LandmarkId::HipCenter, "hip-center",
        {0.0f, joints.hips.y, 0.0f}, LandmarkSide::Center, BoneHips);
    add(set.points, LandmarkId::WaistCenter, "waist-center",
        {0.0f, r2 ? targetY(0.44f) : (joints.hips.y + joints.spine.y) * 0.5f, 0.0f},
        LandmarkSide::Center, BoneSpine);
    add(set.points, LandmarkId::CostalMargin, "costal-margin",
        {0.0f, costalY, r2 ? 0.122f : anthropometry.chestDepthM * 0.28f},
        LandmarkSide::Center, BoneSpine);
    add(set.points, LandmarkId::ChestCenter, "chest-center",
        {0.0f, chestY, r2 ? 0.142f : anthropometry.chestDepthM * 0.34f},
        LandmarkSide::Center, BoneChest);
    add(set.points, LandmarkId::NeckBase, "neck-base",
        {0.0f, joints.neck.y, 0.0f}, LandmarkSide::Center, BoneChest);
    add(set.points, LandmarkId::IliacCrestCenter, "iliac-crest",
        {0.0f, targetY(0.16f), 0.126f}, LandmarkSide::Center, BoneHips,
        shape::v2::ValueConfidence::Assumed, "R2-target");
    add(set.points, LandmarkId::Umbilicus, "umbilicus",
        {0.0f, targetY(0.50f), 0.114f}, LandmarkSide::Center, BoneSpine,
        shape::v2::ValueConfidence::Assumed, "R2-target");
    add(set.points, LandmarkId::LowerRibCageCenter, "lower-rib-cage",
        {0.0f, targetY(0.72f), 0.137f}, LandmarkSide::Center, BoneChest,
        shape::v2::ValueConfidence::Assumed, "R2-target");
    add(set.points, LandmarkId::UpperRibCageCenter, "upper-rib-cage",
        {0.0f, targetY(0.88f), 0.136f}, LandmarkSide::Center, BoneChest,
        shape::v2::ValueConfidence::Assumed, "R2-target");
    add(set.points, LandmarkId::JugularNotch, "jugular-notch",
        {0.0f, targetY(0.96f), 0.060f}, LandmarkSide::Center, BoneChest,
        shape::v2::ValueConfidence::Assumed, "R2-target");
    add(set.points, LandmarkId::SternalAngle, "sternal-angle",
        {0.0f, targetY(0.92f), 0.070f}, LandmarkSide::Center, BoneChest,
        shape::v2::ValueConfidence::Assumed, "R2-target");
    add(set.points, LandmarkId::NeckBaseInterface, "neck-base-interface",
        {0.0f, yNeck, 0.0f}, LandmarkSide::Center, BoneChest,
        shape::v2::ValueConfidence::Assumed, "R2-target");

    for (int sideIndex = 1; sideIndex <= 2; ++sideIndex) {
        const bool left = sideIndex == 1;
        const float side = left ? 1.0f : -1.0f;
        const Vec3 shoulder = left ? joints.leftShoulder : joints.rightShoulder;
        const Vec3 elbow = left ? joints.leftElbow : joints.rightElbow;
        const uint16_t shoulderBone = left ? BoneLeftShoulder : BoneRightShoulder;
        const uint16_t upperArmBone = left ? BoneLeftUpperArm : BoneRightUpperArm;
        const LandmarkSide landmarkSide = left ? LandmarkSide::Left : LandmarkSide::Right;
        const float acromionX = r2 ? side * definition.shoulders.biacromialWidth * 0.5f
                                   : shoulder.x + side * 0.022f;
        const float acromionY = r2 ? definition.shoulders.acromionY
                                   : shoulder.y + 0.012f;
        const float rootX = r2 ? side * (definition.shoulders.externalWidth * 0.5f - 0.005f)
                               : shoulder.x + side * 0.026f;
        const float rootY = r2 ? definition.shoulders.rootY : shoulder.y - 0.055f;
        const float axillaX = r2 ? side * 0.160f : shoulder.x + side * 0.026f;
        const float axillaY = r2 ? 1.285f : shoulder.y - 0.055f;

        add(set.points, left ? LandmarkId::LeftAcromion : LandmarkId::RightAcromion,
            left ? "left-acromion" : "right-acromion",
            {acromionX, acromionY, 0.0f}, landmarkSide, shoulderBone,
            r2 ? shape::v2::ValueConfidence::Assumed : shape::v2::ValueConfidence::Derived,
            r2 ? "R2-target" : "derived");
        add(set.points, left ? LandmarkId::LeftShoulderJoint : LandmarkId::RightShoulderJoint,
            left ? "left-shoulder-joint" : "right-shoulder-joint",
            shoulder, landmarkSide, shoulderBone);
        add(set.points, left ? LandmarkId::LeftAxilla : LandmarkId::RightAxilla,
            left ? "left-axilla" : "right-axilla",
            {axillaX, axillaY, r2 ? 0.005f : 0.004f}, landmarkSide, upperArmBone,
            r2 ? shape::v2::ValueConfidence::Assumed : shape::v2::ValueConfidence::Derived,
            r2 ? "R2-target" : "derived");
        add(set.points, left ? LandmarkId::LeftElbow : LandmarkId::RightElbow,
            left ? "left-elbow" : "right-elbow", elbow, landmarkSide, upperArmBone);
        add(set.points, left ? LandmarkId::LeftClavicleMedial : LandmarkId::RightClavicleMedial,
            left ? "left-clavicle-medial" : "right-clavicle-medial",
            {side * 0.025f, 1.438f, 0.060f}, landmarkSide, shoulderBone,
            shape::v2::ValueConfidence::Assumed, "R2-target");
        add(set.points, left ? LandmarkId::LeftClavicleLateral : LandmarkId::RightClavicleLateral,
            left ? "left-clavicle-lateral" : "right-clavicle-lateral",
            {side * 0.196f, 1.414f, 0.030f}, landmarkSide, shoulderBone,
            shape::v2::ValueConfidence::Assumed, "R2-target");
        add(set.points, left ? LandmarkId::LeftScapulaMedial : LandmarkId::RightScapulaMedial,
            left ? "left-scapula-medial" : "right-scapula-medial",
            {side * 0.055f, 1.350f, -0.105f}, landmarkSide, shoulderBone,
            shape::v2::ValueConfidence::Assumed, "R2-target");
        add(set.points, left ? LandmarkId::LeftScapulaLateral : LandmarkId::RightScapulaLateral,
            left ? "left-scapula-lateral" : "right-scapula-lateral",
            {side * 0.150f, 1.320f, -0.095f}, landmarkSide, shoulderBone,
            shape::v2::ValueConfidence::Assumed, "R2-target");
        add(set.points, left ? LandmarkId::LeftTrapezius : LandmarkId::RightTrapezius,
            left ? "left-trapezius" : "right-trapezius",
            {side * 0.120f, 1.420f, -0.070f}, landmarkSide, shoulderBone,
            shape::v2::ValueConfidence::Assumed, "R2-target");
        add(set.points, left ? LandmarkId::LeftUpperArmRoot : LandmarkId::RightUpperArmRoot,
            left ? "left-upper-arm-root" : "right-upper-arm-root",
            {rootX, rootY, 0.0f}, landmarkSide, upperArmBone,
            r2 ? shape::v2::ValueConfidence::Assumed : shape::v2::ValueConfidence::Derived,
            r2 ? "R2-target" : "derived");
        add(set.points, left ? LandmarkId::LeftDeltoidSurface : LandmarkId::RightDeltoidSurface,
            left ? "left-deltoid-surface" : "right-deltoid-surface",
            {side * definition.shoulders.externalWidth * 0.5f,
             definition.shoulders.deltoidSurfaceY, 0.0f}, landmarkSide, upperArmBone,
            shape::v2::ValueConfidence::Assumed, "R2-target");
    }

    size_t explicitOverrides = 0;
    for (LandmarkPoint& landmark : set.points) {
        for (const shape::v2::LandmarkPoint& explicitPoint : definition.landmarks.placed) {
            if (explicitPoint.name == landmark.name) {
                landmark.position = explicitPoint.position;
                landmark.confidence = explicitPoint.confidence;
                landmark.provenance = explicitPoint.source;
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
