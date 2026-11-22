# ADR-009 上半身几何：统一 SDF 场合成（取代 ADR-008 的混合补丁路线）

- 状态：**Accepted**（2026-11-22，M12-A 实测数据 + 真机渲染证据定案）
- 取代：ADR-008 中「参数化主体 + 肩部局部 SDF 补丁」用于**上半身**的决定（ADR-008 对四肢下半身仍有效）
- 关联：`docs/phases/M12-unified-human-geometry/03-geometry-algorithm-comparison.md`、`tests/unit/anatomy_tests.cpp`、`docs/phases/M12-unified-human-geometry/evidence/`

## M11 的实证教训（为什么 ADR-008 不够）

M11-B PoC 证明「局部 SDF 补丁」只能焊接**一个关节**，但上半身有肩、腋、锁骨、颈胸交界等多个相邻融合区。代码审计（M12 01-code-audit.md）确认：

1. SDF 从未进入生产 Humanoid 生成流程（`Humanoid.cpp` 中 SDF 引用数 = 0），M11 的「SDF 融合」只存在于独立测试；
2. `MeshData` 合并只是 `vertices.insert` + `indices.push_back`（merge-at-447），**不是几何融合**——多个不连接部件被塞进同一个 buffer 就报告"完成"，拓扑上是多个连通分量；
3. 因此 M11 的上半身本质是「若干互不相连的部件拼进一个 buffer」，肩胸之间靠重叠遮挡，没有单一连续曲面。

## 三种方案对比结论（03 文档 §比较表，M12-A 用真实可渲染网格验证）

| 指标 | A 参数化拼接 | B 统一 SDF | C 参数化+局部补丁 |
|---|---|---|---|
| 连通性 | 多分量（merge≠fusion） | **单一水密曲面**（实测 components=1, boundary=0, nonmanifold=0） | 补丁交界无法真焊接 |
| 肩/腋融合 | 重叠遮挡 | **数学保证的 smooth-min 融合** | 裂缝/重叠 |
| 左右对称 | 手工 | **镜像求 min，像素级对称**（实测 L/R 视图 bodypx 差 <0.05%） | 手工 |
| 蒙皮权重 | 路径距离 | **场区域直接归属**（regionFields → regionBone） | 混合 |
| 三角形（真机 72×112×44） | ~14k（M11 avatar） | **24,360** | ~15k |
| 生成耗时 | ~40ms | 单次初始化（异步，符合 Spec §11） | ~200ms |

## 决定

**M12-A 上半身采用方案 B：全区域统一 SDF 场合成 → Surface Nets 提取**，理由：

1. M12 的核心诉求是「真正连续的人体曲面」与「真实拓扑连通」，只有统一场合成能从数学上保证 components=1 / boundary=0 / nonmanifold=0；
2. 场函数本身就是语义权重源，蒙皮归属比 M11 的路径距离更准确；
3. 镜像 min 天然保证左右完美对称；
4. 三角形超预算（24k vs M11 14k）由分辨率档位 LOD 接受，且真机仍稳定 51 FPS（PAE_STATS 实测），帧耗时 19.3ms p50；
5. SDF **真正接入生产流程**：`RenderThread::ensureResources` 直接调用 `anatomy::generateAnatomyUpperBody` 并上传 GPU，不是独立测试。

## 关键实现事实（可核验）

- 场合成：`native/procedural/humanoid/anatomy/AnatomyField.cpp` — `upperBodySdf = min(evaluateSide(p), evaluateSide(mirrorX(p)))`；躯干=肋/腹/盆三椭球 smooth-union + 背部大球 smooth-subtraction；肩=三角肌椭球+锁骨胶囊；臂=圆锥胶囊+肘球+前臂纺锤。
- 拓扑验证：`native/procedural/MeshTopology.{h,cpp}`，`analyzeTopology` 实测 components=1 / boundaryEdges=0 / nonManifoldEdges=0 / duplicateVertices=0（`anatomy_tests.cpp::anatomy_mesh_is_single_watertight_surface`）。
- 参数真实生效：轮廓射线扫描实测 chestWidth 0.8→1.3 时胸廓半宽 0.155→0.320m（`anatomy_parameters_change_body_shape`）。
- 真机渲染：Anatomy Preview Mode 8 固定相机 × 3 调试视图截图见 `docs/phases/M12-unified-human-geometry/evidence/dv{0,1,2}_preset{0..7}.png`，渲染侧确认日志 `PAE_RENDER applied cameraPreset/debugView`。

## 影响与回滚

- `anatomy::generateAnatomyUpperBody` 成为上半身生产生成器；下半身/四肢仍走 M11 参数化路线（ADR-008），两者在髋部交界的焊接是 M12-B 待办；
- 回滚：`previewMode_=0` 即回到 M11 avatar 渲染路径，互不影响；
- 风险：Surface Nets 在极端参数下可能产生 sliver 退化三角（测试容忍 <1%），已在拓扑报告中监控。