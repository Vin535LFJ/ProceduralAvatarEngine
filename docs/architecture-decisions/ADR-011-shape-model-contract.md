# ADR-011 — CharacterDefinition 分层 Shape Model 契约

> **状态：ACCEPTED**（2026-11-24）。依据 DEC-09（已采纳 `09-final-recommendation.md` 的分层契约）。
> 实施授权：**AUTH-08 已签发（2026-11-24）**，见 `docs/phases/anatomy-driven-geometry-poc/10-decision-records/04-AUTH-08-granted.md`。
> 本 ADR 批准的是**契约结构**，不是任何具体实现结果；`G-P1-GEO` 仍为 FAIL。

## 决策记录（DECISION RECORD）

| 字段 | 值 |
|---|---|
| 决策项 | ADR-011 状态 |
| 决策结果 | **ACCEPTED** |
| 决策人 | 项目负责人 |
| 决策日期 | **2026-11-24** |
| 决策来源 | 负责人在 WorkBuddy 会话中明确选择「签 4 份 + 下发 AUTH-08」执行包 |
| 记录方式 | AI **仅作转录与归档**；未代签，未添加负责人未表达的判断 |
| 草案来源 | `ADR-011-shape-model-contract.draft.md`（SHA256 `543283957ddd9a173ae0ade420455d04989888d0de3fed454f3db73c877bbacf`，2026-11-23） |

## 背景

G1/G2 证明了一个关键事实：R3 用 4 个关键高度的半宽/半深包络表达躯干，上胸缺口达 13.5 cm，E3 32.11 mm，且模板加密几乎不收敛（max 仅 −2%）；20 个参数中只有 7 个有效，且有效集是 R2 的真子集。R2 的固定拓扑投影后端本身表现良好（E2 0.98 mm、0 sliver、stable ID 恒定）。

根因不是网格密度，也不是拓扑算法，而是**上游缺少一个能表达人体整体形态关系的语义模型**：参数直接控制 section width / field coefficient，没有共享的胸肩/腋窝/腹腰盆约束。肩根接缝 47.66° > 5° 合同值也是同一根因的表现。

## 候选方案

1. **A：采纳 `CharacterDefinition` 分层契约**（09 文档）——解剖语义为唯一上游，曲面/场是表达工具。
2. B：继续提升 R3 包络的控制点数量。
3. C：不建语义层，让 R2/R3 各自调参逼近目标。

## 决定

**采纳 A。** R2 固定拓扑投影保留为后端候选；R3 暂停投入。理由：B 已被 G2 证伪（加密不收敛）；C 会重演 DEF-02 那种"测的是参数差不是形态差"的归因污染。

## 批准范围（本次批准的唯一内容）

本 ADR **只**批准下列五层子契约的**存在与语义**：

| # | 子契约 | 内容 | 本次批准的是 |
|---|---|---|---|
| 1 | `ProportionProfile` | 身高、头身比、肩髋比、胸腰比、肢段比例 | **契约结构**（字段语义、约束派生关系） |
| 2 | `AnatomicalLandmarks` | 骨骼关节点、体表解剖点、解剖曲线 | **契约结构**；landmark 命名采用国际标准解剖名（可与 BodyParts3D 的 FMA concept id 对接） |
| 3 | `AnatomicalRegions` | 胸腔 / 腹壁 / 骨盆 / 肩带 / 腋窝等的 RegionId 与邻接关系 | **契约结构**；RegionId 贯通 Shape Model / Topology / Skinning |
| 4 | `ShapeParameters` | 体积分布、正/侧轮廓、风格夸张的高层参数 | **契约结构**；每个参数单一语义 + 显式标注影响区域 |
| 5 | `JointDeformationRegions` | 关节影响带、压缩/拉伸侧、骨骼 mask、体积保持与 corrective 入口 | **接口声明**（PoC-SM 只声明，不实现变形） |

**Shared Skin Envelope**：胸腔 B-spline guide + 腹盆 implicit volume field + 肩带 shared field + 凹面约束 → **一份**最终外表面。区域保留 RegionId，但不各自输出可见 mesh。

**未批准的参数取值**：`ProportionProfile` 的具体默认值与体型原型仍属 DEC-05 / Canonical Acceptance Sheet（OQ-01 / OQ-28），本 ADR **不**代为决定。

## 冻结边界（明确列入本次批准）

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
| N-1 | **不批准任何 Gate 判 PASS** | 契约批准 ≠ 实现结果 |
| N-2 | 不批准具体的参数默认值 / 体型原型 | 属 DEC-05 与 Canonical Acceptance Sheet（OQ-01 / OQ-28） |
| N-3 | 不批准改动 `MeshData` 契约、Renderer、RenderThread | SPEC 冻结项 |
| N-4 | 不批准以本 ADR 为由跳过 `G-P1-GEO` 的 ≤5° 硬条件 | SPEC §13 既有合同值（口径见 OQ-03 决策记录） |

## 理由

- 分层契约把"形态语义"和"几何算法"解耦；换算法不需要重定义人体。
- 参数影响区域显式化，才能做 §13 #9 的跨区域影响测量。
- RegionId 贯通 Shape Model / Topology / Skinning，避免四套语义漂移。
- 保持 BoneId 0–21 / UBO[22] ABI 不变（SPEC §11）。

## 影响

- 正面：后续换体型/风格/服装/头发可复用同一套生成库。
- 负面：PoC-SM 需要新 worktree 与新 host-only 目录；旧 G2 的 E1/E3 数字因 Target Reference 改变而作废，需要重跑 G2。
- 保留：E2 归因方法、Layer A/B 指标、`TopologyAudit` 的 crop/裂缝区分、7 视角相机协议均可继承。

## 验证

1. Before/After 灰模对照，同一相机/光/带/取景（DEC-05 rubric）。
2. 肩根接缝最大二面角 **≤5°**（合同值，阻塞项；测量口径见 OQ-03 决策记录）。
3. landmark、7 视角轮廓、10 段截面偏差；阈值由负责人按 DEC-05 批准后判定。
4. 参数有效集不得出现"R3 式 7/20 且是真子集"的塌陷。
5. 以新 Target Reference 重跑 G2；G3 从 DEFER 回到 OPEN。

## 回滚

PoC-SM 全部在新 worktree 的 host-only 目录内；不触碰生产入口与 ABI。失败即丢弃该分支，`a32251b` 的 G2 证据保持不变。

## 签署

```
ADR-011 状态： DRAFT / ACCEPTED / REJECTED
批准范围确认： 仅 CharacterDefinition 五层契约 + 冻结边界 F-1..F-6
本 ADR 不构成： 任何 Gate 的 PASS 判定
```

**签署结果（2026-11-24）：** `ACCEPTED` —— 由负责人在 WorkBuddy 会话中明确批准，AI 转录归档（见文首「决策记录」）。
