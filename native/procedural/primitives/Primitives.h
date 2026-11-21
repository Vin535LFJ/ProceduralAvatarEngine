#pragma once

#include "procedural/MeshData.h"

#include <cstdint>

namespace pae {

MeshData generateUvSphere(float radius, uint32_t segments, uint32_t rings);
MeshData generateEllipsoid(const Vec3& radii, uint32_t segments, uint32_t rings);
MeshData generateCapsule(float radius, float height, uint32_t segments,
                         uint32_t capRings, uint32_t bodyRings);
MeshData generateRoundedBox(const Vec3& size, float cornerRadius, uint32_t subdivisions);

} // namespace pae
