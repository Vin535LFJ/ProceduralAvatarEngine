# 06 — ADR-011 / ADR-012 负责人签署稿

> 状态：**签署稿 / FOR SIGNATURE**。AI 不代签，签字栏留空。
> 本稿基于仓库内 `docs/architecture-decisions/ADR-011-shape-model-contract.draft.md`（SHA256 `543283957ddd9a173ae0ade420455d04989888d0de3fed454f3db73c877bbacf`）
> 与 `ADR-012-saltline-module-study.draft.md`（SHA256 `9f47d0690ac9ef2eab4305bf2505ef557635555c87cab5e75cb765d446b788a2`）整理。
> 建议状态由本轮任务书给定：**ADR-011 = ACCEPTED；ADR-012 = ACCEPTED — RESEARCH ONLY**。

---

# 06-A — ADR-011 签署稿

## ADR-011 — CharacterDefinition 分层 Shape Model 契约

> **建议状态：ACCEPTED**
> **范围限定：只批准 `CharacterDefinition` 五层契约本身。**
> **本 ADR 的签署 ≠ AUTH-08 实施授权。**

## 背景（不变）

G1/G2 证明：R3 用 4 个关键高度的半宽/半深包络表达躯干，上胸缺口达 13.5 cm，E3 32.11 mm，且模板加密几乎不收敛（max 仅 −2%）；20 个参数中只有 7 个有效，且有效集是 R2 的真子集。R2 的固定拓扑投影后端本身表现良好（E2 0.98 mm、0 sliver、stable ID 恒定）。

根因不是网格密度，也不是拓扑算法，而是**上游缺少一个能表达人体整体形态关系的语义模型**。肩根接缝 47.66° > 5° 合同值是同一根因的表现。

## 决定

**采纳方案 A：采纳 `CharacterDefinition` 分层契约。** R2 固定拓扑投影保留为后端候选；R3 暂停投入。

## 批准范围（本次签署的唯一内容）

`[VERIFIED]` 本 ADR **只**批准下列五层子契约的**存在与语义**：

| # | 子契约 | 内容 | 本次批准的是 |
|---|---|---|---|
| 1 | `ProportionProfile` | 身高、头身比、肩髋比、胸腰比、肢段比例 | **契约结构**（字段语义、约束派生关系） |
| 2 | `AnatomicalLandmarks` | 骨骼关节点、体表解剖点、解剖曲线 | **契约结构**；landmark 命名采用国际标准解剖名（可与 BodyParts3D 的 FMA concept id 对接，见本轮 03 §3.2） |
| 3 | `AnatomicalRegions` | 胸腔 / 腹壁 / 骨盆 / 肩带 / 腋窝等的 RegionId 与邻接关系 | **契约结构**；RegionId 贯通 Shape Model / Topology / Skinning |
| 4 | `ShapeParameters` | 体积分布、正/侧轮廓、风格夸张的高层参数 | **契约结构**；每个参数单一语义 + 显式标注影响区域 |
| 5 | `JointDeformationRegions` | 关节影响带、压缩/拉伸侧、骨骼 mask、体积保持与 corrective 入口 | **接口声明**（PoC-SM 只声明，不实现变形） |

**Shared Skin Envelope**：胸腔 B-spline guide + 腹盆 implicit volume field + 肩带 shared field + 凹面约束 → **一份**最终外表面。区域保留 RegionId，但不各自输出可见 mesh。

## 冻结边界（不变，且明确列入本次批准）

| # | 边界 | 来源 |
|---|---|---|
| F-1 | **BoneId 0–21 不变** | SPEC §11 |
| F-2 | **UBO[22] 不变** | SPEC §11 |
| F-3 | **64B GPU vertex packing 不变** | SPEC §11 |
| F-4 | 不用独立肩球、不用圆管、不用 mesh append 冒充连续曲面 | SPEC §14.1 |
| F-5 | T3–T5 不在本 PoC 范围 | SPEC §10.1 |
| F-6 | JNI ABI 不变 | 项目硬约束 |

## 明确不批准的事项

| # | 不批准 | 理由 |
|---|---|---|
| N-1 | **不批准任何代码实施** | 实施授权由 **AUTH-08** 单独签发（OQ-02 仍 `BLOCKING / OPEN`） |
| N-2 | 不批准具体的参数默认值 / 体型原型 | 属 DEC-05 与 Canonical Acceptance Sheet（OQ-01 / OQ-28） |
| N-3 | 不批准改动 `MeshData` 契约、Renderer、RenderThread | SPEC 冻结项 |
| N-4 | 不批准以本 ADR 为由跳过 `G-P1-GEO` 的 ≤5° 硬条件 | SPEC §13 既有合同值 |

## 验证（不变）

1. Before/After 灰模对照，同一相机/光/带/取景（DEC-05 rubric）。
2. 肩根接缝最大二面角 **≤5°**（合同值，阻塞项）。
3. landmark、7 视角轮廓、10 段截面偏差；阈值由负责人按 DEC-05 批准后判定。
4. 参数有效集不得出现"R3 式 7/20 且是真子集"的塌陷。
5. 以新 Target Reference 重跑 G2；G3 从 DEFER 回到 OPEN。

## 回滚（不变）

PoC-SM 全部在新 worktree 的 host-only 目录内；不触碰生产入口与 ABI。失败即丢弃该分支，`a32251b` 的 G2 证据保持不变。

## 签署

```
ADR-011 状态： DRAFT / ACCEPTED / REJECTED      ← 负责人圈选
批准范围确认： 仅 CharacterDefinition 五层契约 + 冻结边界 F-1..F-6
本 ADR 不构成： AUTH-08 实施授权（OQ-02 仍未关闭）

负责人签字：____________________  日期：____________
```

---

# 06-B — ADR-012 签署稿

## ADR-012 — Saltline 程序化资源生成方法：独立技术模块研究

> **建议状态：ACCEPTED — RESEARCH ONLY**
> **只允许：方法研究、文档、接口草图。**
> **本 ADR 不是 PoC-SM 的强制前置条件。**

## 背景（不变）

项目需要一个可复用的程序化资源生成方法：同一套库应能生成不同体型、风格、服装、头发及其他角色。Saltline 被提出作为方法参考。

必须避免两个历史错误：① 直接引入网页运行时依赖（Three.js / JS），把 C++17 + Android NDK + GLES3 引擎变成 Web 栈；② 建设"迷你 Blender"——SPEC §4 明确禁止通用 quad remeshing（§14 延期项）、通用 SDF sculpting 工具与完整 NURBS 编辑器。

## 决定

**采纳方案 A：先做独立研究 ADR，只抽取真正复用的原语，C++17 原生实现。** 研究阶段只产出文档与接口草图。

## 批准范围（本次签署的唯一内容）

`[VERIFIED]` 本 ADR **只**批准**研究行为**：

| # | 允许 | 说明 |
|---|---|---|
| A-1 | Saltline 方法研究 | 组织思路、参数化组织方式、原语边界 |
| A-2 | 文档产出 | 研究笔记、接口草图、可行性说明 |
| A-3 | 接口草图 | 与 ADR-011 的 `CharacterDefinition` 五层子契约的对应关系 |
| A-4 | C++17 / NDK / GLES3 可行性论证 | 明确不依赖 GLES compute |

**研究范围（不变）**：

| 类别 | 候选原语 | 与现有契约的关系 |
|---|---|---|
| 几何生成 | 分层 SDF / envelope 构造、参数化截面、guide surface | 对齐 `MeshData` 与 PoC-SM 的 Shared Skin Envelope |
| 曲面 | B-spline / 隐式场求值、法线与曲率采样 | 复用 `SurfaceMetrics` 的采样口径 |
| 材质 | 灰模 Lambert 参数、材质 id 约定 | 不改 64B GPU packing；UV 仍为 CPU metadata |
| 资源构建 | 任务化生成、版本过滤、CPU mesh cache | 已有 Geometry Worker / cache 思路，避免重复建设 |

## 硬边界（不变）

| # | 禁止 |
|---|---|
| B-1 | **不引入 Three.js**、JS 引擎或任何网页运行时 |
| B-2 | **不实现通用 quad remeshing** |
| B-3 | **不实现通用 SDF sculpting** |
| B-4 | **不实现完整 NURBS 编辑器** |
| B-5 | 不改 BoneId 0–21、UBO[22]、64B packing 等冻结 ABI |
| B-6 | 不改 RenderThread 的生产渲染入口 |
| B-7 | 研究阶段不修改生产源码 |

## 与 PoC-SM 的依赖关系（本轮新增明确条款）

`[VERIFIED — 负责人指令]` **ADR-012 不应成为 PoC-SM 的强制前置条件，除非具体实现明确依赖 ADR-012 的研究结果。**

`[RECOMMENDATION]` 落地方式：ADR-012 的完成状态记为 `RESEARCH ONLY / NOT A PREREQUISITE`；
若 PoC-SM 的某项具体实现**明确**需要 ADR-012 的研究结论（例如 Shared Skin Envelope 的 guide surface 组织方式采用 Saltline 的某种具体分层法），则在 **AUTH-08 的实施说明中逐项标注该依赖**，并把该依赖项标记为"需先完成 ADR-012 对应研究条目"。
**默认状态：PoC-SM 不等待 ADR-012。**

## 明确不批准的事项

| # | 不批准 |
|---|---|
| N-1 | 不批准把 Saltline 运行时或任何 JS 依赖纳入引擎 |
| N-2 | 不批准在研究阶段创建生产代码路径 |
| N-3 | 不批准把研究结论直接当作实现授权（实现仍需 AUTH-08） |

## 验证（不变）

1. 研究 ADR 通过负责人审阅。
2. 抽出的一级原语能与 ADR-011 的 `CharacterDefinition` 五个子契约一一对应。
3. C++17 / NDK 25 / GLES3 可行性说明；不依赖 GLES compute。
4. 明确哪些原语**不**纳入，并给出理由。

## 回滚（不变）

研究阶段无生产变更；不通过则仅保留文档，不写入引擎。

## 签署

```
ADR-012 状态： DRAFT / ACCEPTED — RESEARCH ONLY / REJECTED      ← 负责人圈选
研究范围确认： 仅方法研究 / 文档 / 接口草图（不引入 Three.js、不改生产源码）
与 PoC-SM 关系： 非强制前置（除非实施说明逐项标注具体依赖）

负责人签字：____________________  日期：____________
```

---

## 签署后的影响

| 后果 | 说明 |
|---|---|
| **OQ-04 关闭** | ADR-011 与 ADR-012 均签署后，OQ-04 从 `BLOCKING / DRAFT` → `DONE` |
| **OQ-02 仍 BLOCKING** | ADR 签署**不**解锁 PoC-SM 编码；仍需负责人指定 AUTH-08 的基线与 worktree 路径并签署授权书（本轮 08） |
| **`G-P1-GEO` 仍 FAIL** | ADR 是契约批准，不是实现结果；接缝 ≤5° 仍须在 PoC-SM 中实际达成（OQ-14） |
| 不产生任何代码 | 两份 ADR 均为文档决策；签署后仍需 AUTH-08 才能写 PoC-SM 代码 |
