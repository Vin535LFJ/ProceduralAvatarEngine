# VR-1A-R2 精确实现白名单草稿

文件：`docs/phases/P1-visual-recovery/10-vr1a-r2-implementation-whitelist-draft.md`
关联授权：`09-vr1a-r2-supplemental-authorization-draft.md`
状态：**`EFFECTIVE — BOUND TO APPROVED R2 AUTHORIZATION`**

本白名单与已批准的 `09-vr1a-r2-supplemental-authorization-draft.md` 绑定生效。仅允许 R2 范围；VR-1B 和完整 Canonical 仍未授权。

## 1. 基线

- 分支：`poc-sm/shape-model-v2`。
- 提交：`299ae3bf3fcb6d76be563f815aa52380e018d50b`。
- R2 规划提交：`66a7e5905dde407b46df1e76074cd8b9ea94ab79`。
- 目标：`07-vr1a-r2-shape-target.md`。
- 计划：`08-vr1a-r2-anatomical-shape-revision-plan.md`。

## 2. 允许修改的数据与状态文件

### 2.1 `native/procedural/shape/v2/CharacterDefinition.h/.cpp`

允许：

- `schemaVersion` 升级到 3。
- 在现有 torso/shoulder/upper-arm 职责内增加 typed control stations、8 个角向控制和 directional form 字段。
- 明确 biacromial/shoulder-yoke/final external shoulder 三类宽度及测量区域。
- 增加 clavicle/acromion/axilla/costal margin 等 landmark 曲线定义。
- 为显式 landmark override 增加 confidence/provenance；无来源时保持 `Assumed`。
- 消除 `ShapeParameters` 与 typed 字段的同义双数值权威。
- 默认目标值只有在负责人确认目标形态后才能写入。

禁止：

- 新建第二个 `HumanShapeDefinition`。
- 把 resolved 状态回写为可编辑权威。
- 修改 BoneId/UBO/packing/JNI。

### 2.2 `native/procedural/character/AnatomicalLandmarks.h/.cpp`

允许：

- 增加 R2 所需 surface/curve landmark IDs。
- 解析 control station anchors。
- 保持显式 override 最高优先级。
- 输出可供 generator 直接消费的 landmark set。

禁止：

- 只解析不消费。
- 导入外部 landmark/morph 数据。

### 2.3 `native/procedural/character/ResolvedCharacterState.h/.cpp`

允许：

- 从 definition/landmarks 解析不可变 control stations。
- 校验 station 高度单调、frame/unit/side/BoneId。
- 保持 `BodyJoints/Skeleton` 现有生成路径。

禁止：

- 修改 `Humanoid.cpp` 或 `Skeleton.*`。
- 建立第二套关节权威。

### 2.4 `native/procedural/character/GeometryBuildRequest.cpp`

允许：

- 完整哈希所有 shape、landmark、control station 字段。
- 将 `generatorVersion` 与 `schemaVersion` 解耦，R1 baseline 显式 pin 版本。
- 将 detailLevel 映射到真实 LOD/resolution policy。
- 保留 cacheKey 现有字段格式。

禁止：

- 省略新字段导致 cache 不失效。
- 新增 JNI/UI extra。

## 3. 允许修改的几何与拓扑文件

### 3.1 `LocalShapeBasis.h/.cpp`

允许：

- 增加 station/angular support、direction、side mask、continuity、allowed combinations。
- 使用 C1 compact support 的方向性位移。

禁止：

- 用球形/椭球叠加表达 pectoral/deltoid/scapula。
- 保留无作用的 `region/normal` 字段作为假实现。

### 3.2 `VariableProfileSweep.h/.cpp`

允许：

- 从标量 radial profile 扩展为 8 方向闭合截面控制。
- 使用归一化 `E_position/E_normal/E_curvature` 和带单位的 epsilon 进行曲率/特征驱动采样。
- 保持 RMF/parallel transport 和 Jacobian 诊断。

禁止：

- 新运行时技术栈。
- 为隐藏 seam 均匀增加无意义网格。

### 3.3 `SurfacePatch.h`、`PatchAssembler.h/.cpp`

允许：

- 实际接入肩带、腋窝和上臂根 patch。
- 按 `BoundaryId + Side + ParameterDirection` 匹配两个 patch 的共享边界。
- 按 Stable ID/唯一参数映射对齐顶点和循环方向。
- 校验 C0、Stable ID、T-junction、winding、重复面、退化、sliver、self-intersection 和 Jacobian。
- 将共享边界映射为同一最终 index，并在任何校验失败时拒绝提交候选 mesh。
- 事务式组装和真实 continuity 报告。

禁止：

- 新建第二套同职责 assembler。
- merge-by-distance 伪装内部连续。

### 3.4 `TopologyTemplate.h/.cpp`

允许：

- 使用 control lattice Stable ID。
- 为细分 child 提供 parent/edge 派生 ID。

禁止：

- 改变既有 Stable ID 语义。
- 使 LOD 之间失去 lineage。

### 3.5 `TorsoShoulderPatchGenerator.h/.cpp`

允许：

- 保留 R1 算法作为 `generatorVersion=R1` baseline 路径。
- 新增 R2 多站点躯干、方向性形态基、结构化肩带、腋窝 patch、曲率上臂。
- 消费 `ResolvedCharacterState.landmarks/control stations`。
- 消费 `GeometryBuildRequest.detailLevel`。
- 删除或停用 `buildBridge()/capBoundaryLoops()` 等死代码，避免双拓扑。
- 输出注册 boundary loops 和真实 continuity 测量。

禁止：

- 平面 fan cap 作为最终肩带。
- 单球形腋窝。
- 线性中心路径 + 统一 radius 作为最终上臂。
- 修改 mode-3 Canonical 或 head/hand/foot/legs。

### 3.6 `LodCorrespondence.h/.cpp`

允许：

- parent face/edge/vertex、barycentric、refinement path。
- LOD0↔LOD1↔LOD2 映射验证。

禁止：

- 用 identity map 冒充真实 LOD。
- 破坏 R1 Stable ID 测试。

### 3.7 `RegionWeightGenerator.h/.cpp`

仅在新增肩带/腋窝 region 或权重支撑时允许修改。必须保持：

- Top-4。
- 非负、和 `1±1e-3`。
- BoneId 0–21。
- 左右隔离。
- Heatmap 与实际 skinning 一致。

### 3.8 `native/motion/providers/SyntheticPoseProvider.cpp`（条件项）

仅在授权 §4.5 明确勾选右臂单侧动作驱动时允许增加一个 deterministic right-arm raise action。不得修改骨骼、权重或其他动作语义；未勾选时该文件禁止修改，右臂单侧动作保持 `NOT VERIFIED`。

## 4. 允许修改的 Android/CMake/Test 文件

### 4.1 `native/renderer/RenderThread.h/.cpp`

仅允许：

- 根据 `GeometryBuildRequest.generatorVersion` 选择 R1/R2 baseline。
- mode-2 cache key、detail/LOD 失效和 matching skeleton/joints 恢复。
- R2 诊断日志。
- build-time R1/R2 证据 APK 选择。

禁止：

- camera/lighting 变化。
- mode-0/1/3 绘制变化。
- command queue/lifecycle/EGL/Surface/Shutdown。
- JNI/UI extra。

### 4.2 `native/CMakeLists.txt`

仅允许：

- R2 新增/修改 character 源文件进入 `pae_core`。
- build-time R1/R2 evidence selector。
- 不改变其他 source 顺序和链接目标。

### 4.3 `tests/CMakeLists.txt`

仅允许扩展 `pae_vr_torso_shoulder_tests` 或新增 R2 专项 target；保留所有 R1 测试。

### 4.4 `tests/unit/vr_torso_shoulder_tests.cpp`

必须新增：

- landmark→mesh 因果测试。
- control station 测量。
- directional basis 局部性。
- Acromion/Shoulder Joint/Upper-arm Root/Deltoid Surface 四点语义、权重和动作兼容。
- PatchAssembler 共享边界映射/方向/C0/Stable ID/T-junction/winding/拒绝组装。
- 自适应细分归一化误差、单位、epsilon 标定场景。
- detail 0/1/2 不同网格。
- LOD lineage。
- BoundaryId seam 与全局二面角分离。
- triangle quality/Jacobian/self-intersection。
- cache hash 完整性。
- R1/R2 deterministic baseline。
- R1 baseline golden hash `638a9cf368577940` 或逐顶点 parity。
- landmark override confidence/provenance 语义；R1 旧契约固定在 pin 的 R1 definition/version，R2 使用修正语义。

禁止删除、跳过或降低 R1 断言。

## 5. 明确不修改

- `native/procedural/humanoid/**`。
- `native/skeleton/**`。
- `native/renderer/gl/**`。
- `app/src/main/java/**`。
- `app/src/main/cpp/jni_bridge.cpp`。
- `AGENT.md`。
- `docs/architecture-decisions/**`。
- AUTH/ADR/DEC/CAS/Gate 文件。
- `.workbuddy-ai/`。
- `evidence/character-v6/**`、`evidence/character-v7/**`。

## 6. Boundary 与 Neck Interface 规则

- 肩胸、腋窝、上臂根内部边界必须由 PatchAssembler 映射为同一 indexed vertices，C0 `<=1e-4 m`，且不得登记为 Preview Crop Boundary。
- 若采用方案 A，只允许一个显式 `neck-base-interface` crop loop，并单独报告。
- 若采用方案 B，最小 neck/trapezius transition 必须在授权中单独勾选；不包含头颈动画。

### 6.1 肩部四类点与动作兼容

- Acromion 是 surface landmark；Shoulder Joint 是 bone joint center；Upper-arm Root 是几何连接点；Deltoid Surface 是软组织外轮廓。
- 不得把 Acromion 直接绑定为 Shoulder Joint。
- 必须验证现有 Shoulder Bone、Top-4 权重、左臂抬起和双臂外展下无异常拉扯/塌陷。
- 需要修改 Skeleton 或 BoneId 时立即停止并重新申请授权。

## 7. 证据与性能

必须保留：

- R1/R2 APK hash。
- 同定义 hash。
- camera/lighting 代码未变证据。
- 八视角、近景、Debug views、Actions。
- Host/Android 原始日志。
- 失败项与未验证项。
- 分项生成/组装/权重/上传/帧时间/内存。

R2 性能预算按授权 §4.4 执行。既有 20% guardrail 默认继续适用于同 workload/detail；不同密度必须记录归一化成本，未明确 supersede 前不宣称 guardrail PASS。

## 8. 允许写入的文档与证据

- `docs/phases/P1-visual-recovery/07-vr1a-r2-shape-target.md`
- `08-vr1a-r2-anatomical-shape-revision-plan.md`
- `09-vr1a-r2-supplemental-authorization-draft.md`
- `10-vr1a-r2-implementation-whitelist-draft.md`
- `docs/phases/P1-visual-recovery/evidence/vr1a-r2/**`
- R2 最终实现/视觉/几何/性能报告。

## 9. 当前状态

- R2 授权：`APPROVED FOR P1-VR-1A-R2 ONLY`。
- R2 白名单：`EFFECTIVE`。
- 生产代码：R2 实施已解锁，限于本白名单。
- Git：不自动 commit/push，修改保留在工作区。
- 视觉 Gate：未授权 AI 自行宣布 PASS。
