# M3 — 02 设计

- `Vertex{position, normal, uv, joints, weights, color}`；`SubMesh{firstIndex,indexCount,materialId}`；`MeshData{vertices,indices,subMeshes,bounds}`。
- 图元：UV Sphere / Ellipsoid / Capsule（Lathe 轮廓）/ Rounded Box（clamp + 法线外推）。
- 曲面：Loft（截面环插值）、Sweep（中心线 + 半径曲线 + Parallel Transport Frame）；法线由解析或面积加权面法线累积得到。
- `MeshValidator`：检查有限值、索引范围、三角形面积、法线长度、权重和/影响数、AABB 与参数合理性。
- 确定性：不使用随机数；seed 只作为参数记录，任何扰动均由显式参数函数生成。
