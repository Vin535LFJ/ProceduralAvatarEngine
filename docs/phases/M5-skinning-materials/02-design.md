# M5 — 02 设计

- `SkinnedGpuMesh`：VAO 含 position/normal/joints(uvec4)/weights/color，color.w 携带 materialId；数据来自 M4 `MeshData`。
- `BonePaletteUbo`：`std140` mat4[22] = 1408 字节，binding=0；C++ 以列主序写入 float[16*22]；每帧 `glBufferSubData`。
- `computePoseGlobals`：由 bindLocal 的平移 + 测试旋转构造 local，按父子链乘得 global；palette = global * inverseBind。
- `Skinning`：CPU 参考实现按同样权重公式计算位置，用于单测对照。
- 材质：Uniform 颜色（皮肤/头发/服装/眼睛）+ 简单方向光/环境光/Toon 分段 + Rim。
- 背景：全屏三角形（gl_VertexID），三种模式由 uniform 切换；先画背景再画角色。
