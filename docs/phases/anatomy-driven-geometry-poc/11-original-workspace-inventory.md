# 11 — 原始工作区未提交改动盘点（决策输入）

> **状态：READ-ONLY 盘点结果。** 本文件**未删除、未修改、未提交**原始工作区的任何内容。
> 原始工作区：`C:/Users/Giant/Desktop/GITCODE/Procedural Avatar Engine`
> 盘点日期：2026-11-24 · 盘点时该工作区 HEAD = `99941a94ec58fb37ab0f1a36b90590d3fdce5198`
> 分支 `feature/procedural-character-foundation`（比远端落后 **6 个 commit**）。

---

## 1. 结论（一句话）

`[VERIFIED]` 原始工作区的全部未提交改动 **不含任何唯一内容** ——
**10 份未跟踪草稿、3 份被删 spec、1 份被改写的 README，全部已存在于 Git 提交中**（字节一致）。
该工作树状态是一次**设计包 zip 被解压到仓库根目录**造成的副产物，不是待采纳的编辑成果。

---

## 2. 逐项盘点

### 2.1 10 份未跟踪草稿 → **纯冗余副本**

| 文件 | 字节 | 判定 | 依据 |
|---|---:|---|---|
| `00-current-state-audit.md` | 16 679 | **冗余** | 与已提交的 `docs/proposals/anatomy-driven-geometry-redesign/00-current-state-audit.md` **字节一致** |
| `01-visual-shape-requirements.md` | 8 493 | **冗余** | 同上（字节一致） |
| `02-anatomical-shape-model.md` | 9 303 | **冗余** | 同上（字节一致） |
| `03-geometry-representation-study.md` | 15 764 | **冗余** | 同上（字节一致） |
| `04-shoulder-and-joint-study.md` | 7 239 | **冗余** | 同上（字节一致） |
| `05-topology-and-rigging-design.md` | 11 300 | **冗余** | 同上（字节一致） |
| `06-target-architecture.md` | 9 701 | **冗余** | 同上（字节一致） |
| `07-poc-roadmap.md` | 8 362 | **冗余** | 同上（字节一致） |
| `08-risk-and-decision-register.md` | 6 742 | **冗余** | 同上（字节一致） |
| `09-final-recommendation.md` | 10 631 | **冗余** | 同上（字节一致） |

`[VERIFIED]` 逐文件 `cmp` 全部返回 **IDENTICAL**（10/10）。
这 10 份文件的**正确位置** `docs/proposals/anatomy-driven-geometry-redesign/` 已在 Git 中提交。

### 2.2 3 份被删除的 spec → **删除安全，内容已双重备份**

| 文件 | HEAD 中的大小 | 判定 | 依据 |
|---|---:|---|---|
| `01_Procedural_Character_Foundation_Specification_v1.0.md` | 27 453 B | **删除安全** | 与 `docs/archive/foundation-v1.0/Procedural_Character_Foundation_v1.0.zip` 内同名文件**字节一致**；且仍在 Git HEAD 中 |
| `02_Procedural_Character_Development_Roadmap_v1.0.md` | 15 642 B | **删除安全** | 同上（字节一致） |
| `03_AI_Engineering_Execution_Prompt_v1.0.md` | 11 930 B | **删除安全** | 同上（字节一致） |

`[VERIFIED]` 逐文件 `cmp`（`git show HEAD:<file>` vs zip 内文件）全部返回 **IDENTICAL**。
即：内容在 **归档 zip** 与 **Git 历史** 两处都在，删除不造成任何信息丢失。

### 2.3 被改写的 `README.md` → **被覆盖，非编辑**

| 项 | 值 |
|---|---|
| HEAD 中的 README | `# Procedural Character Foundation v1.0` —— 仓库项目索引，列出 3 份根级 spec + 设计包 + 归档索引 |
| 工作区中的 README | `# ProceduralAvatarEngine — Anatomy-driven Geometry Redesign` —— 设计包自身的索引（"本目录是……"） |
| 判定 | 工作区的 README 与**已提交的** `docs/proposals/anatomy-driven-geometry-redesign/README.md` **字节一致** |
| 净效果 | 仓库根的项目 README **被设计包 README 覆盖** |

`[VERIFIED]` 这是覆盖而非改写：工作区 README 的内容在 Git 中已存在（作为提案包的 README），
但**项目 README 的内容因此从工作区丢失**（仅存于 Git HEAD）。

### 2.4 反证：这是 zip 解压的副产物

`[VERIFIED]` 根目录已提交的 `ProceduralAvatarEngine-Anatomy-driven-Geometry-Redesign-v1.0.zip`
其内容为 `docs/proposals/anatomy-driven-geometry-redesign/{00-09,README}` 共 11 个 `.md`。
逐文件比对：**zip 内全部 11 份与原始工作区根目录的对应文件字节一致**（含 README）。

→ 原始工作区的改动形态 = **把设计包 zip 解压到了仓库根目录**。

---

## 3. 处置建议（**待负责人决定，AI 未执行**）

| # | 建议 | 理由 | 风险 |
|---|---|---|---|
| R-1 | **丢弃**原始工作区的全部未提交改动（`git checkout -- README.md` + `git restore` 恢复 3 份 spec + 删除 10 份未跟踪副本），然后 fast-forward 到 `bf7f854` | 改动**零唯一内容**，全部已提交；恢复后该工作区与远端一致 | 极低。唯一"丢失"的是"zip 曾被解压到根目录"这一事实，与仓库状态无关 |
| R-2 | **保留**现状不动，只把它当作"已知脏工作区"记录在案 | 零改动 | 该工作区将继续落后远端 6 个 commit，后续任何 pull 都是带脏状态的合并 |
| R-3 | 先在原始工作区跑一次 `git diff HEAD` 全量留档，再按 R-1 处理 | 想要审计痕迹 | 额外一步；本盘点已覆盖全部改动 |

`[VERIFIED]` **AI 未执行任何一条。** 本轮对原始工作区只做了只读操作
（`ls` / `git status` / `git diff` / `git show` / `git ls-files` / `unzip -l` / `cmp`），
**未删除、未修改、未 add、未 commit、未 checkout**。

---

## 4. 若决定 R-1，需要注意的两点

1. **原始工作区不能直接 `git pull`**：本地有未提交改动，且本地分支 `feature/procedural-character-foundation`
   在 `99941a9`（落后远端 `bf7f854` **6 个 commit**）。这 6 个 commit 包含 `a32251b` 的
   **代码与证据改动**（不只文档），因此 fast-forward 会真正更新 `native/`、`tests/`、`evidence/`。
2. **该工作区还有一个 `build/` 目录与 `local.properties`**（均为未跟踪/被忽略的本地产物）。
   恢复时不应删除它们；`local.properties` 含本机 SDK 路径，删除会导致本地构建失效。

---

## 5. 关联

| 文档 | 关系 |
|---|---|
| `09-blocker-closure-auth08/01-state-audit-report.md` | §5.5 记录的 4 个 docs-only 提交差异；本盘点扩展到原始工作区的未提交改动 |
| `10-decision-records/00-INDEX.md` | 本轮签署的决策记录索引 |
| `07-open-issues.md` | 单一 Register；本盘点**未**产生新的 OQ 主项 |
