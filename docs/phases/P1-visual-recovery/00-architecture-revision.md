# P1-VR 技术路线修订报告

文件：`docs/phases/P1-visual-recovery/00-architecture-revision.md`
任务：P1-VR / Deliverable R5
日期：2026-11-25
状态：VR-0 修订方案；基线材料已提交并推送 `8151e01`，本修订未 commit/push，不构成 Gate PASS。

## 1. 当前真实状态

| 项目 | 状态 |
|---|---|
| 工作区 | `E:/AI-Tools/ProceduralAvatarEngine-poc-sm` |
| 分支 | `poc-sm/shape-model-v2` |
| HEAD | `8151e01d812b21ba2ad6c5cfde7a657df68ea3c6` |
| 远端同名分支 | 与 HEAD 一致 |
| GitHub 默认分支 | `main`，HEAD `c539afbec299ed4a066429ea09f9d3d36e67f74d` |
| 已跟踪修改 | 本轮修订前无；本轮新增 `03-design-review-addendum.md` 并更新 R1–R6/主计划 |
| 未跟踪 | `.workbuddy-ai/`、v6/v7 evidence，以及本轮新增修订附录 |
| P1-VR 进度 | VR-0 研究/文档；VR-1 未开始 |
| 生产代码修改 | 无 |
| 当前 Agent | 主 Agent 只写研究文档；其他 Agent 只读审查/研究 |
| 本轮性质 | 纯审计/研究轮，按 ADR-013 记为 process note / 治理记录，不是工程里程碑 |

P1 已有成果保持不动：19/19 Host tests、Android build、Canonical/Render Mesh、UV、GPU/CPU parity、Surface recreation、quality refresh 和性能证据。

### 1.1 专家复核结论

专家有条件认可主路线，明确赞成 mode-2 双侧躯干—肩部原型，但要求 VR-1A 前完成一次小规模技术修订。八项意见及处理详见 `docs/phases/P1-visual-recovery/03-design-review-addendum.md`：

- 输入/派生状态分层；
- Sweep frame、截面和形态基约束；
- Skin Shell 边界规则；
- Stable ID + LOD lineage；
- 三层权重/动作/形变验证；
- 先对称后左右独立差异；
- OQ-03 冲突暂不作为硬验收；
- 新增停止条件和性能预算。

## 2. 治理状态与负责人决定

### 2.1 文件事实

- AUTH-08：`GRANTED`，但只授权 `native/procedural/shape/v2/` host-only PoC；FB-5/FB-7/FB-11 禁止 RenderThread、Android 生产路径和 legacy C++ 修改。
- P1 v0.2 任务书：记录 2026-11-24 负责人另行授权 P1 Android 生产生成链路，并给出白名单。
- P0 Gate：原报告为 `NOT VERIFIED`；ADR-010 文本为“Accepted，需用户 P0 Gate 确认后生效”。
- DEC-05/CAS/Canonical Acceptance Sheet：默认体型和审美阈值仍未完全批准。
- ADR-013：纯审计轮是治理里程碑，不是工程里程碑。

### 2.2 负责人在本轮明确选择

1. **新签 P1-VR 补充授权**，明确处理 AUTH-08 与 P1 Android 授权之间的覆盖关系。
2. **确认 P0 Gate 与 ADR-010 生效**。
3. **OQ-03 暂缓硬阈值**：两类角度只作研究诊断，待正式决策记录确认。
4. **确认自然半写实成人目标替代历史自然卡通目标**；AI 不擅自改写旧视觉文件。
5. **采用临时 20% 同设备基线性能 guardrail**，并分项记录生成、组装、权重、GPU、帧时间和内存；不替代产品预算。

记录边界：

- AI 不代签授权文件，不修改 AUTH/ADR/DEC/CAS/Gate 文件。
- 本报告记录负责人选择，不等于补充授权书已正式签发。
- VR-1 生产写入仍以负责人正式签发 P1-VR 补充授权和白名单为前置条件。

### 2.3 OQ-03 未决冲突

仓库 OQ-03 决策记录声称已固定相邻 quad 外法线 max `<=5°`；专家依据 ADR-013/AGENT.md 认为测量口径仍未决。AI 不自行裁定。负责人已决定 VR-1A 暂不把 5° 作为硬验收，可同时记录两类研究诊断，但不得标记 PASS；最终口径与阈值仍待正式决策记录。

### 2.4 建议补充授权覆盖范围

补充授权必须以精确路径/函数清单覆盖，并明确仅对清单内路径 supersede AUTH-08 的 FB-5/FB-6/FB-7/FB-11：

- 原地扩展 `native/procedural/shape/v2/CharacterDefinition.h/.cpp`；
- 新增 `native/procedural/character/Anthropometry.h/.cpp`、`AsymmetryParameters.h/.cpp`、`TopologyPolicy.h/.cpp`、`HumanShapeParameters.h/.cpp`、`ResolvedCharacterState.h/.cpp`、`GeometryBuildRequest.h/.cpp`、`LodCorrespondence.h/.cpp`、`SkinShellContract.h/.cpp`、`AnatomicalLandmarks.h/.cpp`、`AnatomicalRegions.h/.cpp`、`TopologyTemplate.h/.cpp`、`SurfacePatch.h/.cpp`、`VariableProfileSweep.h/.cpp`、`LocalShapeBasis.h/.cpp`、`PatchAssembler.h/.cpp`、`TorsoShoulderPatchGenerator.h/.cpp`、`CanonicalBodyGenerator.h/.cpp`；
- 修改 `native/procedural/character/BodyPatchGenerator.h/.cpp`，包括 mode-2 新旧 generator 选择；
- 修改 `native/procedural/humanoid/canonical/ContinuousSkinGenerator.cpp`、`CanonicalSkinGenerator.cpp/.h`、`HeadAnatomy.cpp/.h`、`FaceGenerator.cpp/.h`、`native/skeleton/weights/**`；
- 修改 `native/renderer/RenderThread.cpp/.h` 的 mode-2 生成、cache key、per-mode skeleton 恢复和 `debugView=5` weight 分支，但不改 JNI、命令架构、UBO、packing；
- 修改 `native/CMakeLists.txt`、`tests/CMakeLists.txt`，新增 `tests/unit/vr_*` 测试；
- 写入 `docs/phases/P1-visual-recovery/evidence/**` 和 `docs/phases/P1-visual-recovery/*.md`；
- Android 真机部署、截图和日志采集；
- 不自动 commit/push，除非负责人另行明确授权；
- ABI、BoneId、UBO、64B packing 和治理文件冻结继续有效。

## 3. 已审查代码的 A/B/C/D 分类

当前没有 VR-1 未提交代码，因此分类对象是 P1 已提交实现。

### A. 保留

- JNI facade、EngineRuntime、RenderThread command queue；
- 22 骨骼、BoneId 0–21、`UBO[22]`、Top-4；
- 64B base VBO + attribute 5 UV VBO；
- Canonical/Render Mesh 逻辑关系；
- `renderToCanonical`；
- GPU transform-feedback parity；
- atomic GPU upload、CPU cache、Surface recreation；
- 现有 Host tests 和真机 capture 能力。

### B. 保留但需进一步验证

- `BodyPatchGenerator`：非圆截面和 shoulder bridge 有价值，但只服务 mode 2，且有单侧/对侧权重需要审查；
- `SharedSkinEnvelope`：C2 曲线和区域 partition-of-unity 有价值，但仍使用 superellipse，且只有躯干；
- `HeadAnatomy`：landmark projection 和连续头颈思路有价值，但全局 primitive union 不适合最终面部；
- `CanonicalSkinGenerator`：边界重采样/匹配/焊接代码存在但生产未调用；`buildRenderMesh` 和 `extractBoundaryLoops` 已在生产使用，不能笼统称为全部未使用；
- 当前 LBS：数值路径稳定，但缺少 posed volume/self-intersection 验证。

### C. 需要重新审查/替换

- 全身 SDF + Surface Nets 作为唯一 Canonical 形态来源；
- 默认 `BodyParameters` 作为唯一人体语义输入；
- 躯干三块 superellipsoid、肩/膝/踝球、四肢胶囊；
- 面部椭球/球/圆锥 smooth union；
- 手指/脚趾单胶囊；
- 仅按最近骨段距离的六区权重；
- 用 hole-fill 和全局平滑掩盖局部结构缺失。

### D. 可能触及授权/架构边界

- 将 `shape::v2` 提升进 Android production；
- 修改 mode-2 `BodyPatchGenerator` 生产路径；
- 修改 `ContinuousSkinGenerator`、`HeadAnatomy`、`RenderThread`；
- 新增 region/topology metadata sidecar；
- 新增面部/手足拓扑和辅助几何。

这些修改不直接回滚；先通过补充授权和白名单，再按最小原型实施。

## 4. 为什么当前结果像机器人

代码层面的根因：

1. **形态先验错误**：躯干是三个 superellipsoid，关节是球/椭球，四肢是胶囊；SDF 只能把这些先验平滑地融合。
2. **blend 尺度接近特征尺度**：躯干 18 mm、关节 14–18 mm、面部 16–30 mm blend，会把胸廓、眼睑、指缝等结构抹成圆钝 blob。
3. **解剖语义没有进入生产**：五层 CharacterDefinition 只在 Host；landmarks 没有空间点；RegionId 没有贯通生产。
4. **稳定拓扑没有建立**：Surface Nets 的顶点/面随场变化，无法直接承载面部、眼睑、指节和足弓模板。
5. **局部结构被全局平均**：手指、脚趾、鼻翼、口角都需要局部拓扑和负向空间，全身标量场无法表达。
6. **规则对称和单一 clay 放大人台感**：完美镜像、缺少自然左右差异、眼/鼻/口空间关系不足。
7. **测试只证明技术可运行**：closure、weights、GPU parity 不能证明自然人体形态。

## 5. 修订后的核心架构

采纳负责人给出的方向：

```text
CharacterDefinition（唯一可编辑人体语义输入）
  → ResolvedCharacterState（不可变派生 Anthropometry/Landmarks/BodyJoints/Skeleton）
  → GeometryBuildRequest（generator/topology/detail/template/region mask）
  → Procedural Surface Construction
  → Local Geometry Fusion
  → Canonical Skin Mesh
  → Existing Android Render Pipeline
```

躯干和四肢：

\[
P(s,\phi)=C(s)+r_1(s,\phi)e_1(s)+r_2(s,\phi)e_2(s)
\]

\[
P'=P+\sum_i a_i\phi_i(s,\phi)n(s,\phi)
\]

面部、手部、足部：独立结构化 `SurfacePatch` / `TopologyTemplate`，不使用全身截面公式替代。

局部 SDF：只做腋窝、腹股沟、臀沟、肘窝、腘窝、鼻孔/口角等局部融合和修正。

## 6. 保留、替换、新增、暂停

| 动作 | 内容 |
|---|---|
| 保留 | 渲染 ABI、GPU skinning、Canonical/Render 映射、缓存、测试框架、既有证据 |
| 替换 | 全身 SDF 作为最终形态来源；椭球/胶囊躯干和关节；单胶囊 digit |
| 新增 | 人体参数 schema、ResolvedCharacterState/GeometryBuildRequest、landmark resolver、LOD lineage、Skin Shell 契约、区域拓扑、可变截面、局部形态基、PatchAssembler、结构化头/手/足 |
| 重构 | `BodyParameters` 角色、`ContinuousSkinGenerator` 职责、region-aware 权重 |
| 暂停 | 头发、服装、PBR、表情、学习模型、外部资产、完整 corrective；VR-4 前不进入 |

## 7. 修订后的 VR-1 至 VR-4

### VR-1A：躯干—肩部连接最小原型

- 只验证 thorax/abdomen/pelvis/shoulder girdle/axilla/左右 upper-arm root；必须真正生成左右两侧，不能复用当前单臂 `BodyPatchGenerator` 的对侧 bridge。
- 使用 mode-2 Android 预览验证局部 patch，不强行把开放局部 mesh 塞进要求封闭的 mode-3 Canonical。
- 旧 P1-A 保留为可选 baseline；新 generator 通过后才替换默认 mode-2 路径。
- 交付参数、非圆截面、局部 relief、边界、权重、Clay/Wireframe/Normal/Weight Heatmap 和同机位 Before/After。
- mode-2 增加 `debugView=5` weight 分支；定义 per-mode cache key，质量变化失效 patch cache，模式切换事务式恢复 matching definition/hash、mesh、Skeleton/BodyJoints，并重置/同步 MotionRuntime、Pose、SkinPalette。
- 先验证左右完全对称，再验证左肩略宽/右肩略窄等独立差异；两侧必须使用独立参数、区域、顶点和 BoneId mask。
- 肩根可独立记录现有 OQ-03 相邻 quad 外法线与专家建议的 seam 两侧采样法线两类诊断；5° 暂不作为硬验收，未确认前不得标记 PASS。
- 通过后才进入 VR-1B。

### VR-1B：躯干与四肢完整轮廓

- 扩展髋、大腿、膝、小腿、踝、前臂、腕。
- 统一 Canonical 封闭，保持现有 Render/UV/GPU 路径。
- 正面/背面/侧面/45° 必须读作成人身体，不再是球管拼接。

### VR-2：头部与面部

- 自有固定面部模板；
- 眼眶、眼睑、眼球辅助几何、鼻、唇、耳、下颌；
- 与颈/躯干共享边界；
- 多角度近景和基础 Head/Neck 变形；
- VR-2 前补充 GNM Head 内部结构/参数分区研究；只借鉴方法，不导入资产。

### VR-3：手部与足部

- 手掌、thenar/hypothenar、MCP、五个 digit；
- 足跟、足弓、前足、hallux、五个 toe；
- 22 骨骼限制下只验证 Hand/Foot/Toe 整体变形；
- 手指/脚趾独立关节运动不宣称完成。

### VR-4：整体融合与最终验证

- 完整 Canonical、Render Mesh、权重、姿态、性能；
- 八类基础动作；
- 真机多视角和局部近景；
- 独立视觉验收；AI 不宣布 Gate PASS。

## 8. 阶段验收与停止条件

每阶段必须同时有：

1. 新代码和测试；
2. 本轮 Android 真机 Clay/Wireframe/Normal 等证据；
3. 原始测试日志和失败/未验证清单。

停止条件：

- 补充授权未签发或超白名单；
- 需要改 ABI/骨骼/UBO/packing；
- 需要替换 RenderThread；
- 无法保持统一 Canonical；
- seam 无法满足 C0 `<=1e-4 m`；肩根 5° 口径/阈值未确认却被当作硬验收；
- CharacterDefinition 与 BodyJoints 出现多重权威；
- LOD Stable ID 无法可靠映射；
- frame 翻转或局部 Jacobian 退化；
- 结构化 Patch 必须依靠大量重叠 SDF 才能融合；
- 局部方案仍读作球/管拼接；
- 真机与 Host 不一致；
- 基准人体参数未批准，或性能超过已批准的临时 guardrail；
- 性能超过负责人批准的 guardrail；
- 外部模型/数据被误引入；
- Weight Heatmap 与实际权重不一致；
- cache mesh 与 Skeleton/BodyJoints 不匹配。

## 9. 本轮交付状态

- R1：已完成。
- R2：已完成。
- R3：已完成。
- R4：已完成。
- R5：本文件。
- R6：见 `docs/phases/P1-visual-recovery/01-minimal-torso-shoulder-prototype-plan.md`。
- 专家修订附录：`docs/phases/P1-visual-recovery/03-design-review-addendum.md`。
- 生产代码：未修改。
- VR-1：未开始。
- 本轮为纯审计/研究轮，按 ADR-013 记为 process note / 治理记录，不是工程里程碑。
