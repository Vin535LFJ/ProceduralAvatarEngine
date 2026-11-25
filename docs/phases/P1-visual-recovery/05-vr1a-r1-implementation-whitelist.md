# VR-1A-R1 精确实现白名单

文件：`docs/phases/P1-visual-recovery/05-vr1a-r1-implementation-whitelist.md`
任务：`P1-VR-1A-R1 — Bilateral Torso-Shoulder Vertical Slice`
日期：2026-11-25
状态：**`EFFECTIVE — AUTH-P1-VR-01 GRANTED`**

## 1. 前置条件

本白名单与已签发的 `AUTH-P1-VR-01` 一起生效。授权记录见 `04-p1-vr-supplemental-authorization.md`；VR-1A-R1 已实现，视觉验收仍 `BLOCKED`。

基线：

- 分支：`poc-sm/shape-model-v2`
- commit：`cc26390bf11f2a965cf75c9a1c8860b118737278`
- 工作区：既有未跟踪 `.workbuddy-ai/`、v6/v7 evidence，以及本白名单与授权草稿；不得清理或纳入生产提交。

## 2. 新增文件

### 2.1 数据与状态

- `native/procedural/character/Anthropometry.h/.cpp`
- `native/procedural/character/AnatomicalLandmarks.h/.cpp`
- `native/procedural/character/AnatomicalRegions.h/.cpp`
- `native/procedural/character/ResolvedCharacterState.h/.cpp`
- `native/procedural/character/GeometryBuildRequest.h/.cpp`

职责边界：

- `CharacterDefinition` 是唯一可编辑人体语义输入。
- `ResolvedCharacterState` 是不可变派生结果，包含 resolved anthropometry、landmarks、regions、`BodyJoints`、`JointFrames`、`Skeleton` 和 `LandmarkResolverReport`。
- `GeometryBuildRequest` 只承载 `TopologyPolicy`、`GeneratorVersion`、`DetailLevel`、`TemplateVersion`、`DefinitionHash`、`PreviewMode`。
- 派生对象不得回写 `CharacterDefinition`。

### 2.2 几何与拓扑

- `native/procedural/character/TopologyTemplate.h/.cpp`
- `native/procedural/character/SurfacePatch.h/.cpp`
- `native/procedural/character/VariableProfileSweep.h/.cpp`
- `native/procedural/character/LocalShapeBasis.h/.cpp`
- `native/procedural/character/PatchAssembler.h/.cpp`
- `native/procedural/character/TorsoShoulderPatchGenerator.h/.cpp`
- `native/procedural/character/LodCorrespondence.h/.cpp`
- `native/procedural/character/RegionWeightGenerator.h/.cpp`

允许的新增 API：

- `resolveCharacterState(const shape::v2::CharacterDefinition&, const LegacyJointAdapter&)`
- `makeGeometryBuildRequest(...)`
- `VariableProfileSweep::build(...)`
- `LocalShapeBasis::evaluate(...)`
- `TorsoShoulderPatchGenerator::generate(...)`
- `PatchAssembler::assemble(...)`
- `LodCorrespondence::build(...)`
- `RegionWeightGenerator::assign(...)`

### 2.3 测试与证据

- `tests/unit/vr_torso_shoulder_tests.cpp`
- `docs/phases/P1-visual-recovery/evidence/vr1a-r1/**`
- `docs/phases/P1-visual-recovery/06-vr1a-r1-implementation-report.md`

## 3. 修改文件与函数边界

### 3.1 `native/procedural/shape/v2/CharacterDefinition.h/.cpp`

允许：

- 增加 `schemaVersion`；
- 增加本轮需要的 `TorsoShapeParameters`、`ShoulderShapeParameters`、`UpperArmShapeParameters`、`AsymmetryParameters` 输入字段；
- 增加显式 landmark 输入/约束；
- 保留现有 `ProportionProfile`、`AnatomicalRegions`、`ShapeParameters`、`JointDeformationRegions` 语义；
- 保留已有 public API 和默认值，除非测试证明必须升级并记录迁移。

禁止：

- 创建第二个同职责 `HumanShapeDefinition`；
- 把 resolved anthropometry/landmarks/BodyJoints 作为可写权威存回本结构；
- 修改默认体型的 measured/assumed 标注来掩盖来源。

### 3.2 `native/renderer/RenderThread.h`

仅允许新增/修改 mode-2 P1-A cache 状态：

- matching definition/hash；
- `MeshData`；
- `Skeleton`；
- `BodyJoints`；
- template/version metadata；
- cache key 和失效标志。

禁止修改：

- command queue；
- lifecycle；
- EGL/Surface；
- mode-3 Canonical cache/revision；
- UBO/packing 字段。

### 3.3 `native/renderer/RenderThread.cpp`

仅允许以下函数中的明确路径；跨模式质量/状态修复须负责人在授权书 §4.1 额外初始：

- `RenderThread::ensureResources()` 的 P1-A 生成/上传块，当前约 `1079-1113`；
- `RenderThread::processCommands()` 的 `SetQuality` case 仅为 mode-2 增加 cache 失效/重建，不改变控制命令顺序；
- `RenderThread::activatePreviewGpuResources()` 的 mode-2 mesh/skeleton/joints 恢复，以及 mode-3 已有 mesh 的匹配 skeleton/joints/MotionRuntime 恢复；
- `RenderThread::ensureCanonicalGeometry()` 的 cached 分支仅为 Surface 重建后重新初始化 `MotionRuntime`，不改变 Canonical 几何、权重、UV 或验证逻辑；
- `RenderThread::drawFrame()` 的 `previewMode_ == 2` 分支，当前约 `1765-1795`；
- mode-2 `debugView_ == 5` Weight Heatmap 分支；
- mode-2 cache key/失效和 MotionRuntime/Pose/SkinPalette 重新同步。

禁止修改：

- `postExternalPose()` 队列策略；
- `processCommands()` 的控制命令顺序、Surface 命令和 Shutdown；
- Surface 生命周期；
- mode-0/1/3 绘制；
- `ensureCanonicalGeometry()` 的生成、缓存 key、网格、权重、UV 和验证逻辑；
- `validateCanonicalGpuPath()`；
- JNI 调用。

### 3.4 `native/CMakeLists.txt`

仅允许：

- 把本轮新增 character 源文件加入 `PAE_CORE_SOURCES`；
- 把现有 `procedural/shape/v2/CharacterDefinition.cpp` 加入 `PAE_CORE_SOURCES`，使 Host 与 Android 使用同一契约实现；

确保不重复编译，不改变其他 source 顺序或链接目标。

### 3.5 `tests/CMakeLists.txt`

仅允许新增 VR-1A-R1 test target，保留所有既有 target 和测试。

## 4. 明确不修改的文件

- `native/procedural/character/CharacterDefinition.h`（legacy P1-A patch parameters，保留不动）
- `native/procedural/character/BodyPatchGenerator.h/.cpp`（旧 baseline 保留不动）
- `native/procedural/humanoid/canonical/ContinuousSkinGenerator.cpp`
- `native/procedural/humanoid/canonical/CanonicalSkinGenerator.cpp/.h`
- `native/procedural/humanoid/Humanoid.cpp/.h`
- `native/procedural/humanoid/HeadAnatomy.cpp/.h`
- `native/procedural/humanoid/FaceGenerator.cpp/.h`
- `native/procedural/fusion/SdfFusion.cpp/.h`
- `native/skeleton/Skeleton.h/.cpp`
- `native/skeleton/Skinning.h/.cpp`
- `native/skeleton/weights/WeightGenerator.h/.cpp`
- `native/renderer/gl/SkinnedGpuMesh.cpp/.h`
- `app/src/main/java/com/pae/host/MainActivity.kt`
- `app/src/main/java/com/pae/host/NativeBridge.kt`
- `app/src/main/cpp/jni_bridge.cpp`
- `AGENT.md`
- `docs/architecture-decisions/**`
- AUTH/ADR/DEC/CAS/Gate 文件
- `.workbuddy-ai/`
- `evidence/character-v6/**`
- `evidence/character-v7/**`

## 5. 新旧生成器选择

- 新实现：`TorsoShoulderPatchGenerator`。
- 旧实现：`BodyPatchGenerator::generate` 保持原样。
- 默认提案：新 APK 使用 VR-1A，旧 P1-A 使用既有 APK/Host 结果作 baseline；是否要求同一 APK 内切换，由负责人在授权书 §6.1 选择。
- 若选择同一 APK 切换，只允许通过内部 `GeometryBuildRequest.generatorVersion` 选择，不新增 JNI/UI extra，也不得复用无关 debug extra。
- 选择未明确前，不得静默替换旧实现或宣称 baseline 等价。

## 6. 本轮必须实现的验证

### 数据层

- CharacterDefinition 输入合法性；
- ResolvedCharacterState 不可变派生；
- Landmark resolver 优先级；
- frame/unit/side/BoneId 绑定；
- 左右完全对称与轻微不对称。

### 几何层

- 非圆闭合周期截面；
- 胸廓宽深独立；
- waist pinch；
- shoulder width；
- RMF orthonormality/handedness；
- tangent 点积与 Jacobian 符号；
- 局部形态基组合；
- 腋窝凹面；
- Shared BoundaryEvaluator；
- C0 `<=1e-4 m`；
- 无 T-junction；
- winding/normal；
- 肩根两类研究诊断：相邻 quad 外法线、seam 两侧对应采样法线；先验证 winding，输出 max/mean/p95、纳入 quad 数和角度样本数；无 crop ring 记 `NOT APPLICABLE`；5° 不作硬验收；
- Stable ID 与 LOD correspondence。

### 权重层

- Top-4、非负、和 `1±1e-3`；
- BoneId 0–21；
- 左右隔离；
- Shoulder/UpperArm 过渡；
- 基础动作与局部形变质量。

### Android

- mode-2 Clay/Wireframe/Normal/Weight Heatmap；
- Weight Heatmap 显示实际 BoneId/权重/主导骨骼；
- cache/skeleton 一致；
- `0→1→2→3`、quality refresh、Surface recreation；
- mode-3 无回归；
- 八视角、Shoulder/Chest/Axilla 近景；
- Bind、左臂抬起、右臂抬起、双臂外展。

## 7. 性能与证据

采用已批准的临时 20% 同设备基线 guardrail，分项记录：

- 初始人体形态生成；
- Landmark/State 解析；
- Patch 生成；
- Patch 组装；
- 权重生成；
- GPU 上传；
- 稳态 FPS/P50/P95/P99；
- 峰值内存。

比较式：

- 时间、内存、P50/P95/P99：`new <= 1.20 * baseline`；
- FPS：`new >= 0.80 * baseline`；
- 局部 Patch 只与旧 P1-A mode-2 的同设备、同配置、同工作量数据比较；
- P1 完整 Canonical 仅保留为参考，不与局部 Patch 作等价生成时间 PASS/FAIL。

必须同时保留 P1 完整 Canonical 基线、旧 P1-A mode-2 基线和新 VR-1A mode-2 数据。

## 8. 停止条件

- 授权未正式签发或本白名单未获签署；
- 基准人体参数未批准；
- OQ-03 未确认阈值被当作硬验收；
- 需要修改任何明确禁止文件；
- 多人体语义权威；
- frame 翻转/Jacobian 退化；
- LOD/Stable ID 映射失败；
- Boundary C0 失败；
- Patch 依赖大量重叠 SDF；
- 左右权重串扰；
- Weight Heatmap 与 Skinning 不一致；
- cache mesh 与 Skeleton 不匹配；
- mode-3 回归；
- Host/Android 明显不一致；
- 超过临时 guardrail；
- 视觉仍为球体/圆管拼接。

## 9. 当前状态

- 授权：`AUTH-P1-VR-01 = GRANTED`。
- 精确白名单：`EFFECTIVE`。
- 生产代码：VR-1A-R1 已实现并接入 Android mode-2。
- Host/Android 技术检查：部分通过，完整结果见 `05-vr1a-visual-and-geometry-review.md`。
- VR-1A-R1 视觉：`BLOCKED`；未宣布 Gate PASS。
