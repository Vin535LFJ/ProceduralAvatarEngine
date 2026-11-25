# PAE P1-VR VR-0 专家复核邀请

日期：2026-11-25
复核对象：自然半写实成人程序化人体形态重建路线
仓库分支：`poc-sm/shape-model-v2`
基线提交：`6e004b2b1e5cf6c6cd60efeb15cc48d16a3a9d2d`

## 一、请专家复核的问题

PAE 当前 P1 已完成连续 Canonical 身体、Render Mesh、UV、GPU skinning、质量切换和 Surface 重建，Host 19/19、Android 构建与真机技术验证通过；但灰模仍明显像机器人/人台。

我们不再把它当作“调几个椭球半径”的问题，而是完成了 VR-0 架构研究，请专家重点复核：

1. 人体语义参数、解剖标志点、区域拓扑和局部形态参数的分层是否合理？
2. 躯干/四肢采用 `P(s,φ)=C(s)+r1(s,φ)e1(s)+r2(s,φ)e2(s)` 加局部法线形态项，是否足以表达自然成人轮廓？
3. 面部、手部、足部采用独立结构化拓扑模板，而不是套用全身截面公式，这一边界是否正确？
4. SDF 仅用于腋窝、腹股沟、臀沟、肘窝、腘窝、鼻孔/口角等局部融合与修正，是否足以避免再次产生球/管拼接？
5. PAE 自有固定区域拓扑 + `templateVersion/templateId/localVertexId` 稳定 ID，能否支持后续形态基、权重和 corrective？
6. 在冻结 22 骨骼、Top-4、UBO[22]、64B packing、JNI ABI 条件下，VR-1 先做 mode-2 躯干—肩部双侧原型，再进入 mode-3 完整 Canonical，是否是合理的最小验证路径？
7. VR-1A 的验收是否既能证明新方法优于旧 P1-A，又不会用局部开放网格掩盖完整 Canonical 的问题？

## 二、当前机器人感的代码根因

- 躯干是三个 superellipsoid，肩/膝/踝是椭球，四肢是 tapered capsules。
- 躯干 18 mm、关节 14–18 mm、面部 16–30 mm smooth-union blend 与局部解剖特征尺度接近，抹平结构并产生圆钝 blob。
- 五层 `CharacterDefinition` 目前只在 Host PoC 编译，生产 Canonical 仍使用默认 `BodyParameters`。
- 生产 Canonical 由全身 SDF + Surface Nets 提取，缺少稳定区域拓扑和专用面部/手足结构。
- 手指/脚趾是单胶囊；面部是椭球/球/圆锥 union。
- 现有测试证明拓扑、权重和 GPU parity，但不能证明自然人体形态。

## 三、修订后的架构

```text
CharacterDefinition（唯一人体语义权威）
  → Anthropometry / Shape Parameters / Anatomical Landmarks / Regional Shape
  → Procedural Surface Construction
      - Variable-profile Sweep
      - SurfacePatch / TopologyTemplate
      - Local Shape Basis
  → Local Geometry Fusion
      - bounded local SDF
      - shared boundary evaluator
      - local concavity / correction
  → Canonical Skin Mesh
      - unified topology
      - CPU sidecar region/topology metadata
      - Top-4 weights
  → Render Mesh
  → Existing Android Render Pipeline
```

躯干与四肢：

\[
P(s,\phi)=C(s)+r_1(s,\phi)e_1(s)+r_2(s,\phi)e_2(s)
\]

\[
P'=P+\sum_i a_i\phi_i(s,\phi)n(s,\phi)
\]

## 四、外部成熟模型研究结论

实际核查了 MakeHuman、MPFB2、SMPL-X、FLAME、MANO、GHUM、MB-Lab、OpenSim 的官方源码/论文/许可：

- MakeHuman/MPFB2：固定 hm08 基础网格 + 语义参数 + 顶点 target delta。
- SMPL-X/FLAME/MANO/GHUM：固定拓扑 + shape/pose basis + joint predictor/regressor + LBS。
- MB-Lab：Blender 固定模板 + JSON morph + 配置驱动。
- OpenSim：关节中心、解剖标志点、人体测量和关节区域语义。

PAE 只借鉴数学/参数化思想，不导入任何外部网格、morph、blendshape、训练数据或运行时。许可证矩阵已区分代码、模型、训练数据、形态数据和论文/算法；不明确项均标记法律复核。

## 五、拟议的 VR-1A 最小原型

范围：

- thorax、abdomen、pelvis 上半；
- shoulder girdle、axilla；
- 左右上臂根到肘上沿；
- landmark/region/shape basis；
- 共享边界和 region-aware Top-4 权重。

接入：

- 使用 Android mode-2 局部预览，不把开放局部网格伪装成 mode-3 封闭 Canonical；
- 旧 P1-A 保留为可选 baseline；
- 新实现必须真正生成左右两侧；
- 增加 mode-2 `debugView=5` Weight Heatmap；
- per-mode cache 保存 matching definition/hash、mesh、Skeleton/BodyJoints，并原子恢复 MotionRuntime。

验收：

- C0 `<=1e-4 m`；
- 肩根 crop 环按 OQ-03：相邻 quad 外法线夹角 `[0°,180°]`，先验证 winding，输出 max/mean/p95/count，硬条件 max `<=5°`；
- 无 crop ring 记 `NOT APPLICABLE`，不得记 PASS；
- 无 T-junction；
- Top-4 合法、无跨区串扰；
- Android Clay/Wireframe/Normal/Weight 多视角证据；
- 与旧 P1-A/P1 Canonical 同机位对比；
- 视觉必须不再读作“躯干 + 肩球 + 圆管”。

## 六、请专家给出的结论

请按以下格式回复：

1. **总体方向**：认可 / 需修订 / 否决，并说明原因。
2. **参数体系**：指出缺失的人体测量、解剖标志点、局部形态或区域耦合参数。
3. **几何方法**：对各区域建议 Surface Patch、Sweep、Control Cage、局部形态基或局部 SDF 的优先级。
4. **拓扑与稳定 ID**：评估固定模板、边界缝合、跨 detail remap 是否可行。
5. **面部/手足**：指出眼睑、鼻、唇、手掌、指蹼、足弓和脚趾拓扑的关键风险。
6. **蒙皮与变形**：评估 22 骨骼限制、区域 mask、Top-4 和基础动作验证是否充分。
7. **VR-1A**：是否同意先做双侧躯干—肩部原型；列出必须增加或删除的验证项。
8. **停止条件**：列出专家认为必须立即停止或回到方案阶段的条件。
9. **参考依据**：请提供具体论文、官方文档或源码路径，不接受仅凭项目简介的判断。

## 七、治理与当前状态

- 负责人已选择新签 P1-VR 补充授权。
- 负责人已确认 P0 Gate 与 ADR-010 生效。
- AI 未代签授权文件，未修改治理文件。
- 当前只完成 VR-0 研究/设计，未修改生产代码，未开始 VR-1。
- 正式补充授权签发前，VR-1 生产写入保持 BLOCKED。
- AI 不宣布任何 Gate PASS。

## 八、复核材料

- 主计划：`docs/phases/P1-visual-recovery/P1-VR-001-Anatomical-Form-Recovery-Plan.md`
- R1 参考项目分析：`docs/research/parametric-human-modeling/01-reference-project-analysis.md`
- R2 技术与许可证矩阵：`docs/research/parametric-human-modeling/02-tech-and-license-matrix.md`
- R3 参数体系：`docs/architecture/parametric-human-model/01-human-shape-parameter-schema.md`
- R4 几何架构：`docs/architecture/parametric-human-model/02-procedural-geometry-architecture.md`
- R5 路线修订：`docs/phases/P1-visual-recovery/00-architecture-revision.md`
- R6 最小原型：`docs/phases/P1-visual-recovery/01-minimal-torso-shoulder-prototype-plan.md`
