#pragma once

// G1 (AUTH-07): unified Shape Model parameter contract.
//
// Every shape parameter carries a name, unit, valid range, default value,
// action region and an HONEST status:
//
//   Wired         - consumed by the generation chain. A measurable mesh delta
//                   is required before it may ever be called VERIFIED.
//   DeclaredOnly  - declared in the parameter struct but never read by the
//                   generation chain. Proven by a zero-delta test.
//   NotImplemented - part of the contract, but no backend yet.
//
// This turns "does this parameter actually do anything?" from a code comment
// into a measured, testable fact (DEC-06 / SPEC section 9).

#include "procedural/humanoid/anatomy/AnatomyField.h"

#include <string>
#include <vector>

namespace pae::shape {

enum class ParameterStatus : int {
    Wired = 0,
    DeclaredOnly = 1,
    NotImplemented = 2
};

struct ParameterMeta {
    const char* name;
    const char* unit;
    float minValue;
    float maxValue;
    float defaultValue;
    const char* region;
    ParameterStatus status;
    const char* note;
};

struct ShapeModel {
    anatomy::HumanAnatomyParameters anatomy{};
};

// Full contract table (one entry per parameter of HumanAnatomyParameters).
const std::vector<ParameterMeta>& parameterTable();

const ParameterMeta* findParameter(const std::string& name);

float readParameter(const ShapeModel& model, const std::string& name, bool* found = nullptr);

// Writes a parameter after validating the name and the range. A DeclaredOnly
// parameter stays writable on purpose: writing it must produce a ZERO mesh
// delta, which is exactly how the label is proved.
bool writeParameter(ShapeModel& model, const std::string& name, float value,
                    std::string* error);

bool parameterIsWired(const std::string& name);

int wiredParameterCount();
int declaredOnlyParameterCount();

} // namespace pae::shape
