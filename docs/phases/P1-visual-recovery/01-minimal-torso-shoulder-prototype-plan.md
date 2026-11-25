# VR-1A 最小原型实施计划：躯干—肩部连接

文件：`docs/phases/P1-visual-recovery/01-minimal-torso-shoulder-prototype-plan.md`
任务：P1-VR / Deliverable R6
日期：2026-11-25
状态：待负责人正式签发 P1-VR 补充授权后实施；本文件不是生产代码。

## 1. 原型目标

用最小区域验证新架构能否真正改善人体形态，而不是继续调椭球参数。

验证内容：

- `CharacterDefinition` 的人体测量/landmark/region 分层进入生产生成；
- 中心线 `C(s)` + 局部 frame `e1/e2`；
- 非圆、宽深独立、随位置变化的截面；
- 局部胸廓/肩带/上臂 relief basis；
- 腋窝 concavity；
- 共享边界 C0 `<=1e-4 m`，肩根 crop 环记录两类研究诊断；OQ-03 5° 阈值暂不作为硬验收；
- region-aware Top-4 权重；
- Android mode-2 真机 Clay/Wireframe/Normal/Weight 证据；
- 与旧 P1-A 和 P1 Canonical 的同机位对比。

不验证：完整人体、面部、手足、最终视觉 Gate、production Canonical 替换。

## 2. 为什么选择躯干—肩部

该区域同时覆盖：

- 宏观比例；
- 解剖标志点；
- 中心线；
- 非圆截面；
- Surface Patch；
- 局部融合；
- 肩胸边界；
-  Chest/Shoulder/UpperArm 权重过渡。

它可以在不重建全身的情况下，证明新方法是否比当前 SDF 更自然。

## 3. 原型范围

纳入：

- thorax；
- abdomen；
- pelvis 上半；
- shoulder girdle；
- axilla；
- 左右上臂根到肘上沿；
- 相应 landmark、region、shape basis 和权重。

不纳入：

- head/face；
- hand/foot；
- 完整闭合 Canonical；
- 新 JNI；
- 新骨骼；
- 新 GPU packing；
- 最终 UV unwrap；
- 学习模型或外部资产。

## 4. 生产接入策略

采用两阶段接入，避免局部开放网格破坏 mode-3 Canonical 验收：

### VR-1A：mode-2 局部原型

- 保留 `previewMode=2` 的 Android 绘制路径；
- 新 `TorsoShoulderPatchGenerator` 与旧 P1-A `BodyPatchGenerator` 并存为可选 baseline，验证通过前不静默覆盖旧路径；
- 新实现必须真正生成左右上臂，使用 side-specific bone mask 和左右独立 seam；当前 `BodyPatchGenerator` 只生成左臂，所谓对侧 bridge 仍复用左臂环/权重，不能作为新方案基础；
- 输出 `MeshData + PatchGenerationReport`，继续走现有 64B upload；
- 局部 patch 允许显式 crop boundary，但所有 seam 必须注册；
- mode-2 增加 `debugView=5` Weight Heatmap 分支，使用现有 `skinnedProgram_`/`uDebugMode=5`，不新增 JNI；
- 定义 mode-2 cache key 为 `schemaVersion + generatorVersion + definitionHash + detailLevel`；质量变化必须失效 patch cache；
- mode-2 per-mode cache payload 由 RenderThread 持有，必须同时包含 matching definition/hash、mesh、`Skeleton` 和 `BodyJoints`，不能只缓存 mesh；cache hit 时原子恢复 mesh/skeleton/joints 并重新初始化或校验 `MotionRuntime`，失败则保留旧 mode 状态；
- mode 切换涉及共享 `skeleton_`/`motion_` 时，必须恢复或重新初始化 mode-2 的 skeleton/joints，不能沿用 mode-3 状态；
- 验收前必须回归 `0→1→2→3`、quality refresh 和 Surface recreation。

### VR-1B：mode-3 完整 Canonical

- 原型通过后，生成完整闭合身体；
- 通过 `generateContinuousSkin` 同一入口接入；
- 保持 RenderThread 上传、UV 和 GPU parity 路径。

这样 VR-1A 不需要先修改 `ContinuousSkinGenerator` 的全局 SDF，也不需要伪造封闭性。

## 5. 数据结构

唯一语义权威沿用并扩展 `pae::shape::v2::CharacterDefinition`，不新增并行 `HumanShapeDefinition`；输入、派生状态和生成请求分层：

```cpp
// Editable input; extends pae::shape::v2::CharacterDefinition in place.
struct CharacterDefinition {
    uint32_t schemaVersion = 1;
    uint64_t seed = 0;
    ProportionProfile proportions;
    AnatomicalLandmarkInput explicitLandmarks;
    AnatomicalRegions regions;
    HumanShapeParameters shape; // VR-1A fills torso/limbs upper-arm fields
    AsymmetryParameters asymmetry;
    JointDeformationRegions jointDeformations;
    ParameterConfig config;
    TopologyPolicyRef topologyRef;
};

// Immutable derived state.
struct ResolvedCharacterState {
    Anthropometry anthropometry;
    AnatomicalLandmarks landmarks;
    AnatomicalRegions resolvedRegions;
    BodyJoints joints;
    std::array<JointFrame, BoneCount> jointFrames;
    Skeleton skeleton;
    ValueConfidence confidence;
};

struct GeometryBuildRequest {
    uint32_t generatorVersion;
    TopologyPolicy topology;
    uint32_t detailLevel;
    uint32_t templateVersion;
    RegionMask regionMask;
};
```

只实现 VR-1A 所需字段，不提前加入 face/hand/foot。

关键类型：

- `LandmarkId`：iliac-crest、waist、costal-margin、sternal-angle、chest、clavicle、acromion、axilla、shoulder、elbow；
- `RegionId`：Thorax、Abdomen、Pelvis、ShoulderGirdle、Axilla、UpperArmL/R；
- `BoundaryId`：pelvis-crop、neck-crop、shoulder-root-L/R、arm-root-L/R；
- `ShapeBasisId`：ribcage-profile、waist-pinch、clavicle-relief、deltoid-relief、axilla-concavity；
- `ValueConfidence`：Measured/Derived/Assumed/Undetermined。

## 6. 算法

### 6.1 Landmark resolver

输入 `CharacterDefinition` 的 `ProportionProfile` 和显式 landmark；legacy `BodyJoints` 只作为一次性兼容适配，不再与 CharacterDefinition 同时成为权威：

- 坐标遵循 ADR-002：右手系、米、+Y 上、+Z 正面、+X 角色左侧；每个 landmark 记录 frame/unit/side/confidence 和可选 BoneId 绑定；
- resolver 优先级固定为：显式 landmark → legacy `BodyJoints` 一次性适配 → proportion/anthropometry 解析 → Undetermined；冲突时保留显式值并记录 confidence 降级；
- 解析结果写入不可变 `ResolvedCharacterState`，`BodyJoints` 与 `Skeleton` 由 resolved joint centers 派生，不得回写输入；
- 由肩髋比和肢段长度解析 acromion/elbow；
- 由胸腰/胸廓比例解析 costal margin、sternal angle、waist；
- 由肩带高度解析 clavicle 和 axilla；
- 所有默认值带 confidence，不把假设标成 measured；
- 持久 `stableId` 与 build-local Canonical index 分离；跨 detail level 使用 template version + local ID 重映射。

### 6.2 躯干 patch

- 中心线由 pelvis → waist → costal margin → sternum → clavicle → neck base；
- 每个截面使用 12–16 个语义控制点，而不是单一 width/depth ellipse；
- 前后、左右分别控制；
- 使用 C2 插值保证区域过渡连续；
- 胸廓、腹部、骨盆通过 partition-of-unity 组合。

Frame 与截面约束：

- 初始 frame 由 landmark/joint frame 固定，使用 Parallel Transport / Rotation-Minimizing Frame；
- 明确 tangent 反向、近零曲率、重复点和急弯处理；
- 检查 orthonormality、相邻 tangent 点积和 Jacobian 符号；
- 12–16 控制点仅为原型起点，复杂肩带/腋窝使用独立 Surface Patch 与闭合周期截面；
- 每个局部 basis 声明 support/amplitude/边界连续性，并测试曲面翻转、折叠、曲率突变和自交。

### 6.3 肩带和上臂根

- 肩带 patch 从 clavicle/acromion 延伸到左右上臂根环；
- 左右上臂分别使用 `P(s,φ)` 可变截面和 side-specific bone mask；
- deltoid/biceps/triceps 使用局部法线 relief；
- 腋窝使用负向 concavity；
- 不使用独立肩球；
- 左右 seam、权重和姿态分别测试，禁止复用左侧 ring/权重充当右侧。

### 6.4 边界组装

- 共享边界由同一个 `BoundaryEvaluator` 求值；
- 两侧使用相同顶点数和弧长参数；
- 共享 Preview Vertex ID；
- 重算法线；
- C0 位置误差 `<=1e-4 m`；
- 普通 seam 报告法线/切平面连续性；肩根 crop 环同时记录两类研究诊断：现有 OQ-03 相邻 quad 外法线夹角，以及专家建议的 seam 两侧对应采样点法线夹角；
- 两类诊断都从最终 mesh face/triangle winding 独立计算，输出 max/mean/p95、样本数和 winding 检查；禁止复用硬编码 0 的 `BoundaryReport`；
- OQ-03 口径/阈值存在治理冲突，5° 暂不作为 VR-1A 硬验收；未由负责人确认前不得标记 PASS。无 crop ring 记 `NOT APPLICABLE`。

### 6.5 权重

- Thorax：Hips/Spine/Chest；
- ShoulderGirdle/Axilla：Chest/Shoulder/UpperArm；
- UpperArm：Shoulder/UpperArm/LowerArm；
- 只允许 region mask 内骨骼；
- Top-4、非负、归一化；
- 肩根禁止 Chest 权重直接串到对侧手臂。

## 7. 预计修改文件

### 新增

- `native/procedural/character/Anthropometry.h/.cpp`
- `native/procedural/character/AsymmetryParameters.h/.cpp`
- `native/procedural/character/TopologyPolicy.h/.cpp`
- `native/procedural/character/HumanShapeParameters.h/.cpp`
- `native/procedural/character/ResolvedCharacterState.h/.cpp`
- `native/procedural/character/GeometryBuildRequest.h/.cpp`
- `native/procedural/character/LodCorrespondence.h/.cpp`
- `native/procedural/character/AnatomicalLandmarks.h/.cpp`
- `native/procedural/character/AnatomicalRegions.h/.cpp`
- `native/procedural/character/TopologyTemplate.h/.cpp`
- `native/procedural/character/SurfacePatch.h/.cpp`
- `native/procedural/character/VariableProfileSweep.h/.cpp`
- `native/procedural/character/LocalShapeBasis.h/.cpp`
- `native/procedural/character/PatchAssembler.h/.cpp`
- `native/procedural/character/TorsoShoulderPatchGenerator.h/.cpp`
- `tests/unit/vr_torso_shoulder_tests.cpp`

### 修改

- `native/procedural/shape/v2/CharacterDefinition.h/.cpp`：唯一人体语义权威原地扩展；
- `native/procedural/character/BodyPatchGenerator.cpp/.h`：保留旧 baseline，增加新 generator 选择；
- `native/renderer/RenderThread.cpp/.h`：mode-2 构造 `shape::v2::CharacterDefinition`、cache key、per-mode skeleton 恢复、`debugView=5` weight 分支；不改 JNI/命令架构；
- `native/CMakeLists.txt`
- `tests/CMakeLists.txt`
- `docs/phases/P1-visual-recovery/evidence/`：本轮证据

### 不修改

- `Skeleton.h/.cpp`
- `Skinning.h/.cpp` 的 LBS 数学和 UBO；
- `SkinnedGpuMesh` packing；
- JNI；
- `RenderThread` queue/lifecycle；
- 治理文件。

## 8. Host 测试

新增测试必须覆盖：

1. landmark 解析唯一、有限，frame/unit/side/BoneId 绑定和 resolver 优先级正确；
2. 截面非圆，正面/背面宽深独立；
3. 改变 ribcage volume 只影响 Thorax 和必要边界；
4. 改变 shoulder girdle 不影响对侧手臂；
5. axilla concavity 为负向局部位移；
6. 左右上臂都真实生成，side-specific 骨骼 mask 正确；
7. 先验证左右完全对称，再测试左肩略宽/右肩略窄；证明共享规则、独立参数、独立顶点/区域/骨骼，无镜像复用或权重串扰；
8. 肩胸 seam C0 `<=1e-4 m`；
9. 肩根 crop 环两类诊断（相邻 quad 外法线 / seam 两侧采样法线）均有 winding、max/mean/p95/count 记录；5° 未获负责人确认前不得作为硬验收；
10. 普通 seam 连续性报告不得替代肩根诊断；无 crop ring 时 `NOT APPLICABLE`；
11. 无 T-junction；
12. 权重合法、Top-4、无跨区串扰；
13. normals/winding 一致；
14. 持久 stableId 与 build-local index 分离；LOD 新顶点记录 parent face/edge/vertex、barycentric、refinement path 和 LOD level；LOD0↔LOD1↔LOD2 映射可测试；
15. 参数变化时 topology template ID/local ID 稳定；
16. mode-2 cache payload 包含 matching definition/hash、mesh、Skeleton/BodyJoints；cache key/质量失效和 per-mode skeleton 恢复正确；
17. `0→1→2→3`、quality refresh、Surface recreation 回归；
18. mode-2 `debugView=5` 产生 Weight Heatmap，而不是 fallback clay；
19. 与旧 P1-A 同输入的 mesh delta 可量化；
20. 生成时间和内存记录；
21. 新 PatchAssembler 不依赖现有 `CanonicalSkinGenerator::weldBoundaryLoops` 的等密度/`1e-3 m` 假设；非等密度重采样、切线元数据和 `1e-4 m` C0 必须有独立测试。
22. 基础动作：Bind、左右 arm raise、shoulder abduction、shoulder flexion；每个动作记录角度、旋转轴、坐标系和左右独立结果。
23. 局部形变：测量肩部体积变化、腋窝凹陷保留、关节塌陷、异常凸包、穿透和左右一致性；Top-4 合法不得替代本层。
24. Weight Heatmap 数据验证：显示当前 BoneId、每顶点实际权重、主导骨骼及权重、左右非法影响，并与模式切换后的 Skinning 权重一致。

## 9. Android 真机证据

设备沿用 `nunai_max`，固定 APK SHA、detail、相机、光照、Clay 材质和分辨率。

必交视图：

- front、back、left、right、45° left/right；
- shoulder/chest close-up；
- Clay、Wireframe、Normal、Weight Heatmap；
- Bind、左右 arm raise、shoulder abduction、shoulder flexion 对比；记录角度/轴/坐标系；
- 肩部体积、腋窝凹陷、关节塌陷、异常凸包、穿透和左右一致性的 posed 检查；
- `debugView=5` 明确显示权重颜色，不能以 clay fallback 代替；
- `0→1→2→3`、quality refresh、Surface recreation 的 mode/cache/skeleton 日志；
- 旧 P1-A、新 VR-1A、当前 P1 Canonical 的同机位 Before/After。

证据文件：

- APK SHA-256；
- Host test log 原文；
- Android build log；
- logcat；
- mesh SHA-256；
- 截图；
- 分项记录初始形态生成、Patch 组装、权重生成、GPU 上传、稳态帧时间与峰值内存；
- 已批准的临时 20% 同设备基线 guardrail；正式产品预算后续批准，不替代当前 guardrail；
- 失败和未验证清单。

## 10. 验收标准

技术基础：

- 构建和 Host tests 全绿；
- seam C0 `<=1e-4 m`；肩根两类研究诊断完整记录，5° 未获负责人确认前不作为硬验收；无 crop ring 时只记 `NOT APPLICABLE`；
- 权重合法；
- 三层变形验证通过：权重合法性、明确角度/轴/坐标系的基础动作、肩部体积/腋窝/塌陷/凸包/穿插/左右一致性；
- Android mode-2 真机运行无 GL/shader 错误；
- mode-2 Weight Heatmap、cache 失效、per-mode skeleton 恢复通过；
- `0→1→2→3`、quality refresh、Surface recreation 不回归；
- GPU/CPU 路径不回归。

视觉目标：

- 胸廓、腰、骨盆有自然体积变化；
- 肩胸不再读作“躯干 + 肩球”；
- 上臂不再读作圆管；
- 腋窝有真实凹陷；
- 正面/侧面/45° 的轮廓明显优于旧 P1-A；
- 不依赖材质、头发、服装或描边掩饰几何。

只有负责人确认视觉方向后，才进入 VR-1B。AI 不自行判定 Gate PASS。

## 11. 停止条件

- 补充授权未正式签发；
- 基准人体参数未批准，或性能超过已批准的临时 guardrail；
- OQ-03 口径/阈值未确认却被当作硬验收；
- CharacterDefinition 与 BodyJoints 出现多重权威；
- LOD Stable ID 无法可靠映射；
- frame 翻转或局部 Jacobian 退化；
- 需要改 ABI/骨骼/UBO/packing；
- 新 generator 无法保持边界一致；
- 结构化 Patch 必须依靠大量重叠 SDF 才能融合；
- 权重跨区或无法归一化；
- Weight Heatmap 与实际权重不一致；
- cache mesh 与 Skeleton/BodyJoints 不匹配；
- 真机与 Host hash/结果不一致；
- 生成时间或内存超过批准的 guardrail；
- 视觉仍为球/管拼接；
- 需要外部模型或 morph 数据才能改善。

## 12. 实施顺序

1. 负责人签发补充授权和白名单；
2. 记录已批准的视觉目标替代关系、临时 20% 性能 guardrail 和 OQ-03 暂缓硬阈值决定；
3. 新建 schema/ResolvedCharacterState/GeometryBuildRequest 最小实现和 Host tests；
4. 实现 bilateral torso/shoulder patch；
5. Host 边界、权重、参数差分、LOD lineage 和 Jacobian 验证；
6. 接入 Android mode-2；
7. 真机截图和性能采集；
8. 与旧 P1-A/P1 Canonical 对比；
9. 只读独立审查；
10. 负责人视觉验收；
11. 通过后才进入 VR-1B。

## 13. 当前状态

- 计划完成；
- 生产代码未修改；
- Host/Android 新原型未运行；
- 负责人已确认视觉目标替代、临时 20% 性能 guardrail 和 OQ-03 暂缓硬阈值；
- 等待负责人正式签发 P1-VR 补充授权。
