#pragma once

// Shared AL-6 differential helper. Used by both the unit tests and the
// evidence probe so that "declared" and "measured" cannot drift apart.

#include "procedural/MeshData.h"
#include "procedural/shape/v2/CharacterDefinition.h"
#include "procedural/shape/v2/SharedSkinEnvelope.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace pae {
namespace shape {
namespace v2 {
namespace param_diff {

constexpr int kRings = 26;
constexpr int kSegments = 32;

inline SharedSkinEnvelopeResult build(const CharacterDefinition& def) {
    SharedSkinEnvelopeSpec spec;
    spec.rings = kRings;
    spec.segments = kSegments;
    const BodyJoints joints{};
    return buildSharedSkinEnvelope(def, spec, joints);
}

// Region attribution is a HEIGHT-BAND PROXY, not a spatial region field:
// PoC-SM has no placed anatomical landmarks, so a surface point cannot be
// assigned to a named anatomical region with evidence. The four bands below
// come from the envelope's own anchor heights and are reported as such.
enum Band : int { Pelvis = 0, Abdomen = 1, Thorax = 2, ShoulderGirdle = 3, BandCount = 4 };

struct Delta {
    double maxM = 0.0;
    double meanM = 0.0;
    double rmsM = 0.0;
    int comparedVertices = 0;
    std::array<double, BandCount> bandMaxM{};
    std::array<int, BandCount> bandVertexCount{};
};

inline Delta compare(const MeshData& base, const MeshData& changed,
                     const SharedSkinEnvelopeResult& baseEnvelope) {
    Delta d;
    const size_t n = std::min(base.vertices.size(), changed.vertices.size());
    d.comparedVertices = static_cast<int>(n);
    if (n == 0) return d;

    std::array<double, BandCount> sums{};
    double sumSq = 0.0;
    for (size_t i = 0; i < n; ++i) {
        const Vec3 a = base.vertices[i].position;
        const Vec3 b = changed.vertices[i].position;
        const Vec3 delta = b - a;
        const double mag = std::sqrt(static_cast<double>(delta.lengthSq()));
        d.maxM = std::max(d.maxM, mag);
        d.meanM += mag;
        sumSq += mag * mag;

        const float y = a.y;
        const std::vector<float>& h = baseEnvelope.anchorHeights;
        int band = Band::ShoulderGirdle;
        if (h.size() >= 6) {
            if (y < h[1]) band = Band::Pelvis;
            else if (y < h[3]) band = Band::Abdomen;
            else if (y < h[5]) band = Band::Thorax;
        }
        d.bandMaxM[static_cast<size_t>(band)] =
            std::max(d.bandMaxM[static_cast<size_t>(band)], mag);
        ++d.bandVertexCount[static_cast<size_t>(band)];
        sums[static_cast<size_t>(band)] += mag;
    }
    d.meanM /= static_cast<double>(n);
    d.rmsM = std::sqrt(sumSq / static_cast<double>(n));
    (void)sums;
    return d;
}

struct ShapeResult {
    std::string name;
    std::string declaredStatus;
    std::string usedValue;
    Delta delta;
    bool actuallyMovesGeometry = false;
};

inline std::vector<ShapeResult> scanShapeParameters() {
    const CharacterDefinition baseDef = defaultCharacterDefinition();
    const SharedSkinEnvelopeResult base = build(baseDef);
    std::vector<ShapeResult> out;

    for (size_t i = 0; i < baseDef.parameters.declarations.size(); ++i) {
        const ShapeParameterDecl& decl = baseDef.parameters.declarations[i];
        CharacterDefinition changed = defaultCharacterDefinition();
        float value = decl.maxValue;
        if (std::fabs(value - decl.defaultValue) < 1e-9f) value = decl.minValue;
        changed.parameters.setValue(decl.name, value);
        const SharedSkinEnvelopeResult mesh = build(changed);

        ShapeResult r;
        r.name = decl.name;
        r.declaredStatus = decl.status == ParameterStatus::Wired ? "Wired" :
                           (decl.status == ParameterStatus::DeclaredOnly ? "DeclaredOnly" :
                                                                          "NotImplemented");
        r.usedValue = std::to_string(changed.parameters.value(decl.name));
        r.delta = compare(base.mesh, mesh.mesh, base);
        r.actuallyMovesGeometry = (r.delta.maxM > 1e-6);   // > 1 micron
        out.push_back(r);
    }
    return out;
}

struct ProportionResult {
    std::string field;
    double appliedScale = 0.0;
    Delta delta;
    bool actuallyMovesGeometry = false;
};

inline std::vector<ProportionResult> scanProportions() {
    const CharacterDefinition baseDef = defaultCharacterDefinition();
    const SharedSkinEnvelopeResult base = build(baseDef);
    std::vector<ProportionResult> out;

    const double scale = 1.10;
    auto push = [&](const char* name) {
        CharacterDefinition changed = defaultCharacterDefinition();
        float* target = nullptr;
        if (std::string(name) == "statureM") target = &changed.proportions.statureM;
        else if (std::string(name) == "shoulderBreadthRatio") target = &changed.proportions.shoulderBreadthRatio;
        else if (std::string(name) == "biacromialRatio") target = &changed.proportions.biacromialRatio;
        else if (std::string(name) == "hipBreadthRatio") target = &changed.proportions.hipBreadthRatio;
        else if (std::string(name) == "chestDepthRatio") target = &changed.proportions.chestDepthRatio;
        else if (std::string(name) == "abdominalDepthRatio") target = &changed.proportions.abdominalDepthRatio;
        else if (std::string(name) == "headHeightRatio") target = &changed.proportions.headHeightRatio;
        else if (std::string(name) == "waistCircumferenceRatio") target = &changed.proportions.waistCircumferenceRatio;
        else if (std::string(name) == "waistAspectRatio") target = &changed.proportions.waistAspectRatio;
        if (target == nullptr) return;
        const float original = *target;
        *target = static_cast<float>(original * scale);
        const SharedSkinEnvelopeResult mesh = build(changed);
        ProportionResult r;
        r.field = name;
        r.appliedScale = scale;
        r.delta = compare(base.mesh, mesh.mesh, base);
        r.actuallyMovesGeometry = (r.delta.maxM > 1e-6);
        out.push_back(r);
    };

    push("statureM");
    push("shoulderBreadthRatio");
    push("biacromialRatio");
    push("hipBreadthRatio");
    push("chestDepthRatio");
    push("abdominalDepthRatio");
    push("headHeightRatio");
    push("waistCircumferenceRatio");
    push("waistAspectRatio");
    return out;
}

} // namespace param_diff
} // namespace v2
} // namespace shape
} // namespace pae
