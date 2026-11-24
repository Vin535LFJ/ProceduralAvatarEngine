# 10 — 决策记录归档（2026-11-24 签署）

> 本目录收录 **2026-11-24** 由项目负责人签署的决策记录。
> **签署 ≠ 实施完成 ≠ Gate 通过。** 本目录不改变任何 Gate 的判定状态。

---

## 1. 这批记录解决什么问题

第 2 轮（`../09-blocker-closure-auth08/`）把 6 个"材料齐备可签"的主项整理成签署稿，
但签字栏留空。2026-11-24 负责人在 WorkBuddy 会话中明确选择「签 4 份 + 下发 AUTH-08」执行包，
并选定「新建独立 worktree」落地形态。本目录把该决定的**可审计留痕**固化进仓库。

---

## 2. 决策记录清单

| 文件 | 决策项 | 结果 | 关闭的 OQ |
|---|---|---|---|
| `01-OQ-03-decision-seam-hard-condition.md` | ≤5° 肩根接缝的性质与测量口径 | **硬条件**：阻止几何 PASS，不阻止编码 | **OQ-03** |
| `02-OQ-04-decision-adr-signoff.md` | ADR-011 / ADR-012 是否签署 | **ADR-011 = ACCEPTED；ADR-012 = ACCEPTED — RESEARCH ONLY** | **OQ-04** |
| `03-OQ-05-decision-g3-defer.md` | G3 决策记录是否按 DEFER 签署 | **`DEFER`**，`approved route` 保持空 | **OQ-05** |
| `04-AUTH-08-granted.md` | PoC-SM 实施授权 | **`GRANTED`**（基线 `a32251b` / worktree `E:/AI-Tools/ProceduralAvatarEngine-poc-sm` / 分支 `poc-sm/shape-model-v2` / OQ-33 纳入） | **OQ-02** + **OQ-33** |

### 相关的正式文件（不在本目录，但由本批决策产生）

| 文件 | 说明 |
|---|---|
| `docs/architecture-decisions/ADR-011-shape-model-contract.md` | ADR-011 正式版（ACCEPTED） |
| `docs/architecture-decisions/ADR-012-saltline-module-study.md` | ADR-012 正式版（ACCEPTED — RESEARCH ONLY） |
| `docs/phases/anatomy-driven-geometry-poc/05-g3-decision-record.md` | G3 Decision Record 正式版（DEFER，已签署） |
| `docs/phases/anatomy-driven-geometry-poc/07-open-issues.md` | 单一 Register（已归并 + 已更新状态） |
| `docs/phases/anatomy-driven-geometry-poc/00-implementation-spec.md` | §7 Register / §13 #7 已同步 |

> 2026-11-23 的 `.draft.md` 版本由 Git 历史保留（提交 `1ce1a21`）。

---

## 3. 状态变化总表

| 项 | 签署前 | 签署后 |
|---|---|---|
| OQ-02（AUTH-08） | `BLOCKING / OPEN` | **`DONE`** |
| OQ-03（≤5° 性质） | `BLOCKING / OPEN` | **`DONE`** |
| OQ-04（ADR 签署） | `BLOCKING / DRAFT` | **`DONE`** |
| OQ-05（G3 DEFER） | `BLOCKING / DRAFT` | **`DONE`**（状态 `DEFER`） |
| OQ-33（姿态轴入 AUTH-08） | `OPEN` | **`DONE`**（纳入 AL-7） |
| OQ-01（DEC-05 家族） | `BLOCKING / OPEN` | **`BLOCKING / OPEN`（未变）** |
| ADR-011 / ADR-012 | `DRAFT` | **`ACCEPTED`** / **`ACCEPTED — RESEARCH ONLY`** |
| GATE-04 | `DEFER`（未签字） | **`DEFER`（已签署）**，`approved route` 留空 |
| AUTH-08 | `OPEN — 未授权` | **`GRANTED`** |

**独立 BLOCKING 主项：5 → 1**（只剩 OQ-01 家族）。

---

## 4. ⚠️ 仍未关闭 / 仍未达成的事

| 项 | 状态 | 影响 |
|---|---|---|
| **OQ-01 及其子项 OQ-28/29/30/31/32** | `BLOCKING / OPEN` | **几何验收与视觉判定仍不可执行**；G3 四项补证之 ① 未达成 |
| `G-P1-GEO` | **FAIL** | R1/R2 47.66°、R3 16.13°，均 > 5° |
| `G-P1-TOPO` | R2 PASS / R3 待复测 | — |
| `G-P1-ANIM` | PARTIAL（CPU 22/22；GPU parity `UNVERIFIED`） | — |
| `G-P2-MOD` / `G-FINAL-VIS` | `NOT RUN` | — |
| **G4** | **不可启动** | SPEC 要求 G3 已签署**且**视觉 Gate 通过 |
| OQ-06…OQ-13 | `NOT RUN` | 审核人意见；建议在 G3 **重新批准**前收回 |
| OQ-27 / OQ-34 / OQ-35 | `OPEN` | 五 Gate 是否入 SPEC / 面数可核验规则 / 材质光照基线与性能预算 |
| §13 其余阈值 | `TBD` | **DEC-05 未批准前不得判 PASS** |

---

## 5. 签署方式与诚实性声明

`[VERIFIED]` 本批决策记录由 AI **转录与归档**，具体做法：

1. 负责人在本会话中以**明确选项**表达决定（选择「签 4 份 + 下发 AUTH-08」执行包、
   选择「新建独立 worktree」落地形态）；
2. AI 将该决定**逐字对齐**到第 2 轮已审定的签署稿建议内容，**未增删任何负责人未表达的判断**；
3. 每份记录都带「决策记录」表，写明**决策人、日期、决策来源、记录方式**；
4. 签字栏的原始占位符**保留**在正文中，并在其后追加「签署结果」行 —— 便于与原始稿逐字对照。

**AI 未在任何文件中伪造手写签名，未代负责人作出判断，未代填 `approved route`。**

> 文档完成 / 入库 / AI 声称完成 / Git 工作区干净 **均不等于** 批准或 Gate 通过。

---

## 6. 关联目录

| 目录 | 内容 |
|---|---|
| `../08-dec05-visual-direction-review/` | 第 1 轮：视觉方向、参考体系、rubric 修订（**部分已被第 2 轮取代**，见其 README §2） |
| `../09-blocker-closure-auth08/` | 第 2 轮：真实状态审计、参考更新矩阵、Canonical Sheet v0、OQ 归并表、五 Gate Matrix、签署稿、AUTH-08 候选、技术缺口计划、就绪结论 |
| `../10-decision-records/` | **本目录**：2026-11-24 签署的决策记录 |
