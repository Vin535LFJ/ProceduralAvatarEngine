#pragma once

// PoC-SM (AUTH-08 AL-2): the CharacterDefinition five-layer contract.
//
// ADR-011 (ACCEPTED 2026-11-24) approves the EXISTENCE AND SEMANTICS of five
// sub-contracts and nothing else:
//
//   1. ProportionProfile      - body ratios
//   2. AnatomicalLandmarks    - bone joints, surface landmarks, curves
//   3. AnatomicalRegions      - RegionId + adjacency
//   4. ShapeParameters        - volume / profile / stylisation parameters
//   5. JointDeformationRegions - INTERFACE ONLY (no deformation in PoC-SM)
//
// What this header must NOT become:
//
//   - a place to hard-code a body type. Every default that comes from a
//     measurement states its source and its confidence; every value that is
//     an assumption states that it is an assumption.
//   - a place to smuggle in a second notion of shape. The G2 failure was
//     precisely that R1/R2/R3 each carried their own implicit body; this
//     contract is the single upstream description they must all resolve to.
//
// FROZEN BOUNDARIES (ADR-011 F-1..F-6) - this file touches none of them:
//   BoneId 0-21, UBO[22], 64B GPU vertex packing, JNI ABI.
//
// This is host-only. It is NOT compiled into the Android production build.

#include "core/math/Math.h"

#include <cstdint>
#include <string>
#include <vector>

namespace pae {
namespace shape {
namespace v2 {

// ---------------------------------------------------------------------------
// Confidence of a value. Mirrors the evidence labels the project uses in its
// reports so that a number in code and a number in a report cannot drift
// apart silently.
// ---------------------------------------------------------------------------
enum class ValueConfidence : int {
    // Measured from a first-party anthropometric source.
    Measured = 0,
    // Derived by arithmetic from Measured values only (no assumption added).
    Derived = 1,
    // An assumption chosen by us; the number is only as good as the assumption.
    Assumed = 2,
    // Not yet determined. Must be filled before the value may gate anything.
    Undetermined = 3
};

const char* confidenceName(ValueConfidence c);

// ---------------------------------------------------------------------------
// 1. ProportionProfile
//
// Body ratios, all expressed as dimensionless fractions of stature so the
// profile can be rescaled to any height without re-deriving anything.
// ---------------------------------------------------------------------------
struct ProportionProfile {
    // Stature, metres. Source: SRC-ANTH-01 (ANSUR II) male 50th percentile.
    float statureM = 1.7549f;

    // Bideltoid shoulder breadth / stature. Derived from ANSUR II #17.
    float shoulderBreadthRatio = 0.290f;
    // Biacromial shoulder breadth / stature. Derived from ANSUR II #18.
    float biacromialRatio = 0.237f;
    // Hip breadth / stature. Derived from ANSUR II #19.
    float hipBreadthRatio = 0.196f;
    // Chest depth / stature. Derived from ANSUR II #20.
    float chestDepthRatio = 0.144f;
    // Abdominal depth / stature. Derived from ANSUR II #21 (PROXY: abdominal
    // depth is not pelvic depth - see the contract note below).
    float abdominalDepthRatio = 0.143f;

    // Head height / stature. Assumed: the head-to-body ratio is not in either
    // anthropometric source used, so it is an explicit assumption.
    float headHeightRatio = 0.125f;

    // Waist circumference / stature. Source: SRC-ANTH-02 (CDC NHANES SR50),
    // male 20-29 median.
    float waistCircumferenceRatio = 0.5325f;

    // The waist cross-section is solved from ONE equation (circumference) in
    // TWO unknowns (half width, half depth). This is the shape assumption
    // that closes the system. It is ASSUMED, not measured.
    float waistAspectRatio = 1.25f;

    // P1-VR segment length ratios (metres per metre of stature). These are
    // explicit Assumed inputs until the Canonical Acceptance Sheet fixes the
    // measured adult values.
    float upperArmLengthRatio = 0.185f;
    float forearmLengthRatio = 0.168f;
    float upperLegLengthRatio = 0.259f;
    float lowerLegLengthRatio = 0.254f;
    float handLengthRatio = 0.108f;
    float footLengthRatio = 0.152f;

    float stature() const { return statureM; }
    float shoulderBreadthM() const { return statureM * shoulderBreadthRatio; }
    float biacromialM() const { return statureM * biacromialRatio; }
    float hipBreadthM() const { return statureM * hipBreadthRatio; }
    float chestDepthM() const { return statureM * chestDepthRatio; }
    float abdominalDepthM() const { return statureM * abdominalDepthRatio; }
    float headHeightM() const { return statureM * headHeightRatio; }
    float waistCircumferenceM() const { return statureM * waistCircumferenceRatio; }

    // Half axes of the waist section, solved from the circumference and the
    // assumed aspect ratio (see waistAspectRatio).
    void waistHalfAxes(float* halfWidthM, float* halfDepthM) const;

    // Chest depth is measured; chest breadth is NOT in the sources, so it is
    // derived from the waist-to-chest relationship and stays Assumed.
    float chestHalfWidthM() const;
    float chestHalfDepthM() const { return 0.5f * chestDepthM(); }
};

// ---------------------------------------------------------------------------
// 2. AnatomicalLandmarks
//
// Landmark names follow international standard anatomical nomenclature so
// they can be mapped onto FMA concept ids (BodyParts3D). The PoC-SM slice
// only needs the torso set; the rest of the catalogue is declared, not
// positioned.
// ---------------------------------------------------------------------------
enum class LandmarkKind : int {
    BoneJoint = 0,   // a skeletal joint position
    Surface = 1,     // a palpable surface landmark
    Curve = 2        // a named anatomical curve (polyline of surface points)
};

struct LandmarkDecl {
    std::string name;         // international anatomical name
    LandmarkKind kind = LandmarkKind::Surface;
    bool positioned = false;  // false = declared but not placed in this project
    ValueConfidence confidence = ValueConfidence::Undetermined;
};

struct LandmarkPoint {
    std::string name;
    Vec3 position{};  // metres, project coordinates
};

struct AnatomicalLandmarks {
    std::vector<LandmarkDecl> catalogue;   // the full declared set
    std::vector<LandmarkPoint> placed;     // the subset placed in this project
};

// The standard torso landmark catalogue used by the PoC-SM slice.
const std::vector<LandmarkDecl>& standardLandmarkCatalogue();

// ---------------------------------------------------------------------------
// 3. AnatomicalRegions
//
// RegionId is the vocabulary that runs through Shape Model -> Topology ->
// Skinning. A region is a semantic area of the body surface; it carries no
// geometry of its own.
// ---------------------------------------------------------------------------
enum class RegionId : int {
    Thorax = 0,
    Abdomen,
    Pelvis,
    ShoulderGirdle,
    Axilla,
    Neck,
    RegionCount
};

const char* regionName(RegionId id);

struct RegionAdjacency {
    RegionId a = RegionId::Thorax;
    RegionId b = RegionId::Thorax;
    float sharedWeight = 0.0f;  // 0 = no shared influence
};

struct AnatomicalRegions {
    std::vector<RegionId> regions;
    std::vector<RegionAdjacency> adjacency;

    // Regions whose influence extends to the given height, used to decide
    // how the shared envelope is assembled.
    bool contains(RegionId id) const;
    const RegionAdjacency* adjacencyBetween(RegionId a, RegionId b) const;
};

// ---------------------------------------------------------------------------
// 4. ShapeParameters
//
// Every parameter has ONE semantic and an explicit list of the regions it
// affects. The affected-region list is what makes SPEC 13 #9 (cross-region
// parameter impact) measurable rather than a comment.
//
// This mirrors the G1 ParameterMeta contract (shape/ShapeModel.h) but is
// independent of it: the G1 table describes the LEGACY parameters, this one
// describes the v2 contract.
// ---------------------------------------------------------------------------
enum class ParameterStatus : int {
    // Consumed by the v2 generation chain; a measurable mesh delta exists.
    Wired = 0,
    // Declared in the contract, not yet consumed.
    DeclaredOnly = 1,
    // Part of the contract, no backend yet.
    NotImplemented = 2
};

struct ShapeParameterDecl {
    std::string name;
    std::string unit;
    std::string semantic;  // ONE sentence: what this parameter changes
    float minValue = 0.0f;
    float maxValue = 0.0f;
    float defaultValue = 0.0f;
    std::vector<RegionId> affectedRegions;
    ParameterStatus status = ParameterStatus::DeclaredOnly;
    ValueConfidence confidence = ValueConfidence::Assumed;
};

struct ShapeParameters {
    std::vector<ShapeParameterDecl> declarations;
    std::vector<float> values;  // parallel to declarations

    const ShapeParameterDecl* find(const std::string& name) const;
    float value(const std::string& name, float fallback = 0.0f) const;
    void setValue(const std::string& name, float value);
};

// ---------------------------------------------------------------------------
// 5. JointDeformationRegions - INTERFACE ONLY
//
// ADR-011 approves the CONTRACT of this layer and explicitly NOT its
// implementation: "JointDeformationRegions | ... | 接口声明（PoC-SM 只声明，
// 不实现变形）". So this is a declaration with no evaluator. There is
// deliberately no function that applies it.
// ---------------------------------------------------------------------------
enum class DeformSide : int {
    Compress = 0,
    Stretch = 1
};

struct JointDeformationRegion {
    uint16_t boneId = 0;              // within the frozen BoneId 0-21 range
    std::string name;
    float influenceBandM = 0.0f;      // radius of the influence band
    DeformSide side = DeformSide::Compress;
    uint32_t boneMask = 0;            // which bones the band may affect
    bool volumePreserving = true;     // volume-preservation entry point
    bool hasCorrective = false;       // corrective-shape entry point
};

// ---------------------------------------------------------------------------
// P1-VR-1A torso/shoulder parameters. These are semantic shape inputs, not a
// second body definition. Values are normalized unless stated in metres.
// ---------------------------------------------------------------------------
struct TorsoShapeParameters {
    float ribcageWidthScale = 1.0f;
    float chestDepthScale = 1.0f;
    float waistPinch = 0.45f;
    float upperPelvisWidthScale = 1.0f;
    float shoulderGirdleMass = 1.0f;
    float axillaDepth = 0.028f;
    float clavicleRelief = 0.010f;
};

struct ShoulderShapeParameters {
    float leftWidthScale = 1.0f;
    float rightWidthScale = 1.0f;
    float deltoidVolume = 1.0f;
};

struct UpperArmShapeParameters {
    float leftRadiusScale = 1.0f;
    float rightRadiusScale = 1.0f;
    float elbowRadiusScale = 0.72f;
};

struct AsymmetryParameters {
    float leftShoulderWidthDelta = 0.0f;
    float rightShoulderWidthDelta = 0.0f;
    float leftUpperArmRadiusDelta = 0.0f;
    float rightUpperArmRadiusDelta = 0.0f;
};

// ---------------------------------------------------------------------------
// The contract itself.
// ---------------------------------------------------------------------------
struct CharacterDefinition {
    uint32_t schemaVersion = 2;
    uint64_t seed = 20261125;
    ProportionProfile proportions{};
    AnatomicalLandmarks landmarks{};
    AnatomicalRegions regions{};
    ShapeParameters parameters{};
    TorsoShapeParameters torso{};
    ShoulderShapeParameters shoulders{};
    UpperArmShapeParameters upperArms{};
    AsymmetryParameters asymmetry{};
    std::vector<JointDeformationRegion> jointDeformations{};
};

// Builds the default definition, grounded in the measured anthropometric
// values recorded in the Canonical Acceptance Sheet v0. Every value carries
// its confidence; nothing here is a number picked to look right.
CharacterDefinition defaultCharacterDefinition();

// The full parameter table of the default definition.
const std::vector<ShapeParameterDecl>& defaultShapeParameterTable();

} // namespace v2
} // namespace shape
} // namespace pae
