#include "procedural/shape/ShapeModel.h"

#include <cstring>

namespace pae::shape {
namespace {

using ParamPtr = float anatomy::HumanAnatomyParameters::*;

struct Binding {
    const char* name;
    ParamPtr ptr;
    const char* unit;
    float minValue;
    float maxValue;
    float defaultValue;
    const char* region;
    ParameterStatus status;
    const char* note;
};

// The status column is derived from the actual consumption in
// native/procedural/humanoid/anatomy/AnatomyField.cpp:
//   - buildPrimitives() reads the region scales,
//   - makeUpperBodyField() reads smoothness / axillaDepth.
// torsoHeight, upperArmLength and stylization are declared in
// HumanAnatomyParameters but never read anywhere -> DeclaredOnly.
const Binding kBindings[] = {
    {"torsoHeight", &anatomy::HumanAnatomyParameters::torsoHeight, "ratio", 0.80f, 1.25f, 1.00f,
     "torso", ParameterStatus::DeclaredOnly,
     "declared in HumanAnatomyParameters, never read by buildPrimitives/makeUpperBodyField"},
    {"shoulderWidth", &anatomy::HumanAnatomyParameters::shoulderWidth, "ratio", 0.70f, 1.30f, 1.00f,
     "shoulder", ParameterStatus::Wired, "trapezius sheet radii"},
    {"chestWidth", &anatomy::HumanAnatomyParameters::chestWidth, "ratio", 0.70f, 1.30f, 1.00f,
     "ribcage", ParameterStatus::Wired, "rib cage half-width / pectoralis belly"},
    {"chestDepth", &anatomy::HumanAnatomyParameters::chestDepth, "ratio", 0.70f, 1.30f, 1.00f,
     "ribcage", ParameterStatus::Wired, "rib cage half-depth / pectoralis origin"},
    {"waistWidth", &anatomy::HumanAnatomyParameters::waistWidth, "ratio", 0.70f, 1.30f, 1.00f,
     "waist", ParameterStatus::Wired, "abdomen half-width / lumbar mass"},
    {"waistDepth", &anatomy::HumanAnatomyParameters::waistDepth, "ratio", 0.70f, 1.30f, 1.00f,
     "waist", ParameterStatus::Wired, "abdomen half-depth / rectus path"},
    {"pelvisWidth", &anatomy::HumanAnatomyParameters::pelvisWidth, "ratio", 0.70f, 1.30f, 1.00f,
     "pelvis", ParameterStatus::Wired, "pelvis radii / lumbar base"},
    {"ribCageVolume", &anatomy::HumanAnatomyParameters::ribCageVolume, "ratio", 0.70f, 1.30f, 1.00f,
     "ribcage", ParameterStatus::Wired, "rib cage volume scale"},
    {"abdominalVolume", &anatomy::HumanAnatomyParameters::abdominalVolume, "ratio", 0.70f, 1.30f, 1.00f,
     "abdomen", ParameterStatus::Wired, "abdomen volume scale"},
    {"backDepth", &anatomy::HumanAnatomyParameters::backDepth, "normalized", 0.00f, 1.00f, 0.50f,
     "back", ParameterStatus::Wired, "back carve plane position"},
    {"waistCurvature", &anatomy::HumanAnatomyParameters::waistCurvature, "normalized", 0.00f, 1.00f, 0.50f,
     "waist", ParameterStatus::Wired, "waist pinch strength"},
    {"clavicleWidth", &anatomy::HumanAnatomyParameters::clavicleWidth, "ratio", 0.70f, 1.30f, 1.00f,
     "clavicle", ParameterStatus::Wired, "clavicle span and radius"},
    {"deltoidVolume", &anatomy::HumanAnatomyParameters::deltoidVolume, "ratio", 0.70f, 1.30f, 1.00f,
     "deltoid", ParameterStatus::Wired, "deltoid centre offset and radii"},
    {"shoulderSlope", &anatomy::HumanAnatomyParameters::shoulderSlope, "ratio", 0.50f, 1.50f, 1.00f,
     "deltoid", ParameterStatus::Wired, "deltoid / clavicle vertical drop"},
    {"axillaDepth", &anatomy::HumanAnatomyParameters::axillaDepth, "normalized", 0.00f, 1.00f, 0.60f,
     "axilla", ParameterStatus::Wired, "shoulder smooth-union blend sharpness"},
    {"upperArmLength", &anatomy::HumanAnatomyParameters::upperArmLength, "ratio", 0.85f, 1.15f, 1.00f,
     "upperarm", ParameterStatus::DeclaredOnly,
     "declared in HumanAnatomyParameters, never read by buildPrimitives/makeUpperBodyField"},
    {"upperArmVolume", &anatomy::HumanAnatomyParameters::upperArmVolume, "ratio", 0.70f, 1.30f, 1.00f,
     "upperarm", ParameterStatus::Wired, "upper arm / biceps / triceps radii"},
    {"elbowWidth", &anatomy::HumanAnatomyParameters::elbowWidth, "ratio", 0.70f, 1.30f, 1.00f,
     "elbow", ParameterStatus::Wired, "elbow radius and upper arm base"},
    {"forearmLength", &anatomy::HumanAnatomyParameters::forearmLength, "ratio", 0.85f, 1.15f, 1.00f,
     "forearm", ParameterStatus::Wired, "forearm end x offset"},
    {"forearmVolume", &anatomy::HumanAnatomyParameters::forearmVolume, "ratio", 0.70f, 1.30f, 1.00f,
     "forearm", ParameterStatus::Wired, "forearm / spindle / flexor / extensor radii"},
    {"wristWidth", &anatomy::HumanAnatomyParameters::wristWidth, "ratio", 0.70f, 1.30f, 1.00f,
     "wrist", ParameterStatus::Wired, "forearm end radius"},
    {"stylization", &anatomy::HumanAnatomyParameters::stylization, "ratio", 0.50f, 1.50f, 1.00f,
     "global", ParameterStatus::DeclaredOnly,
     "declared in HumanAnatomyParameters, never read by buildPrimitives/makeUpperBodyField"},
    {"smoothness", &anatomy::HumanAnatomyParameters::smoothness, "ratio", 0.20f, 2.00f, 1.00f,
     "global", ParameterStatus::Wired, "global smooth-union k multiplier and prune radii"},
};

constexpr int kBindingCount = static_cast<int>(sizeof(kBindings) / sizeof(kBindings[0]));

const Binding* findBinding(const std::string& name) {
    for (int i = 0; i < kBindingCount; ++i) {
        if (name == kBindings[i].name) return &kBindings[i];
    }
    return nullptr;
}

std::vector<ParameterMeta> buildTable() {
    std::vector<ParameterMeta> table;
    table.reserve(static_cast<size_t>(kBindingCount));
    for (int i = 0; i < kBindingCount; ++i) {
        const Binding& b = kBindings[i];
        table.push_back(ParameterMeta{b.name, b.unit, b.minValue, b.maxValue, b.defaultValue,
                                      b.region, b.status, b.note});
    }
    return table;
}

} // namespace

const std::vector<ParameterMeta>& parameterTable() {
    static const std::vector<ParameterMeta> table = buildTable();
    return table;
}

const ParameterMeta* findParameter(const std::string& name) {
    const Binding* binding = findBinding(name);
    if (!binding) return nullptr;
    for (const ParameterMeta& meta : parameterTable()) {
        if (std::strcmp(meta.name, binding->name) == 0) return &meta;
    }
    return nullptr;
}

float readParameter(const ShapeModel& model, const std::string& name, bool* found) {
    const Binding* binding = findBinding(name);
    if (!binding) {
        if (found) *found = false;
        return 0.0f;
    }
    if (found) *found = true;
    return model.anatomy.*(binding->ptr);
}

bool writeParameter(ShapeModel& model, const std::string& name, float value,
                    std::string* error) {
    const Binding* binding = findBinding(name);
    if (!binding) {
        if (error) *error = "unknown shape parameter: " + name;
        return false;
    }
    if (!(value == value)) {
        if (error) *error = "non-finite value for " + name;
        return false;
    }
    if (value < binding->minValue || value > binding->maxValue) {
        if (error) {
            *error = "value out of range for " + name + " [" +
                     std::to_string(binding->minValue) + ", " +
                     std::to_string(binding->maxValue) + "]";
        }
        return false;
    }
    model.anatomy.*(binding->ptr) = value;
    return true;
}

bool parameterIsWired(const std::string& name) {
    const Binding* binding = findBinding(name);
    return binding && binding->status == ParameterStatus::Wired;
}

int wiredParameterCount() {
    int count = 0;
    for (const ParameterMeta& meta : parameterTable()) {
        if (meta.status == ParameterStatus::Wired) ++count;
    }
    return count;
}

int declaredOnlyParameterCount() {
    int count = 0;
    for (const ParameterMeta& meta : parameterTable()) {
        if (meta.status == ParameterStatus::DeclaredOnly) ++count;
    }
    return count;
}

} // namespace pae::shape
