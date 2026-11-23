#pragma once

// G2 (SPEC 11 / 11.1): skinning data contract and CPU-side deformation.
//
// SPEC 11.1 assigns G2 exactly two parity responsibilities:
//   - weight legality:  <= 4 influences, non-negative, sum = 1 +/- 1e-3,
//                       bone IDs in [0, 21];
//   - CPU-side deformation correctness.
// GPU numerical parity is explicitly NOT required at G2 and remains
// [UNVERIFIED] - that is not a kill condition.
//
// The anatomy weight assignment below mirrors the production formula in
// HumanAnatomyGenerator.cpp so the PoC route meshes (R2 / R3) can carry the
// same semantic weights as R1. It is verified against R1's own production
// weights by a unit test rather than being trusted by inspection.

#include "procedural/MeshData.h"
#include "procedural/humanoid/anatomy/AnatomyField.h"
#include "skeleton/Skeleton.h"

namespace pae::shape {

struct WeightLegality {
    int vertexCount = 0;
    int maxInfluences = 0;             // most non-zero weights on one vertex
    int overFourInfluenceVertices = 0;
    int negativeWeightVertices = 0;
    int sumToleranceViolations = 0;
    int invalidBoneIdVertices = 0;
    int zeroWeightVertices = 0;
    double maxSumError = 0.0;
    bool valid = false;
};

WeightLegality checkWeightLegality(const MeshData& mesh, double sumTolerance = 1e-3);

// Assigns semantic anatomy weights to every vertex of a mesh using the same
// formula as the production generator: the owning region field selects the
// primary bone, the nearest upper-body bone segment selects the secondary,
// and the two inverse-square scores are renormalised.
void assignAnatomyWeights(MeshData& mesh,
                          const anatomy::HumanAnatomyParameters& params,
                          const BodyJoints& joints,
                          const Skeleton& skeleton);

struct CpuDeformationReport {
    int vertexCount = 0;
    int nonFinitePositions = 0;
    int rigidVertexCount = 0;
    double maxRigidError = 0.0;       // synthetic weight-1 probe vs palette[bone]
    double bindResidual = 0.0;        // bind-pose palette must be the identity
    double rootResidual = 0.0;        // movement of a root-weighted vertex
    double maxDisplacement = 0.0;
    double meanDisplacement = 0.0;
    double volumeBefore = 0.0;
    double volumeAfter = 0.0;
    double relativeVolumeChange = 0.0;
    bool valid = false;
};

// Applies a deterministic test pose (chest + left shoulder rotations) with the
// CPU reference skinner and checks that rigidly weighted vertices follow
// their bone exactly, root-weighted vertices stay fixed, and the result is
// finite and volume-plausible.
CpuDeformationReport verifyCpuDeformation(const MeshData& mesh, const BodyJoints& joints);

} // namespace pae::shape
