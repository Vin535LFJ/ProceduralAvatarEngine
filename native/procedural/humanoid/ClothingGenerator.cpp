#include "ClothingGenerator.h"

#include "procedural/surfaces/ParametricSurfaces.h"

#include <algorithm>
#include <cmath>

namespace pae::clothing {

namespace {

constexpr float kPi = 3.14159265f;

std::vector<LoftSection> inflate(const std::vector<LoftSection>& sections,
                                 float thickness, float ease,
                                 float yMin, float yMax) {
    std::vector<LoftSection> out;
    for (const LoftSection& section : sections) {
        if (section.center.y < yMin || section.center.y > yMax) continue;
        LoftSection inflated = section;
        inflated.radiusX += thickness + ease;
        inflated.radiusZ += thickness + ease;
        out.push_back(inflated);
    }
    return out;
}

std::vector<SweepRing> inflate(const std::vector<SweepRing>& rings,
                               float thickness, float ease, float yMin, float yMax) {
    std::vector<SweepRing> out;
    for (const SweepRing& ring : rings) {
        if (ring.point.y < yMin || ring.point.y > yMax) continue;
        SweepRing inflated = ring;
        inflated.radius += thickness + ease;
        out.push_back(inflated);
    }
    return out;
}

} // namespace

std::vector<ClothingPiece> generateClothing(const BodyParameters& params,
                                            const BodyJoints& joints, float scale,
                                            const ClothingParams& cp) {
    std::vector<ClothingPiece> pieces;
    const float offset = cp.thickness + cp.ease;
    const float T = params.torsoLength * scale;

    // Shirt body: inflated torso shell from the hem up to the collar.
    // Sections above the hem are kept; a hem ring is extrapolated at hemY
    // (below the lowest section for a long shirt, above for a crop top).
    {
        const float hemY = joints.hips.y + T * cp.shirtHem;
        std::vector<LoftSection> body = computeTorsoSections(params, joints, scale);
        std::vector<LoftSection> shell;
        for (const LoftSection& section : body) {
            if (section.center.y < hemY) continue;
            LoftSection inflated = section;
            inflated.radiusX += offset;
            inflated.radiusZ += offset;
            shell.push_back(inflated);
        }
        if (shell.empty()) {
            // Hem below every station: extrapolate a hem ring from the lowest.
            LoftSection hem = body.front();
            hem.center.y = hemY;
            hem.radiusX += offset;
            hem.radiusZ += offset;
            shell.push_back(hem);
            for (const LoftSection& section : body) {
                LoftSection inflated = section;
                inflated.radiusX += offset;
                inflated.radiusZ += offset;
                shell.push_back(inflated);
            }
        } else {
            // Prepend a hem ring extrapolated from the lowest kept section.
            LoftSection hem = shell.front();
            hem.center.y = hemY;
            shell.insert(shell.begin(), hem);
        }
        if (shell.size() >= 2) {
            ClothingPiece piece;
            piece.materialId = 2;
            piece.mesh = generateSplineLoft(shell, 24u, 4);
            piece.path = {joints.hips, joints.spine, joints.chest, joints.neck};
            piece.pathBones = {BoneHips, BoneSpine, BoneChest, BoneNeck};
            pieces.push_back(std::move(piece));
        }
    }

    // Collar: modest neck band. The torso shell now carries the shoulder
    // taper, so an oversized ring is neither needed nor visually plausible.
    {
        const float collarY = joints.neck.y - 0.010f * scale;
        const float neckRadius = params.neckLength * params.neckShape.radiusFactor
                               * scale + offset * 0.9f;
        std::vector<SweepRing> rings = {
            {{0.0f, collarY - 0.030f * scale, 0.0f}, neckRadius * 1.22f, {1.0f, 0.92f}, 0.0f},
            {{0.0f, collarY + 0.006f * scale, 0.0f}, neckRadius * 1.12f, {1.0f, 1.0f}, 0.0f},
            {{0.0f, collarY + 0.026f * scale, 0.0f}, neckRadius * 1.05f, {1.0f, 1.0f}, 0.0f},
        };
        ClothingPiece piece;
        piece.materialId = 2;
        piece.mesh = generateSplineSweep(rings, 24u, 3, false, false, false);
        piece.path = {joints.neck, joints.chest};
        piece.pathBones = {BoneNeck, BoneChest};
        pieces.push_back(std::move(piece));
    }

    // Sleeves: inflated arm rings over the upper arm.
    for (int side = -1; side <= 1; side += 2) {
        const bool left = side > 0;
        const std::vector<SweepRing> armRings = computeArmRings(params, joints, scale, left);
        const float cutY = joints.leftShoulder.y - (joints.leftShoulder.y - joints.leftElbow.y) * cp.sleeveLength;
        std::vector<SweepRing> sleeve = inflate(armRings, cp.thickness, cp.ease,
                                                cutY, joints.leftShoulder.y + 0.02f * scale);
        if (sleeve.size() < 2) continue;
        ClothingPiece piece;
        piece.materialId = 2;
        piece.mesh = generateSplineSweep(sleeve, 20u, 3, false, false, false);
        piece.path = {left ? joints.leftShoulder : joints.rightShoulder,
                      left ? joints.leftElbow : joints.rightElbow};
        piece.pathBones = {left ? BoneLeftUpperArm : BoneRightUpperArm,
                           left ? BoneLeftUpperArm : BoneRightUpperArm};
        pieces.push_back(std::move(piece));
    }

    // Pants: pelvis shell + per-leg tubes down to pantsLength.
    {
        const float hipBottomY = joints.hips.y - 0.10f * T;
        std::vector<LoftSection> body = computeTorsoSections(params, joints, scale);
        std::vector<LoftSection> pelvis = inflate(body, cp.thickness, cp.ease,
                                                  hipBottomY, joints.hips.y + 0.25f * T);
        if (pelvis.size() >= 2) {
            ClothingPiece piece;
            piece.materialId = 5;
            piece.mesh = generateSplineLoft(pelvis, 24u, 4);
            piece.path = {joints.hips, joints.spine};
            piece.pathBones = {BoneHips, BoneSpine};
            pieces.push_back(std::move(piece));
        }
        for (int side = -1; side <= 1; side += 2) {
            const bool left = side > 0;
            const std::vector<SweepRing> legRings = computeLegRings(params, joints, scale, left);
            const float kneeY = (left ? joints.leftKnee : joints.rightKnee).y;
            const float ankleY = (left ? joints.leftAnkle : joints.rightAnkle).y;
            const float cutY = kneeY - (kneeY - ankleY) * (cp.pantsLength * 2.0f - 1.0f);
            std::vector<SweepRing> tube = inflate(legRings, cp.thickness, cp.ease,
                                                  cutY, joints.hips.y + 0.2f * T);
            if (tube.size() < 2) continue;
            ClothingPiece piece;
            piece.materialId = 5;
            piece.mesh = generateSplineSweep(tube, 20u, 3, false, false, false);
            piece.path = {left ? joints.leftHip : joints.rightHip,
                          left ? joints.leftKnee : joints.rightKnee,
                          left ? joints.leftAnkle : joints.rightAnkle};
            piece.pathBones = {left ? BoneLeftUpperLeg : BoneRightUpperLeg,
                               left ? BoneLeftLowerLeg : BoneRightLowerLeg,
                               left ? BoneLeftFoot : BoneRightFoot};
            pieces.push_back(std::move(piece));
        }
    }
    return pieces;
}

} // namespace pae::clothing
