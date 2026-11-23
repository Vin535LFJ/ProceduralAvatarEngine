# Open Issues / Questions Register — G2 冻结后

> 目的：把散落在 G2 报告、rubric、G3 记录、ADR 草案里的**待决问题**收成一份可跟踪清单，供负责人和审核人逐条回复。
> 基线：`a32251b`（G2 证据）；当前远端：`f792d52`。
> 状态定义：`OPEN`（待决定）/ `BLOCKING`（阻塞后续阶段）/ `NOT RUN`（未执行）/ `CLEAR`（已澄清）/ `DONE`（已关闭）。

## 1. 阻塞性问题（BLOCKING — 负责人必须给值）

| ID | 问题 | 阻塞什么 | 状态 | 负责人回复 |
|---|---|---|---|---|
| OQ-01 | DEC-05 的授权参考图 + rubric 来源是什么？ | 视觉 `PASS` 判定、阶段 1 视觉标准、阶段 3 重跑 G2 | **BLOCKING / OPEN** | ____________________ |
| OQ-02 | AUTH-08 的基线 commit 与 worktree 路径？ | 阶段 2 PoC-SM 代码实施（AUTH-07 不覆盖新 Shape Model） | **BLOCKING / OPEN** | ____________________ |
| OQ-03 | ≤5° 肩根接缝失败在 PoC-SM 前算阻塞还是可接受风险？ | G3 签署条件 | **BLOCKING**（方案建议：阻塞；负责人已倾向采纳） | 签字确认 ______ |
| OQ-04 | ADR-011 / ADR-012 是否签署？ | PoC-SM 契约、Saltline 研究边界 | **BLOCKING / DRAFT** | 签字 ______ 日期 ______ |
| OQ-05 | G3 决策记录是否按 `DEFER` 签署？ | 重开 G2 的正式起点、G4 启动条件 | **BLOCKING / DRAFT** | 签字 ______ 日期 ______ |

> OQ-01 与 OQ-02 没有默认值，AI 不代填。在此之前不写 PoC-SM 代码、不宣布任何视觉合格。

## 2. 待审核人回答（NOT RUN — 不阻塞文档推进，但建议在 G3 签署前收回）

| ID | 问题 | 载体 | 状态 |
|---|---|---|---|
| OQ-06 | E1/E2/E3 归因是否成立？"R3 误差在曲面不在投影"有无被采样口径污染？ | `02-route-review-prompt.md` Q1 | NOT RUN |
| OQ-07 | SPEC §8.1 八项对称性哪一项实际没做到？ | 同上 Q2 | NOT RUN |
| OQ-08 | R1/R2 的"自比为 0"解释是否诚实？ | 同上 Q3 | NOT RUN |
| OQ-09 | `TopologyAudit` 的 crop/裂缝区分是否可靠？肩根是否为连续连接？ | 同上 Q4 | NOT RUN |
| OQ-10 | 独立看 63 张灰模图，有无数值没抓到的一票否决缺陷？ | 同上 Q6 | NOT RUN |
| OQ-11 | 有无 §13 TBD 指标被误判 PASS？ | 同上 Q7 | NOT RUN |
| OQ-12 | `SdfFusion.cpp` 符号推导、`SkinWeights.cpp` 生产等价性、灰模相机一致性是否正确？ | 同上 Q9 | NOT RUN |
| OQ-13 | 是否同意 R2 作为提交签署的候选路线？ | 同上 Q10 | NOT RUN |

## 3. 未解决的技术/测量缺口（OPEN — 不立即阻塞，但需排期）

| ID | 缺口 | 证据 | 计划 |
|---|---|---|---|
| OQ-14 | 肩根接缝 ≤5° 合同失败（R1/R2 47.66°、R3 16.13°） | G2 §5 | PoC-SM 阻塞验收条件 |
| OQ-15 | R3 包络上胸缺口 13.5 cm、有效参数 7/20 且加密不收敛 | G1/G2 | R3 暂停投入，不再加码 |
| OQ-16 | 骨盆扇盖处 144 个法线翻转（ring 0–1） | G2 §4 | PoC-SM 复测；须区分 cap 伪影与真实缺陷 |
| OQ-17 | 跨参数组区域影响：R2 chest 组 33 顶点 >1 mm；R3 waist 组 250 顶点 >1 mm（max 14.86 mm） | G2 §13 #9 | 阈值 TBD，需负责人批准后判定 |
| OQ-18 | 体积对比不是同口径（R1 闭合、R2/R3 crop 开口，垂直范围不同） | G2 §7 注 | 需补同带体积测量 |
| OQ-19 | GPU 数值 parity `[UNVERIFIED]` | G2 §6 | G4 前专项完成；G2 不是 kill |
| OQ-20 | APK 哈希因 `SdfFusion.cpp` 改动变化（`088476fc…` → `c1cace9d…`），设备复验 NOT RUN | 符号推导文档 | G4 前复验 |
| OQ-21 | 参数"声明未达顶点"仍是 PARTIALLY VERIFIED，未升级 VERIFIED | G2 §8 | 逐参数差分测试排期 |

## 4. 已关闭（DONE）

| ID | 事项 | 关闭证据 |
|---|---|---|
| OQ-22 | G3 是否 DEFER / R1·R2·R3 如何处置 | 负责人 2026-11-23 采纳；SPEC §7.4 |
| OQ-23 | 是否采纳 09 分层契约 | DEC-09 = APPROVED；SPEC §7.1 |
| OQ-24 | PoC-A 命名冲突 | 改名 PoC-R / PoC-SM / PoC-SHOULDER / PoC-LIMB / PoC-FULL |
| OQ-25 | Saltline 是否单独立 ADR | ADR-012 草案，不引入 Three.js |
| OQ-26 | smooth-union 符号是否猜改 | 先推导 + 双实现单测（修复前 212 断言失败），再改 |

## 5. 同步状态

- 本文与全部证据/文档已在 `origin/feature/procedural-character-foundation`，HEAD `f792d52`。
- 回复请直接填 ID（如 "OQ-01: xxx，OQ-02: xxx"），我会把结果写回 SPEC register 并更新本表状态。
