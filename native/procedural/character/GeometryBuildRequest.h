#pragma once

#include "procedural/shape/v2/CharacterDefinition.h"

#include <cstdint>
#include <string>

namespace pae::character {

enum class TopologyPolicy : uint8_t {
    FixedTemplate = 0
};

constexpr uint32_t kR1GeneratorVersion = 2;
constexpr uint32_t kR2GeneratorVersion = 3;

struct GeometryBuildRequest {
    TopologyPolicy topology = TopologyPolicy::FixedTemplate;
    uint32_t generatorVersion = 1;
    uint32_t detailLevel = 1;
    uint32_t templateVersion = 1;
    uint64_t definitionHash = 0;
    int previewMode = 2;
    uint32_t regionMask = 0xFFFFFFFFu;

    std::string cacheKey() const;
};

uint64_t hashCharacterDefinition(const shape::v2::CharacterDefinition& definition);
GeometryBuildRequest makeGeometryBuildRequest(
    const shape::v2::CharacterDefinition& definition,
    uint32_t detailLevel = 1,
    int previewMode = 2);

} // namespace pae::character