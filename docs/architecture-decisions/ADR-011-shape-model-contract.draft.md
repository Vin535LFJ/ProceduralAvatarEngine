# ADR-011 — CharacterDefinition 分层 Shape Model 契约（草案）

> 状态：**DRAFT / 待负责人签署**。依据 DEC-09（已采纳 09-final-recommendation.md 的分层契约）。
> 实施授权：AUTH-08 仍为 OPEN；本 ADR 签署不等于授权写代码。

## 背景

G1/G2 证明了一个关键事实：R3 用 4 个关键高度的半宽/半深包络表达躯干，上胸缺口达 13.5 cm，E3 32.11 mm，且模板加密几乎不收敛（max 仅 −2%）；20 个参数中只有 7 个有效，且有效集是 R2 的真子集。R2 的固定拓扑投影后端本身表现良好（E2 0.98 mm、0 sliver、stable ID 恒定）。

根因不是网格密度，也不是拓扑算法，而是**上游缺少一个能表达人体整体形态关系的语义模型**：参数直接控制 section width / field coefficient，没有共享的胸肩/腋窝/腹腰盆约束。肩根接缝 47.66° > 5° 合同值也是同一根因的表现。

## 候选方案

1. **A：采纳 `CharacterDefinition` 分层契约**（09 文档）——解剖语义为唯一上游，曲面/场是表达工具。
2. B：继续提升 R3 包络的控制点数量。
3. C：不建语义层，让 R2/R3 各自调参逼近目标。

## 决定（建议）

**采纳 A。** R2 固定拓扑投影保留为后端候选；R3 暂停投入。理由：B 已被 G2 证伪（加密不收敛）；C 会重演 DEF-02 那种"测的是参数差不是形态差"的归因污染。

## 契约范围（PoC-SM）

`CharacterDefinition` 下辖五个子契约（来自 09 文档 §2 问题二）：

| 子契约 | 内容 | PoC-SM 必须 |
|---|---|---|
| `ProportionProfile` | 身高、头身比、肩髋比、胸腰比、肢段比例 | 只锁一种中性轻度运动型（DEC-01 推荐，仍待确认） |
| `AnatomicalLandmarks` | 骨骼关节点、体表解剖点、解剖曲线 | 与现有 15 个 landmark 集合对齐、可扩展 |
| `AnatomicalRegions` | 胸腔/腹壁/骨盆/肩带/腋窝等的 RegionId 与邻接 | 必须，用于权重与拓扑 |
| `ShapeParameters` | 体积分布、正/侧轮廓、风格夸张的高层参数 | 每个参数单一语义 + 标影响区域 |
| `JointDeformationRegions` | 关节影响带、压缩/拉伸侧、骨 mask | 只声明接口，PoC-SM 不实现变形 |

**Shared Skin Envelope**：胸腔 B-spline guide + 腹盆 implicit volume field + 肩带 shared field + 凹面约束 → 一份最终外表面。区域保留 RegionId，但不各自输出可见 mesh（09 问题四）。

**硬边界**：不用独立肩球、不用圆管、不用 mesh append 冒充连续曲面（SPEC §14.1）；T3–T5 不在本 PoC 范围（SPEC §10.1）。

## 理由

- 分层契约把"形态语义"和"几何算法"解耦；换算法不需要重定义人体。
- 参数影响区域显式化，才能做 §13 #9 的跨区域影响测量。
- RegionId 贯通 Shape Model / Topology / Skinning，避免四套语义漂移（09 问题六）。
- 保持 BoneId 0–21 / UBO[22] ABI 不变（SPEC §11）。

## 影响

- 正面：后续换体型/风格/服装/头发可复用同一套生成库。
- 负面：PoC-SM 需要新 worktree 与新 host-only 目录；旧 G2 的 E1/E3 数字因 Target Reference 改变而作废，需要重跑 G2。
- 保留：E2 归因方法、Layer A/B 指标、`TopologyAudit` 的 crop/裂缝区分、7 视角相机协议均可继承。

## 验证

1. Before/After 灰模对照，同一相机/光/带/取景（DEC-05 rubric）。
2. 肩根接缝最大二面角 ≤5°（合同值，阻塞项）。
3. 15 个 landmark、7 视角轮廓、10 段截面偏差；阈值由负责人按 DEC-05 批准后判定。
4. 参数有效集不得出现"R3 式 7/20 且是真子集"的塌陷。
5. 以新 Target Reference 重跑 G2；G3 从 DEFER 回到 OPEN。

## 回滚

PoC-SM 全部在新 worktree 的 host-only 目录内；不触碰生产入口与 ABI。失败即丢弃该分支，`a32251b` 的 G2 证据保持不变。

## 签署

```
ADR-011 状态： DRAFT / ACCEPTED / REJECTED
负责人签字：____________________  日期：____________
```
