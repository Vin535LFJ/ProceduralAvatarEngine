# 03 — Geometry / Mesh Contract（P0 冻结）

## 正式主线

- Rig-aware Anatomical Landmarks → Topology Templates → Surface Patches → Variable-profile Sweeps → Patch Assembler。
- 截面必须是多控制点、非圆、宽深独立的闭合环；使用稳定 PTF，不允许固定 cross-product 翻转；taper 区域法线必须解析或面积加权修正。
- SDF/Surface Nets 保留为研究/对照和局部形态辅助（ADR-010），不得作为 P1 最终动画人体网格的唯一生产来源；`vertices.insert` + `indices.push_back` 不算几何融合。

## Topology Template / Patch Boundary 契约

- 每个 patch 定义：`boundaryId`、环顶点数、顶点顺序（顺时针/逆时针）、边界位置、边界法线、seam tag、相邻 patch id。
- 缝合只允许按 boundary 契约匹配对应环顶点；缝两侧位置误差 ≤1e-4 m，法线夹角 ≤5°；不允许靠重叠遮挡掩盖裂缝。
- 预留 seam：neck、shoulder、wrist、waist、hip/crotch、ankle/shoe。
- 每个 patch 独立保留 UV 与 region id；材质/换装组件可通过 seam tag 查找替换范围。
- `MeshTopology::analyzeTopology` 输出连通分量、边界边、非流形边、重复顶点；P1-A PoC 需报告，不要求所有 patch 单一水密（分组件允许，但每个组件边界必须闭合且符合契约）。

### Boundary Validation 数值层级

- **C0 位置连续**：相邻 patch 的对应边界顶点位置差 ≤1e-4 m；顶点数、顺序与 winding 一致。
- **G1 切平面/法线连续**：Smooth seam 的法线夹角 ≤5°、切平面夹角 ≤5°；Hard seam 允许有意法线突变，但必须标记为 hard；Open seam（独立组件允许开放边界）必须显式登记，不得误判为裂缝。
- 验证项：Boundary vertex correspondence、Position deviation、Winding consistency、Expected normal continuity、Tangent continuity where required、Manifold edge validation、Intentional open-boundary validation。
- 不要求所有边界法线数值完全相等：不同参数化方向与有意硬边可存在合理差异。

## Vertex / MeshData

- CPU `Vertex`：`position(Vec3)`, `normal(Vec3)`, `uv(Vec2)`, `joints(U16Vec4)`, `weights(Vec4)`, `color(Vec4)`；color.w 当前承载 `materialId`。
- `MeshData`：`vertices`, `indices(uint32)`, `subMeshes`, `bounds(Aabb)`；`SubMesh{firstIndex,indexCount,materialId}` 表示同一 buffer 内的材质/组件分段。
- 三角形 winding：逆时针为正面；法线单位长度；索引 < vertex count；AABB 与顶点一致。

## GPU Packing

- Skinned VAO 当前 64 字节交错：`pos(3f)@0`、`normal(3f)@12`、`joints(4×uint16)@24`、`weights(4f)@32`、`color(4f)@48`。
- 该 packing 未上传 UV；shader 当前也不使用 UV。若在 P3 引入纹理/程序化 UV 细节，必须新增 ADR 并同步 CPU 结构、GPU packing、VAO、GLSL、测试。
- 非蒙皮 GpuMesh 仍为 6 float（pos+color）简单布局，仅用于研究/辅助显示，不作为 P1 正式人体。

## 验证清单

- P1-A：非圆截面参数真实改变形状；PTF 连续无翻转；patch 边界位置/法线一致；法线与 winding 一致；每个参数影响目标区域；肩胸无可见缝或孤立球。
- P1-B：全身各部位接口 seam 合规；三参数组均通过 validator。
- P2：换装组件复用同一 seam contract，替换不重生成角色。
- 证据：`MeshValidator` 通过报告、`MeshTopology` 报告、真机 wireframe/normal/gray Lambert 截图。
