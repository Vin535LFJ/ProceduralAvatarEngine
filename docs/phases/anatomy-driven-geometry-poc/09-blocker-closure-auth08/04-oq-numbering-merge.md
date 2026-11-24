# 04 — OQ 编号归并表

> 状态：**合并方案 / PROPOSED**。本文件**不修改**仓库内 `07-open-issues.md`；
> §4 是该文件的**完整替换稿**，待负责人批准后再写回仓库（写回需一次 commit，本轮未执行）。
> 目的：满足"所有问题只有一个正式编号、禁止并行维护多个 Register"。

---

## 1. OQ-27 是否存在？—— 核查结果

`[VERIFIED]` **在正式 Register 中不存在。**

| 载体 | 是否含 OQ-27 | 证据 |
|---|---|---|
| 仓库内 `docs/phases/anatomy-driven-geometry-poc/07-open-issues.md`（正式 Register） | **否** | 该表编号连续为 OQ-01…OQ-26，其中 OQ-22…OQ-26 位于 §4「已关闭（DONE）」。**OQ-27 空缺。** |
| 仓库外草稿 `Deliverable-5-open-questions.md` §B | **是** | 草稿自行新增 OQ-27…OQ-35 共 9 条 |

`[VERIFIED]` 因此当前的真实状况是：**正式 Register = OQ-01…OQ-26；草稿另起一套 OQ-27…OQ-35。两套编号并存，且草稿的 OQ-28（Canonical Sheet）与正式 OQ-01（DEC-05 参考图来源）指向同一个 DEC-05 问题；草稿的 OQ-33（姿态轴是否入 AUTH-08）与正式 OQ-02（AUTH-08 范围）指向同一个 AUTH-08 问题。**

`[INFERENCE]` 这正是负责人指出的"复制两个阻塞项"问题：同一个 DEC-05 决策被登记为两个 BLOCKING，会让阻塞计数虚高一倍。

---

## 2. 归并原则

| # | 原则 | 落实方式 |
|---|---|---|
| P-1 | **一个正式编号一个问题** | 单一 Register，单一编号空间，无重号、无空缺 |
| P-2 | **父子关系显式化，不复制阻塞项** | 新增 `Parent` 列。**父项的 Closure Criteria = 子项全部关闭 + 负责人签字**。子项**不单独计为 BLOCKING**，阻塞计数不增加 |
| P-3 | **OQ-02 / AUTH-08 同一主线** | OQ-02 为 AUTH-08 主项；所有 AUTH-08 范围子决策（OQ-33）挂为子项 |
| P-4 | 保留既有编号稳定 | OQ-01…OQ-26 编号**不变**，仅补充 Owner / Blocking Target / Evidence / Closure Criteria 四列与 Parent 列 |
| P-5 | 新增问题接入同一编号空间 | OQ-27…OQ-35 保留其编号（OQ-27 本就空缺，无冲突），全部并入本表并建立父子关系 |

---

## 3. 新旧 OQ 对照表

### 3.1 草稿 OQ-27…OQ-35 → 正式 Register 映射

| 草稿编号 | 处置 | 正式编号 | Parent | 说明 |
|---|---|---|---|---|
| OQ-27 五 Gate 是否入 SPEC Gate 链 | **保留**，升为独立主项 | **OQ-27** | — | 独立决策，不隶属 DEC-05 |
| OQ-28 Canonical Sheet 走 A/B/C | **并入 OQ-01**，建立父子 | **OQ-28** | **OQ-01** | 同一 DEC-05 问题，不再单列为阻塞 |
| OQ-29 Layer B 剩余拓扑链接 | **并入 OQ-01**，建立父子 | **OQ-29** | **OQ-01** | 同上 |
| OQ-30 A-Pose / bind pose 角度 | **并入 OQ-01**（rubric 协议决策） | **OQ-30** | **OQ-01** | 姿态轴属验收协议定义，与 rubric 同源 |
| OQ-31 相机高度绑定方式 | **并入 OQ-01** | **OQ-31** | **OQ-01** | 同上 |
| OQ-32 必检动作姿态集 | **并入 OQ-01** | **OQ-32** | **OQ-01** | 同上 |
| OQ-33 姿态轴是否入 AUTH-08 | **并入 OQ-02**，建立父子 | **OQ-33** | **OQ-02** | 同一 AUTH-08 范围主线 |
| OQ-34 "非高面数"可核验规则 | **保留**，升为独立主项 | **OQ-34** | — | 独立决策 |
| OQ-35 G-FINAL-VIS 材质/光照基线 + 性能预算 | **保留**，升为独立主项 | **OQ-35** | — | 独立决策 |

### 3.2 正式 Register 中需要补充列的既有条目

| 正式编号 | 补充内容 |
|---|---|
| OQ-01 | Parent 置空；Blocking Target 改为"参考体系 C 的批准 + 五 Gate 结构进入 rubric + 其 5 个子项关闭"；Closure Criteria = OQ-28/29/30/31/32 全部关闭 **且** 负责人签字 |
| OQ-02 | Blocking Target 改为"AUTH-08 授权书签署（基线 + worktree 路径 + 实施范围）"；Closure Criteria = 负责人给出基线与路径 **且** 签署 AUTH-08 **且** OQ-33 有明确结论 |
| OQ-03 | 按本轮 07 的收敛建议改写为**硬条件**（不阻止编码、阻止几何 PASS） |
| OQ-05 | 按本轮 07 的收敛建议改写为**DEFER + approved route 保持空 + 四项补证** |

### 3.3 阻塞计数变化（关键）

| 口径 | 合并前（正式 + 草稿相加） | 合并后 |
|---|---:|---:|
| 独立 BLOCKING 主项 | 5（OQ-01…05）+ 9（OQ-27…35 中的阻塞项）= **最多 14** | **5**（OQ-01…05） |
| 其中 DEC-05 相关 | OQ-01 + OQ-28 + OQ-29 = 3 条重复 | **1**（OQ-01，含 5 个子项） |
| 其中 AUTH-08 相关 | OQ-02 + OQ-33 = 2 条重复 | **1**（OQ-02，含 1 个子项） |

`[VERIFIED]` 合并后**真正需要负责人签字才能推进的主项仍是 5 个**，与合并前的正式 Register 一致；草稿里的 OQ-28/29/33 被降级为子项输入，不再虚增阻塞计数。

---

## 4. 单一 Register 替换稿（`07-open-issues.md` 完整内容，待批准后写回）

> 以下内容可直接替换 `docs/phases/anatomy-driven-geometry-poc/07-open-issues.md`。
> 状态定义扩展：`OPEN` / `BLOCKING` / `NOT RUN` / `PARTIAL` / `CLEAR` / `DONE`。

```markdown
# Open Issues / Questions Register — G2 冻结后

> 目的：把散落在 G2 报告、rubric、G3 记录、ADR 草案、DEC-05 参考体系里的待决问题
> 收成**唯一一份**可跟踪清单。
> G2 证据基线：`a32251b`；当前远端：`7c451ae`（本表上一版本误记为 `f792d52`）。
> 状态：`OPEN`（待决定）/ `BLOCKING`（阻塞后续阶段）/ `NOT RUN`（未执行）/
> `PARTIAL`（部分关闭）/ `CLEAR`（已澄清）/ `DONE`（已关闭）。
> **父子项**：父项的 Closure Criteria 包含其子项全部关闭。子项**不单独计为 BLOCKING**。

## 1. 阻塞性主项（BLOCKING — 负责人必须签字）

| ID | Parent | 问题 | Owner | Blocking Target | Evidence | Status | Closure Criteria | 负责人回复 |
|---|---|---|---|---|---|---|---|---|
| OQ-01 | — | DEC-05 参考体系：是否批准方案 C（外部视觉参考 + 解剖工程参考 + 自有 Canonical Acceptance Sheet），并把五 Gate 结构写入 rubric？ | 项目负责人 | 视觉 `PASS` 判定；`G-P1-GEO` / `G-FINAL-VIS` 能否判 PASS | 本轮 02 参考更新矩阵；本轮 03 Canonical Sheet v0 | **BLOCKING / OPEN** | ① 子项 OQ-28/29/30/31/32 全部关闭 ② 负责人签署 DEC-05 修订稿 | ____________________ |
| OQ-02 | — | AUTH-08（PoC-SM / Shape Model v2）：授权基线 commit、worktree 路径、实施范围 | 项目负责人 | PoC-SM 代码实施 | 本轮 08 授权书 + 路径候选 | **BLOCKING / OPEN** | ① 负责人指定基线（建议 `a32251b`）与 worktree 路径 ② 签署 AUTH-08 ③ 子项 OQ-33 有结论 | ____________________ |
| OQ-03 | — | 肩根接缝 ≤5° 的性质：硬条件（阻止几何 PASS，不阻止编码）？ | 项目负责人 | `G-P1-GEO` 判定；G2 重开；G3 重新批准 | 本轮 07 收敛建议；SPEC §13（既有合同值） | **BLOCKING / OPEN** | 负责人签字确认"≤5° 为 PoC-SM 几何验收硬条件，且不阻止 PoC-SM 开始编码" | 签字 ______ |
| OQ-04 | — | ADR-011（Shape Model 契约）/ ADR-012（Saltline 研究）是否签署？ | 项目负责人 | PoC-SM 契约依据；Saltline 研究边界 | 本轮 06 签署稿 | **BLOCKING / DRAFT** | 负责人签署两份 ADR | 签字 ______ 日期 ______ |
| OQ-05 | — | G3 决策记录是否按 `DEFER` 签署？ | 项目负责人 | 重开 G2 的正式起点；G4 启动条件 | 本轮 07 签署稿 | **BLOCKING / DRAFT** | 负责人签署 G3 DEFER 记录，`approved route` 保持空 | 签字 ______ 日期 ______ |

> OQ-01 与 OQ-02 没有默认值，AI 不代填。在此之前不写 PoC-SM 代码、不宣布任何视觉合格。

## 2. OQ-01 的子项（DEC-05 参考体系与协议输入）

| ID | Parent | 问题 | Owner | Blocking Target | Evidence | Status | Closure Criteria | 负责人回复 |
|---|---|---|---|---|---|---|---|---|
| OQ-28 | OQ-01 | Canonical Acceptance Sheet 走哪条路径（A 负责人提供自有图 / B AI 制作比例与体积关系基准 / C 组合）？体型原型选 A/B/C？是否做风格化调整？ | 项目负责人 | `G-P1-GEO` 判 PASS | 本轮 03 §2、§8.6 | **BLOCKING / OPEN** | 路径 + 原型 + 调整系数三项均确定 | ____________________ |
| OQ-29 | OQ-01 | Layer B 剩余 3 组拓扑参考（R-C01 Meshy / R-C03 CG Topology / R-C05 mutante28 T-Pose）的原始链接与图片 | 项目负责人 | Layer B 完整性 | 本轮 02 §3.4 | **PARTIAL / OPEN** | 链接提交后完成核验并入矩阵（R-B10/11/12、R-B02 已核验） | ____________________ |
| OQ-30 | OQ-01 | A-Pose 与 bind pose 的确切姿态定义（手臂下斜角度） | 项目负责人 | 姿态轴协议；`G-P1-ANIM` | 本轮 03 §5、§5.1（45°/60°/75° 候选） | **OPEN** | 角度值确定并写入协议 | ____________________ |
| OQ-31 | OQ-01 | 相机高度绑定方式：解剖轴（hips→chest 中点）vs 固定世界 Y | 项目负责人 | 图像协议一致性 | 本轮 03 §4.1 | **OPEN** | 选定其一并固定 | ____________________ |
| OQ-32 | OQ-01 | 必检动作姿态集（建议肘屈 90° / 肩前屈 90° / 髋屈 90° / 膝屈 90°） | 项目负责人 | `G-P1-ANIM` 判据 | 本轮 03 §5 | **OPEN** | 姿态集确定 | ____________________ |

## 3. OQ-02 的子项（AUTH-08 范围）

| ID | Parent | 问题 | Owner | Blocking Target | Evidence | Status | Closure Criteria | 负责人回复 |
|---|---|---|---|---|---|---|---|---|
| OQ-33 | OQ-02 | 姿态轴协议扩展（`applyPose` + `band = none`）是否纳入 AUTH-08 实施范围？ | 项目负责人 | `G-P1-ANIM` 能否产出证据 | 本轮 03 §5.2；本轮 08 §4 | **OPEN** | 纳入 / 排除有明确结论 | ____________________ |

## 4. 独立主项（非 DEC-05 / 非 AUTH-08）

| ID | Parent | 问题 | Owner | Blocking Target | Evidence | Status | Closure Criteria | 负责人回复 |
|---|---|---|---|---|---|---|---|---|
| OQ-27 | — | 五 Gate 分离是否写入 SPEC Gate 链（GATE-04 之后的细分验收序列）？ | 项目负责人 | SPEC 结构；验收可审计性 | 本轮 05 | **OPEN** | 负责人确认是否入 SPEC | ____________________ |
| OQ-34 | — | "高品质视觉目标 ≠ 高面数"是否写成可核验的评审规则（如面数上限 / 不得以面数换视觉）？ | 项目负责人 | 防止工程目标被静默稀释 | 本轮 09 | **OPEN** | 写入或明确不写 | ____________________ |
| OQ-35 | — | `G-FINAL-VIS` 的材质与光照基线由谁定义？设备性能预算（目标 FPS / 生成耗时 / 内存）？ | 项目负责人 | `G-FINAL-VIS`；性能判定 | 本轮 05、09 | **OPEN** | 基线与预算值确定 | ____________________ |

## 5. 待审核人回答（NOT RUN — 不阻塞文档推进，建议 G3 签署前收回）

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

## 6. 技术/测量缺口（OPEN — 需排期，不阻塞文档批准）

| ID | Parent | 问题 | Owner | Blocking Target | Evidence | Status | Closure Criteria |
|---|---|---|---|---|---|---|---|
| OQ-14 | — | 肩根接缝 ≤5° 合同失败（R1/R2 47.66°、R3 16.13°） | AI + 负责人 | `G-P1-GEO` 硬条件 | G2 §5 | **OPEN** | PoC-SM 几何验收前复测 ≤5°，并固定测量/边界/法线口径 |
| OQ-15 | — | R3 包络上胸缺口 13.5 cm、有效参数 7/20 且加密不收敛 | AI | 无（已暂停） | G1/G2 | **CLEAR（暂停投入）** | 保持暂停；**不阻止 PoC-SM** |
| OQ-16 | — | 骨盆扇盖处 144 个法线翻转（ring 0–1） | AI | `G-P1-GEO` 复测 | G2 §4 | **OPEN** | PoC-SM 复测并区分 cap 伪影与真实缺陷 |
| OQ-17 | — | 跨参数组区域影响：R2 chest 组 33 顶点 >1 mm；R3 waist 组 250 顶点 >1 mm（max 14.86 mm） | AI | `G-P1-GEO` + `G-P1-TOPO` | G2 §13 #9 | **OPEN** | 阈值批准后判定；加入逐参数差分测试 |
| OQ-18 | — | 体积对比非同口径（R1 闭合、R2/R3 crop 开口，垂直范围不同） | AI | G2 重测 | G2 §7 注 | **OPEN** | 新 G2 之前统一测量口径 |
| OQ-19 | — | GPU 数值 parity `[UNVERIFIED]` | AI | G4；`G-P1-ANIM` 完整判定 | G2 §6 | **OPEN** | G4 前专项完成；**CPU 侧已成立，不可替代 GPU 结论** |
| OQ-20 | — | APK 哈希因 `SdfFusion.cpp` 改动变化（`088476fc…` → `c1cace9d…`），设备复验 NOT RUN | AI | G4 | 符号推导文档 | **OPEN** | G4 前设备复验 |
| OQ-21 | — | 参数"声明未达顶点"仍是 PARTIALLY VERIFIED | AI | 参数语义可信度 | G2 §8 | **OPEN** | 逐参数差分测试通过后升级 VERIFIED |

## 7. 已关闭（DONE）

| ID | Parent | 事项 | 关闭证据 | Status |
|---|---|---|---|---|
| OQ-22 | — | G3 是否 DEFER / R1·R2·R3 如何处置 | 负责人 2026-11-23 采纳；SPEC §7.4 | DONE |
| OQ-23 | — | 是否采纳 09 分层契约 | DEC-09 = APPROVED；SPEC §7.1 | DONE |
| OQ-24 | — | PoC-A 命名冲突 | 改名 PoC-R / PoC-SM / PoC-SHOULDER / PoC-LIMB / PoC-FULL | DONE |
| OQ-25 | — | Saltline 是否单独立 ADR | ADR-012 草案，不引入 Three.js | DONE |
| OQ-26 | — | smooth-union 符号是否猜改 | 先推导 + 双实现单测（修复前 212 断言失败），再改 | DONE |

## 8. 同步状态

- 本文与全部证据/文档已在 `origin/feature/procedural-character-foundation`，HEAD `7c451ae`。
- **本替换稿尚未写回仓库**；写回需负责人批准 + 一次 commit。
- 回复请直接填 ID（如 "OQ-01: xxx，OQ-02: xxx"），我会写回 SPEC register 并更新本表状态。
```

---

## 5. 需要负责人确认的归并决策

| # | 决策 | AI 建议 |
|---|---|---|
| M-1 | 是否接受 OQ-28…OQ-32 降为 OQ-01 的子项（不单列阻塞）？ | 接受 —— 阻塞计数从最多 14 回到 5 |
| M-2 | 是否接受 OQ-33 降为 OQ-02 的子项？ | 接受 —— 同一 AUTH-08 范围主线 |
| M-3 | 是否接受 OQ-27 / OQ-34 / OQ-35 保持为 3 个独立主项？ | 接受 —— 三者互不隶属 |
| M-4 | 是否用 §4 的替换稿覆盖仓库内 `07-open-issues.md`？ | 覆盖 —— 现有版本缺 Owner / Blocking Target / Evidence / Closure Criteria 四列，且远端 SHA 记录已过期 |
| M-5 | 是否需要保留旧编号别名以便历史文档追溯（如 OQ-28 旧指 DEC-05）？ | 建议保留 §3.1 对照表在本文档内，Register 本身只留单一编号 |

`[OPEN QUESTION]` M-1…M-5 需负责人确认后写回。
