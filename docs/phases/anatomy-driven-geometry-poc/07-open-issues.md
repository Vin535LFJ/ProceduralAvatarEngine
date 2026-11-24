# Open Issues / Questions Register — G2 冻结后

> 目的：把散落在 G2 报告、rubric、G3 记录、ADR、DEC-05 参考体系里的待决问题
> 收成**唯一一份**可跟踪清单。
> G2 证据基线：`a32251b`；本表版本：2026-11-24（OQ 归并后 + 签署状态同步）。
> 状态：`OPEN`（待决定）/ `BLOCKING`（阻塞后续阶段）/ `NOT RUN`（未执行）/
> `PARTIAL`（部分关闭）/ `CLEAR`（已澄清）/ `DONE`（已关闭）。
> **父子项**：父项的 Closure Criteria 包含其子项全部关闭。子项**不单独计为 BLOCKING**。

## 1. 阻塞性主项（BLOCKING — 负责人必须签字）

| ID | Parent | 问题 | Owner | Blocking Target | Evidence | Status | Closure Criteria | 负责人回复 |
|---|---|---|---|---|---|---|---|---|
| OQ-01 | — | DEC-05 参考体系：是否批准方案 C（外部视觉参考 + 解剖工程参考 + 自有 Canonical Acceptance Sheet），并把五 Gate 结构写入 rubric？ | 项目负责人 | 视觉 `PASS` 判定；`G-P1-GEO` / `G-FINAL-VIS` 能否判 PASS | 本轮 02 参考更新矩阵；本轮 03 Canonical Sheet v0 | **BLOCKING / OPEN** —— 负责人 2026-11-24 决定**等 PoC-SM 灰模产出后再定** | ① 子项 OQ-28/29/30/31/32 全部关闭 ② 负责人签署 DEC-05 修订稿 | **等灰模**（2026-11-24）：PoC-SM 首批灰模已产出（`poc-sm` 分支 `evidence/poc-sm/`，7 视角） |

> **当前唯一未关闭的独立 BLOCKING 主项。**
> OQ-02 已于 2026-11-24 由 AUTH-08 关闭 → **PoC-SM 可以编码**。
> 但 **OQ-01 未关闭 → 几何验收与视觉判定仍不可执行，任何指标不得判 PASS**。
> DEC-05 / Canonical Acceptance Sheet 的取值**没有默认值，AI 不代填**。

### 1.1 已关闭的阻塞性主项（DONE）

| ID | 问题 | 状态 | Closure Evidence |
|---|---|---|---|
| OQ-02 | AUTH-08 的基线 commit 与 worktree 路径？ | **DONE**（2026-11-24） | `10-decision-records/04-AUTH-08-granted.md`：基线 `a32251b` / worktree `E:/AI-Tools/ProceduralAvatarEngine-poc-sm` / 分支 `poc-sm/shape-model-v2`；授权 `GRANTED` |
| OQ-03 | ≤5° 肩根接缝失败在 PoC-SM 前算阻塞还是可接受风险？ | **DONE**（2026-11-24） | `10-decision-records/01-OQ-03-decision-seam-hard-condition.md`：结论 = **硬条件**（阻止几何 PASS，不阻止编码）；测量口径已固定并写入 SPEC §13 #7 |
| OQ-04 | ADR-011 / ADR-012 是否签署？ | **DONE**（2026-11-24） | `10-decision-records/02-OQ-04-decision-adr-signoff.md`：ADR-011 = `ACCEPTED`；ADR-012 = `ACCEPTED — RESEARCH ONLY` |
| OQ-05 | G3 决策记录是否按 `DEFER` 签署？ | **DONE**（2026-11-24） | `10-decision-records/03-OQ-05-decision-g3-defer.md` + `05-g3-decision-record.md`：`DEFER`，`approved route` 保持空 |

## 2. OQ-01 的子项（DEC-05 参考体系与协议输入）

| ID | Parent | 问题 | Owner | Blocking Target | Evidence | Status | Closure Criteria | 负责人回复 |
|---|---|---|---|---|---|---|---|---|
| OQ-28 | OQ-01 | Canonical Acceptance Sheet 走哪条路径（A 负责人提供自有图 / B AI 制作比例与体积关系基准 / C 组合）？体型原型选 A/B/C？是否做风格化调整？ | 项目负责人 | `G-P1-GEO` 判 PASS | 本轮 03 §2、§8.6 | **BLOCKING / OPEN** | 路径 + 原型 + 调整系数三项均确定 | ____________________ |
| OQ-29 | OQ-01 | Layer B 剩余 3 组拓扑参考（R-C01 Meshy / R-C03 CG Topology / R-C05 mutante28 T-Pose）的原始链接与图片 | 项目负责人 | Layer B 完整性 | 本轮 02 §3.4 | **PARTIAL / OPEN** | 链接提交后完成核验并入矩阵（R-B02/R-B10/R-B11/R-B12 已核验） | ____________________ |
| OQ-30 | OQ-01 | A-Pose 与 bind pose 的确切姿态定义（手臂下斜角度） | 项目负责人 | 姿态轴协议；`G-P1-ANIM` | 本轮 03 §5、§5.1（45°/60°/75° 候选） | **OPEN** | 角度值确定并写入协议 | ____________________ |
| OQ-31 | OQ-01 | 相机高度绑定方式：解剖轴（hips→chest 中点）vs 固定世界 Y | 项目负责人 | 图像协议一致性 | 本轮 03 §4.1 | **OPEN** | 选定其一并固定 | ____________________ |
| OQ-32 | OQ-01 | 必检动作姿态集（建议肘屈 90° / 肩前屈 90° / 髋屈 90° / 膝屈 90°） | 项目负责人 | `G-P1-ANIM` 判据 | 本轮 03 §5 | **OPEN** | 姿态集确定 | ____________________ |

> OQ-30 虽未关闭，**AUTH-08 的 AL-7（姿态轴机制）已获授权**；AL-7 只授权"机制"，**不预设角度值**。

## 3. OQ-02 的子项（AUTH-08 范围）

| ID | Parent | 问题 | Owner | Blocking Target | Evidence | Status | Closure Criteria | 负责人回复 |
|---|---|---|---|---|---|---|---|---|
| OQ-33 | OQ-02 | 姿态轴协议扩展（`applyPose` + `band = none \| torso`）是否纳入 AUTH-08 实施范围？ | 项目负责人 | `G-P1-ANIM` 能否产出证据 | 本轮 03 §5.2；本轮 08 §4 | **DONE**（2026-11-24） | — | **纳入 AL-7**（见 `10-decision-records/04-AUTH-08-granted.md` §2） |

## 4. 独立主项（非 DEC-05 / 非 AUTH-08）

| ID | Parent | 问题 | Owner | Blocking Target | Evidence | Status | Closure Criteria | 负责人回复 |
|---|---|---|---|---|---|---|---|---|
| OQ-27 | — | 五 Gate 分离是否写入 SPEC Gate 链（GATE-04 之后的细分验收序列）？ | 项目负责人 | SPEC 结构；验收可审计性 | 本轮 05 | **OPEN** | 负责人确认是否入 SPEC | ____________________ |
| OQ-34 | — | "高品质视觉目标 ≠ 高面数"是否写成可核验的评审规则（如面数上限 / 不得以面数换视觉）？ | 项目负责人 | 防止工程目标被静默稀释 | 本轮 09 | **OPEN** | 写入或明确不写 | ____________________ |
| OQ-35 | — | `G-FINAL-VIS` 的材质与光照基线由谁定义？设备性能预算（目标 FPS / 生成耗时 / 内存）？ | 项目负责人 | `G-FINAL-VIS`；性能判定 | 本轮 05、09 | **OPEN** | 基线与预算值确定 | ____________________ |

## 5. 待审核人回答（NOT RUN — 不阻塞文档推进，建议 G3 重新批准前收回）

| ID | Parent | 问题 | Owner | Blocking Target | Evidence | Status | Closure Criteria |
|---|---|---|---|---|---|---|---|
| OQ-06 | — | E1/E2/E3 归因是否成立？"R3 误差在曲面不在投影"有无被采样口径污染？ | 审核人 | 归因结论可信度 | `02-route-review-prompt.md` Q1 | NOT RUN | 审核人签署复核 |
| OQ-07 | — | SPEC §8.1 八项对称性哪一项实际没做到？ | 审核人 | 对称性契约合规 | 同上 Q2 | NOT RUN | 同上 |
| OQ-08 | — | R1/R2 的"自比为 0"解释是否诚实？ | 审核人 | E1 解释 | 同上 Q3 | NOT RUN | 同上 |
| OQ-09 | — | `TopologyAudit` 的 crop/裂缝区分是否可靠？肩根是否为连续连接？ | 审核人 | `G-P1-TOPO` 结论 | 同上 Q4 | NOT RUN | 同上 |
| OQ-10 | — | 独立看 63 张灰模图，有无数值没抓到的一票否决缺陷？ | 审核人 | 视觉结论 | 同上 Q6 | NOT RUN | 同上 |
| OQ-11 | — | 有无 §13 TBD 指标被误判 PASS？ | 审核人 | 阈值纪律 | 同上 Q7 | NOT RUN | 同上 |
| OQ-12 | — | `SdfFusion.cpp` 符号推导、`SkinWeights.cpp` 生产等价性、灰模相机一致性是否正确？ | 审核人 | 代码级复核 | 同上 Q9 | NOT RUN | 同上 |
| OQ-13 | — | 是否同意 R2 作为提交签署的候选路线？ | 审核人 | 路线建议 | 同上 Q10 | NOT RUN | 同上 |

> OQ-06 与 OQ-08 直接关系到 G3 已签署证据（E1 自比值、R3 归因）的可信度，
> **不阻塞 G3 的 `DEFER` 状态，但阻塞 G3 的重新批准**。

## 6. 技术/测量缺口（OPEN — 需排期，不阻塞文档批准）

| ID | Parent | 问题 | Owner | Blocking Target | Evidence | Status | Closure Criteria |
|---|---|---|---|---|---|---|---|
| OQ-14 | — | 肩根接缝 ≤5° 合同失败（R1/R2 47.66°、R3 16.13°） | AI + 负责人 | `G-P1-GEO` 硬条件 | G2 §5；**PoC-SM v2 首测 21.76°（32 段）**，见 `12-poc-sm-vertical-slice.md` §3/§4 | **OPEN + 新发现待裁定** | PoC-SM 按固定口径复测 ≤5°。**⚠️ 2026-11-24 新发现**：本口径测的是环带的多边形离散化（完美圆柱在 32 段下也读 11.25°，excess ≈ 370/S），与 `docs/foundation/03` "缝两侧法线夹角 ≤5°" 的跨缝语义不一致。三个出路（扣基线 / 改测跨缝 / 提段数至 ~150）**需负责人裁定**；裁定前 21.76° 不作判定依据 |
| OQ-15 | — | R3 包络上胸缺口 13.5 cm、有效参数 7/20 且加密不收敛 | AI | 无（已暂停） | G1/G2 | **CLEAR（暂停投入）** | 保持暂停；**不阻止 PoC-SM** |
| OQ-16 | — | 骨盆扇盖处 144 个法线翻转（ring 0–1） | AI | `G-P1-GEO` 复测 | G2 §4 | **OPEN** | PoC-SM 复测并区分 cap 伪影与真实缺陷 |
| OQ-17 | — | 跨参数组区域影响：R2 chest 组 33 顶点 >1 mm；R3 waist 组 250 顶点 >1 mm（max 14.86 mm） | AI | `G-P1-GEO` + `G-P1-TOPO` | G2 §13 #9 | **OPEN** | 阈值批准后判定；加入逐参数差分测试（AL-6） |
| OQ-18 | — | 体积对比非同口径（R1 闭合、R2/R3 crop 开口，垂直范围不同） | AI | G2 重测 | G2 §7 注 | **OPEN** | 新 G2 之前统一测量口径 |
| OQ-19 | — | GPU 数值 parity `[UNVERIFIED]` | AI | G4；`G-P1-ANIM` 完整判定 | G2 §6 | **OPEN** | G4 前专项完成；**CPU 侧已成立，不可替代 GPU 结论** |
| OQ-20 | — | APK 哈希因 `SdfFusion.cpp` 改动变化（`088476fc…` → `c1cace9d…`），设备复验 NOT RUN | AI | G4 | 符号推导文档 | **OPEN** | G4 前设备复验 |
| OQ-21 | — | 参数"声明未达顶点"仍是 PARTIALLY VERIFIED | AI | 参数语义可信度 | G2 §8 | **OPEN** | 逐参数差分测试通过后升级 VERIFIED（AL-6） |

## 7. 已关闭（DONE）

| ID | Parent | 事项 | 关闭证据 | Status |
|---|---|---|---|---|
| OQ-22 | — | G3 是否 DEFER / R1·R2·R3 如何处置 | 负责人 2026-11-23 采纳；SPEC §7.4 | DONE |
| OQ-23 | — | 是否采纳 09 分层契约 | DEC-09 = APPROVED；SPEC §7.1 | DONE |
| OQ-24 | — | PoC-A 命名冲突 | 改名 PoC-R / PoC-SM / PoC-SHOULDER / PoC-LIMB / PoC-FULL | DONE |
| OQ-25 | — | Saltline 是否单独立 ADR | ADR-012 草案，不引入 Three.js | DONE |
| OQ-26 | — | smooth-union 符号是否猜改 | 先推导 + 双实现单测（修复前 212 断言失败），再改 | DONE |

> 编号说明：OQ-27…OQ-35 原为第 1 轮草稿中的并行编号，现已并入本表统一编号空间
> （对照表见 `09-blocker-closure-auth08/04-oq-numbering-merge.md` §3.1）。
> 本表**不再有第二个编号空间**。

## 8. 同步状态

- 本表与全部证据/文档已在 `origin/feature/procedural-character-foundation`。
- 本版变更（2026-11-24，第 2 次）：① OQ-01 记录负责人"等灰模再定"的排期决定，并注明首批灰模已产出；
  ② OQ-14 记录 PoC-SM v2 首测值（21.76°）与**口径与合同原文语义不一致**的待裁定发现。
- PoC-SM 首个垂直切片见 `12-poc-sm-vertical-slice.md`（`poc-sm/shape-model-v2` 分支）。
- 回复请直接填 ID（如 "OQ-01: xxx，OQ-28: xxx"），我会写回 SPEC register 并更新本表状态。
- ⚠️ **文档完成 / 入库 / AI 声称完成 / Git 工作区干净，均不等于批准，也不等于任何 Gate 通过。**
