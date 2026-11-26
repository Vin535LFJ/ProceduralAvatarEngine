#include "procedural/shape/v2/CharacterDefinition.h"

#include <algorithm>
#include <cmath>

namespace pae {
namespace shape {
namespace v2 {

namespace {

constexpr float kPi = 3.14159265358979323846f;

std::vector<TorsoControlStation> defaultTorsoControlStations() {
    auto station = [](const char* id, float height, float centerZ, float lateralX,
                      float frontZ, float backZ, float diagonalX,
                      float frontDiagonalZ, float backDiagonalZ) {
        TorsoControlStation value;
        value.id = id;
        value.height = height;
        value.controls.left = {lateralX, 0.0f, centerZ};
        value.controls.right = {-lateralX, 0.0f, centerZ};
        value.controls.front = {0.0f, 0.0f, frontZ};
        value.controls.back = {0.0f, 0.0f, backZ};
        value.controls.frontLeft = {diagonalX, 0.0f, frontDiagonalZ};
        value.controls.frontRight = {-diagonalX, 0.0f, frontDiagonalZ};
        value.controls.backLeft = {diagonalX, 0.0f, backDiagonalZ};
        value.controls.backRight = {-diagonalX, 0.0f, backDiagonalZ};
        return value;
    };
    return {
        station("upper-pelvis", 0.00f, 0.000f, 0.166f, 0.128f, -0.122f, 0.150f, 0.098f, -0.092f),
        station("iliac-crest", 0.16f, -0.002f, 0.174f, 0.126f, -0.122f, 0.156f, 0.096f, -0.092f),
        station("lower-abdomen", 0.32f, 0.004f, 0.148f, 0.114f, -0.104f, 0.132f, 0.088f, -0.078f),
        station("waist", 0.44f, 0.002f, 0.134f, 0.108f, -0.096f, 0.118f, 0.082f, -0.070f),
        station("umbilicus", 0.50f, 0.006f, 0.140f, 0.114f, -0.100f, 0.124f, 0.086f, -0.074f),
        station("costal-margin", 0.62f, 0.004f, 0.154f, 0.122f, -0.108f, 0.136f, 0.092f, -0.080f),
        station("lower-rib-cage", 0.72f, 0.006f, 0.168f, 0.137f, -0.116f, 0.150f, 0.104f, -0.088f),
        station("chest", 0.80f, 0.012f, 0.174f, 0.142f, -0.118f, 0.156f, 0.112f, -0.098f),
        station("upper-rib-cage", 0.88f, 0.008f, 0.170f, 0.136f, -0.114f, 0.152f, 0.106f, -0.094f),
        station("shoulder-girdle", 0.90f, 0.000f, 0.216f, 0.098f, -0.098f, 0.190f, 0.072f, -0.074f),
        station("clavicle-level", 0.94f, 0.004f, 0.196f, 0.096f, -0.086f, 0.174f, 0.070f, -0.064f),
        station("neck-base-interface", 1.00f, 0.000f, 0.060f, 0.052f, -0.052f, 0.045f, 0.040f, -0.040f),
    };
}

} // namespace

const char* confidenceName(ValueConfidence c) {
    switch (c) {
        case ValueConfidence::Measured: return "MEASURED";
        case ValueConfidence::Derived: return "DERIVED";
        case ValueConfidence::Assumed: return "ASSUMED";
        case ValueConfidence::Undetermined: return "UNDETERMINED";
    }
    return "UNKNOWN";
}

// --- 1. ProportionProfile -------------------------------------------------

void ProportionProfile::waistHalfAxes(float* halfWidthM, float* halfDepthM) const {
    // Ramanujan's ellipse perimeter:
    //   P = pi * (3(a+b) - sqrt((3a+b)(a+3b)))
    // The circumference is MEASURED (CDC NHANES SR50); the aspect ratio a/b is
    // ASSUMED, so the solved axes inherit "assumed" quality: they are only as
    // good as the 1.25 ratio.
    const float perimeter = waistCircumferenceM();
    const float ratio = std::max(1.0f, waistAspectRatio);

    // Solve for `a` with b = a / ratio.
    float lo = 0.01f;
    float hi = 1.0f;
    for (int i = 0; i < 80; ++i) {
        const float mid = 0.5f * (lo + hi);
        const float b = mid / ratio;
        const float p = kPi * (3.0f * (mid + b) -
                               std::sqrt((3.0f * mid + b) * (mid + 3.0f * b)));
        if (p < perimeter) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    const float a = 0.5f * (lo + hi);
    if (halfWidthM) *halfWidthM = a;
    if (halfDepthM) *halfDepthM = a / ratio;
}

float ProportionProfile::chestHalfWidthM() const {
    // Chest BREADTH is not in either anthropometric source (the Canonical
    // Acceptance Sheet v0 records M-3 as TBD). It is therefore derived from an
    // ASSUMED chest aspect ratio and stays Assumed - it must not be used to
    // gate a pass.
    //
    // The chest is less wide relative to its depth than the waist is, so the
    // assumed ratio is smaller than the waist's.
    const float assumedAspect = 1.15f;
    const float depth = chestHalfDepthM();
    return depth * assumedAspect;
}

// --- 2. AnatomicalLandmarks ----------------------------------------------

const std::vector<LandmarkDecl>& standardLandmarkCatalogue() {
    // Names are the international anatomical names, so they can be mapped onto
    // FMA concept ids (BodyParts3D, R-B02) later without renaming.
    static const std::vector<LandmarkDecl> kCatalogue = {
        {"jugular-notch",        LandmarkKind::Surface,  false, ValueConfidence::Measured},
        {"sternal-angle",        LandmarkKind::Surface,  false, ValueConfidence::Measured},
        {"xiphoid-process",      LandmarkKind::Surface,  false, ValueConfidence::Measured},
        {"costal-margin",        LandmarkKind::Surface,  false, ValueConfidence::Measured},
        {"clavicle",             LandmarkKind::Surface,  false, ValueConfidence::Measured},
        {"acromion",             LandmarkKind::Surface,  false, ValueConfidence::Measured},
        {"iliac-crest",          LandmarkKind::Surface,  false, ValueConfidence::Measured},
        {"anterior-superior-iliac-spine", LandmarkKind::Surface, false, ValueConfidence::Measured},
        {"umbilicus",            LandmarkKind::Surface,  false, ValueConfidence::Measured},
        {"linea-alba",           LandmarkKind::Curve,    false, ValueConfidence::Measured},
        {"jugular-notch-to-sternal-angle", LandmarkKind::Curve, false, ValueConfidence::Measured},
        {"midline",              LandmarkKind::Curve,    false, ValueConfidence::Measured},
        {"lower-rib-cage",       LandmarkKind::Surface,  false, ValueConfidence::Assumed},
        {"upper-rib-cage",       LandmarkKind::Surface,  false, ValueConfidence::Assumed},
        {"left-clavicle-medial", LandmarkKind::Surface,  false, ValueConfidence::Assumed},
        {"right-clavicle-medial",LandmarkKind::Surface,  false, ValueConfidence::Assumed},
        {"left-clavicle-lateral",LandmarkKind::Surface,  false, ValueConfidence::Assumed},
        {"right-clavicle-lateral",LandmarkKind::Surface, false, ValueConfidence::Assumed},
        {"left-scapula-medial",  LandmarkKind::Surface,  false, ValueConfidence::Assumed},
        {"right-scapula-medial", LandmarkKind::Surface,  false, ValueConfidence::Assumed},
        {"left-scapula-lateral", LandmarkKind::Surface,  false, ValueConfidence::Assumed},
        {"right-scapula-lateral",LandmarkKind::Surface,  false, ValueConfidence::Assumed},
        {"left-trapezius",       LandmarkKind::Surface,  false, ValueConfidence::Assumed},
        {"right-trapezius",      LandmarkKind::Surface,  false, ValueConfidence::Assumed},
        {"left-upper-arm-root",  LandmarkKind::Surface,  false, ValueConfidence::Assumed},
        {"right-upper-arm-root", LandmarkKind::Surface,  false, ValueConfidence::Assumed},
        {"left-deltoid-surface", LandmarkKind::Surface,  false, ValueConfidence::Assumed},
        {"right-deltoid-surface",LandmarkKind::Surface,  false, ValueConfidence::Assumed},
        {"neck-base-interface",  LandmarkKind::Curve,    false, ValueConfidence::Assumed},
    };
    return kCatalogue;
}

// --- 3. AnatomicalRegions -------------------------------------------------

const char* regionName(RegionId id) {
    switch (id) {
        case RegionId::Thorax: return "thorax";
        case RegionId::Abdomen: return "abdomen";
        case RegionId::Pelvis: return "pelvis";
        case RegionId::ShoulderGirdle: return "shoulder-girdle";
        case RegionId::Axilla: return "axilla";
        case RegionId::Neck: return "neck";
        case RegionId::RegionCount: break;
    }
    return "unknown";
}

bool AnatomicalRegions::contains(RegionId id) const {
    return std::find(regions.begin(), regions.end(), id) != regions.end();
}

const RegionAdjacency* AnatomicalRegions::adjacencyBetween(RegionId a, RegionId b) const {
    for (const RegionAdjacency& adj : adjacency) {
        if ((adj.a == a && adj.b == b) || (adj.a == b && adj.b == a)) return &adj;
    }
    return nullptr;
}

// --- 4. ShapeParameters ---------------------------------------------------

const ShapeParameterDecl* ShapeParameters::find(const std::string& name) const {
    for (const ShapeParameterDecl& decl : declarations) {
        if (decl.name == name) return &decl;
    }
    return nullptr;
}

float ShapeParameters::value(const std::string& name, float fallback) const {
    for (size_t i = 0; i < declarations.size(); ++i) {
        if (declarations[i].name == name) {
            return i < values.size() ? values[i] : declarations[i].defaultValue;
        }
    }
    return fallback;
}

void ShapeParameters::setValue(const std::string& name, float value) {
    for (size_t i = 0; i < declarations.size(); ++i) {
        if (declarations[i].name != name) continue;
        if (values.size() < declarations.size()) values.resize(declarations.size(), 0.0f);
        values[i] = std::clamp(value, declarations[i].minValue, declarations[i].maxValue);
        return;
    }
}

// --- 5. Default definition ------------------------------------------------

const std::vector<ShapeParameterDecl>& defaultShapeParameterTable() {
    static const std::vector<ShapeParameterDecl> kTable = {
        {"thorax-ribcage-volume", "ratio",
         "Scales the rib-cage volume of the thorax guide, widening and deepening the chest.",
         0.85f, 1.15f, 1.00f,
         {RegionId::Thorax}, ParameterStatus::DeclaredOnly, ValueConfidence::Assumed},

        {"thorax-depth-ratio", "ratio",
         "Scales chest depth relative to chest width.",
         0.85f, 1.15f, 1.00f,
         {RegionId::Thorax}, ParameterStatus::DeclaredOnly, ValueConfidence::Assumed},

        {"abdomen-volume", "ratio",
         "Scales the abdominal section volume between the costal margin and the iliac crest.",
         0.85f, 1.15f, 1.00f,
         {RegionId::Abdomen}, ParameterStatus::DeclaredOnly, ValueConfidence::Assumed},

        {"waist-pinch", "ratio",
         "Controls how strongly the waist narrows between the rib cage and the pelvis.",
         0.0f, 1.0f, 0.5f,
         {RegionId::Abdomen}, ParameterStatus::DeclaredOnly, ValueConfidence::Assumed},

        {"pelvis-volume", "ratio",
         "Scales the pelvic section volume at the hip breadth.",
         0.85f, 1.15f, 1.00f,
         {RegionId::Pelvis}, ParameterStatus::DeclaredOnly, ValueConfidence::Assumed},

        {"shoulder-girdle-mass", "ratio",
         "Scales the shoulder-girdle mass that forms the deltoid and clavicle region above the thorax.",
         0.85f, 1.15f, 1.00f,
         {RegionId::ShoulderGirdle}, ParameterStatus::DeclaredOnly, ValueConfidence::Assumed},

        {"shoulder-girdle-height", "m",
         "Sets the vertical extent over which the shoulder girdle rises above the thorax.",
         0.03f, 0.14f, 0.075f,
         {RegionId::ShoulderGirdle}, ParameterStatus::DeclaredOnly, ValueConfidence::Assumed},

        {"axilla-depth", "m",
         "Depth of the concave armpit constraint carved into the shared envelope.",
         0.0f, 0.06f, 0.028f,
         {RegionId::Axilla}, ParameterStatus::DeclaredOnly, ValueConfidence::Assumed},

        {"stylization", "ratio",
         "Global exaggeration of the section shape away from a pure ellipse.",
         0.0f, 1.0f, 0.25f,
         {RegionId::Thorax, RegionId::Abdomen, RegionId::Pelvis},
         ParameterStatus::DeclaredOnly, ValueConfidence::Assumed},

        {"spine-curve", "m",
         "Lateral (sagittal) offset of the body centreline at the top of the thorax.",
         -0.03f, 0.03f, 0.0f,
         {RegionId::Thorax, RegionId::Abdomen},
         ParameterStatus::DeclaredOnly, ValueConfidence::Assumed},
    };
    return kTable;
}

CharacterDefinition defaultCharacterDefinition() {
    CharacterDefinition def;

    def.proportions = ProportionProfile{};
    def.torso.controlStations = defaultTorsoControlStations();

    def.landmarks.catalogue = standardLandmarkCatalogue();

    def.regions.regions = {
        RegionId::Thorax,
        RegionId::Abdomen,
        RegionId::Pelvis,
        RegionId::ShoulderGirdle,
        RegionId::Axilla,
    };
    def.regions.adjacency = {
        {RegionId::Thorax, RegionId::ShoulderGirdle, 0.55f},
        {RegionId::Thorax, RegionId::Axilla, 0.40f},
        {RegionId::ShoulderGirdle, RegionId::Axilla, 0.60f},
        {RegionId::Thorax, RegionId::Abdomen, 0.85f},
        {RegionId::Abdomen, RegionId::Pelvis, 0.90f},
        {RegionId::Pelvis, RegionId::Axilla, 0.0f},
    };

    def.parameters.declarations = defaultShapeParameterTable();
    def.parameters.values.reserve(def.parameters.declarations.size());
    for (const ShapeParameterDecl& decl : def.parameters.declarations) {
        def.parameters.values.push_back(decl.defaultValue);
    }

    // JointDeformationRegions is an INTERFACE DECLARATION only (ADR-011):
    // PoC-SM declares it and does not implement any deformation. The bone ids
    // stay inside the frozen BoneId 0-21 range.
    def.jointDeformations = {
        {0,  "hips",        0.10f, DeformSide::Compress, 0x0000000Fu, true,  false},
        {1,  "spine",       0.12f, DeformSide::Stretch,  0x0000001Fu, true,  false},
        {2,  "chest",       0.14f, DeformSide::Compress, 0x0000003Fu, true,  false},
        {3,  "neck",        0.08f, DeformSide::Compress, 0x0000007Fu, true,  false},
    };

    return def;
}

CharacterDefinition defaultR1CharacterDefinition() {
    CharacterDefinition definition = defaultCharacterDefinition();
    definition.schemaVersion = 2;
    definition.torso.controlStations.clear();
    return definition;
}

} // namespace v2
} // namespace shape
} // namespace pae
