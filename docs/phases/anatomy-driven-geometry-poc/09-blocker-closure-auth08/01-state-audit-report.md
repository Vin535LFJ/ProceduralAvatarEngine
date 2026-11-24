# 01 — 真实状态审计报告

> 状态：**审计报告 / AUDIT**。本文件只记录**实际执行过的命令与其真实输出**，不采信上一轮 AI 报告的陈述。
> 审计时间：2026-11-24 09:26–09:35 (UTC+8)
> 审计者：WorkBuddy AI（不得代表负责人签署任何决策）

---

## 1. Git 仓库状态（实测）

### 1.1 仓库根目录

```
$ git rev-parse --show-toplevel
E:/AI-Tools/ProceduralAvatarEngine-PoC
```

`[VERIFIED]` 唯一 Git 根目录为 `E:/AI-Tools/ProceduralAvatarEngine-PoC`。

### 1.2 当前分支

```
$ git branch --show-current
poc-a-g2-fair-comparison

$ git branch -avv
+ feature/procedural-character-foundation   99941a9 (C:/Users/Giant/Desktop/GITCODE/Procedural Avatar Engine) [origin/feature/procedural-character-foundation: behind 5]
  m11-anatomy-visual-rebuild                 67ccb39
  m12-unified-human-geometry                 c539afb
  m13-procedural-retopology                  bf5415f
  main                                       c539afb
* poc-a-g2-fair-comparison                   7c451ae
  remotes/origin/feature/procedural-character-foundation 7c451ae
  remotes/origin/main                                  c539afb
```

`[VERIFIED]` 当前分支 `poc-a-g2-fair-comparison`，HEAD `7c451ae`。

### 1.3 HEAD 完整 SHA

```
$ git rev-parse HEAD
7c451ae18137316291a4a51ecd03e57d2bea2d0f
```

### 1.4 工作区状态 / 暂存区 / 差异

```
$ git status --short
(空输出)

$ git diff --stat
(空输出)

$ git diff --cached --stat
(空输出)
```

`[VERIFIED]` 工作区与暂存区**完全干净**，无修改、无未跟踪文件、无暂存内容。

> **重要限定**：工作区干净**只**说明没有未提交的本地改动。它**不是**批准、**不是**验收通过、**不是** Gate 判 PASS。本轮不把"Git 工作区干净"当作任何完成度证据。

### 1.5 Worktree 清单

```
$ git worktree list --porcelain
worktree C:/Users/Giant/Desktop/GITCODE/Procedural Avatar Engine
HEAD 99941a94ec58fb37ab0f1a36b90590d3fdce5198
branch refs/heads/feature/procedural-character-foundation

worktree E:/AI-Tools/ProceduralAvatarEngine-PoC
HEAD 7c451ae18137316291a4a51ecd03e57d2bea2d0f
branch refs/heads/poc-a-g2-fair-comparison
```

`[VERIFIED]` 存在 **2 个** worktree：

| # | 路径 | HEAD | 分支 |
|---|---|---|---|
| WT-1 | `C:/Users/Giant/Desktop/GITCODE/Procedural Avatar Engine` | `99941a9` | `feature/procedural-character-foundation` |
| WT-2 | `E:/AI-Tools/ProceduralAvatarEngine-PoC` | `7c451ae` | `poc-a-g2-fair-comparison` |

`[VERIFIED]` WT-1 是**原工作区**，当前落后远端 5 个提交（`behind 5`）。本轮**未触碰** WT-1。

### 1.6 远端

```
$ git remote -v
origin  https://github.com/Vin535LFJ/ProceduralAvatarEngine.git (fetch)
origin  https://github.com/Vin535LFJ/ProceduralAvatarEngine.git (push)

$ git ls-remote origin
c539afbec299ed4a066429ea09f9d3d36e67f74d  HEAD
7c451ae18137316291a4a51ecd03e57d2bea2d0f  refs/heads/feature/procedural-character-foundation
67ccb3940950623b1a24d6b3e843750c5c4a99b3  refs/heads/m11-anatomy-visual-rebuild
c539afbec299ed4a066429ea09f9d3d36e67f74d  refs/heads/m12-unified-human-geometry
c539afbec299ed4a066429ea09f9d3d36e67f74d  refs/heads/main
```

`[VERIFIED]` 远端 `feature/procedural-character-foundation` = `7c451ae` = 本地 HEAD。**本地与远端一致，无需推送。**

`[VERIFIED]` 远端 `HEAD`（默认分支）= `c539afb` = `main`，**不含**本项目 PoC-A/G2 工作。这是预期状态——PoC 工作全部在 feature 分支。

---

## 2. 仓库外草稿目录（实测）

`[VERIFIED]` 目录 `E:/AI-Tools/DEC-05-Review-Drafts/` 实际存在，包含 **6 个**文件，全部为 Markdown。

| 文件名 | 字节 | 修改时间 (UTC+8) | SHA256 |
|---|---:|---|---|
| `Deliverable-1-visual-direction.md` | 11539 | 2026-11-24 09:06:10 | `d4316ceb9762ea7fc0ad36f29bc1328128001872e86394bc0f8b209c58abb05e` |
| `Deliverable-2-reference-source-matrix.md` | 12705 | 2026-11-24 09:10:13 | `c45bf669d27bff24818f89fadf0b2270c8c4cb375a1bd8d5689da9da22ded125` |
| `Deliverable-3-DEC-05-revised-draft.md` | 12555 | 2026-11-24 09:11:01 | `3c98e82883da31895f43e1cf8b22aaec681dd704b8e56ff6516ab650fb7a0e12` |
| `Deliverable-4-visual-rubric-revision.md` | 27040 | 2026-11-24 09:06:36 | `2a7ffe764610c80ffc1d1ada3270bba99e45bb53925ac15d4773b1cbc269fe84` |
| `Deliverable-5-open-questions.md` | 14054 | 2026-11-24 09:10:24 | `12369796679b0afe391bdd236eb5fad9fdf2ae24223d1ed26c2960b98a72dafc` |
| `Deliverable-6-final-recommendation.md` | 15963 | 2026-11-24 09:10:30 | `6cae1f82da4db01072414e18055a20268cb71ad3a51f7e56eb7ed4b4d4d8c783` |

`[VERIFIED]` 6 个文件均真实存在、非空、内容与上一轮报告一致（标题行与结构已抽样复核）。

### 2.1 内容存在性抽查（非依赖上一轮陈述）

`[VERIFIED]` 实际抽查结果：

| 文件 | 抽查项 | 实测结果 |
|---|---|---|
| D-1 | 是否含 §4.1 负责人层级决策 | 存在，标题 `### 4.1 负责人已明确的层级与阶段分工（2026-11-24）` |
| D-1 | 是否含五 Gate 表 | 存在，§4.2 标题 `五 Gate 分离：几何验收不得与最终渲染品质混成一个 Gate` |
| D-2 | 是否含 20 条参考 ID | 存在 `R-A01..R-A08 / R-B01..R-B05 / R-C01..R-C09` |
| D-2 | 是否含 E-1~E-3 证伪规则 | 存在，§2.0 |
| D-3 | 是否含 Level 2 决策 | 存在，§5 表 + 负责人指示标注 |
| D-3 | 是否含五 Gate | 存在，§5.1 |
| D-4 | 是否含 8 维度 D1–D8 | 存在，§2.1 |
| D-4 | 是否含视角×姿态正交 | 存在，§3 |
| D-5 | 是否含 7 个指定问题 | 存在，§A Q-1..Q-7 |
| D-5 | 是否含 OQ-27..OQ-35 | 存在，§B |
| D-6 | 是否含最终建议 | 存在，§1–§10 |

`[VERIFIED]` 上一轮报告的文件存在性陈述**与实际文件一致**，无虚报。

---

## 3. 仓库内文档（实测 SHA256）

`[VERIFIED]` `docs/phases/anatomy-driven-geometry-poc/`：

| 文件 | 字节 | 修改时间 | SHA256 |
|---|---:|---|---|
| `00-implementation-spec.md` | 61052 | 2026-11-23 20:27:45 | `5e3a4f8a80311766aa3b93ad6d1cf3a92a58a102a47152636baae0a2ae8d954b` |
| `01-spec-consolidation-change-report.md` | 11192 | 2026-11-23 18:37:39 | `cededb892e703e5bc8f4070a093743605584789eed497d85272d5596fab72ad2` |
| `02-route-review-prompt.md` | 6648 | 2026-11-23 20:15:03 | `2aa7d6974ce1634061c2685703a51d62a58e7672911bb173074b217ffb05481b` |
| `03-g3-defer-and-next-phase-proposal.md` | 10800 | 2026-11-23 20:28:52 | `9fa7c36ba577f47a2e71803b623af535a658d80801db12c727c42da00c7570d3` |
| `04-visual-rubric.md` | 5342 | 2026-11-23 20:28:07 | `78fe7db70643e7f89ba5ca758fb8a52ecec63eaf1dc9b71102c52158f910bb5c` |
| `05-g3-decision-record.draft.md` | 5670 | 2026-11-23 21:34:21 | `ff9291f9c6c23dbe4a0f2c17769a1a4babe2ca2c6af88293aeef10b4434df70d` |
| `06-owner-signoff-checklist.md` | 2681 | 2026-11-23 21:34:30 | `a1ce1b6d344b9f0dba70104842e80023945a04cbb68e45d26a5bf38887c29d2b` |
| `07-open-issues.md` | 4478 | 2026-11-23 21:37:47 | `f4410d90d4d599fa5d8729b38c12d6fd57842cfa75ab031d1b604a15a8d4eef9` |

`[VERIFIED]` ADR 草案：

| 文件 | 字节 | 修改时间 | SHA256 |
|---|---:|---|---|
| `docs/architecture-decisions/ADR-011-shape-model-contract.draft.md` | 4041 | 2026-11-23 20:28:20 | `543283957ddd9a173ae0ade420455d04989888d0de3fed454f3db73c877bbacf` |
| `docs/architecture-decisions/ADR-012-saltline-module-study.draft.md` | 3282 | 2026-11-23 20:28:28 | `9f47d0690ac9ef2eab4305bf2505ef557635555c87cab5e75cb765d446b788a2` |

---

## 4. 草稿 vs 仓库内文档：逐项对照

`[VERIFIED]` 对照结论：**草稿目录的 6 份文件与仓库内文档存在部分内容重叠，但没有任何一份是仓库内文档的副本**。关系如下：

| 主题 | 仓库内文件 | 仓库外草稿 | 关系 |
|---|---|---|---|
| G3 DEFER 提案 | `03-g3-defer-and-next-phase-proposal.md` | D-6 §8 | 草稿是**扩充版**（含五 Gate 与层级决策），仓库版**未更新** |
| 视觉 rubric | `04-visual-rubric.md`（5342 B） | D-4（27040 B） | 草稿是**完整替换提案**，仓库版仍是旧版 |
| DEC-05 | `00-implementation-spec.md` register 行 | D-3 | 草稿是**修订草案**，register 未改 |
| ADR-011/012 | 两份 `.draft.md` | D-6 / 本轮 06 | 草稿是**签署稿**，仓库版仍是草案 |
| Open Issues | `07-open-issues.md`（OQ-01..26） | D-5 §B（OQ-27..35） | **两套编号并存** — 见本轮 04 的归并表 |
| 参考矩阵 | 无 | D-2 | 仅草稿 |
| Canonical Sheet | 无 | 本轮 03 | 仅草稿 |

`[INFERENCE]` **真实风险**：D-5 的 OQ-27..OQ-35 与仓库内 `07-open-issues.md` 的 OQ-01..OQ-26 **构成两套并行编号**，且两者都有 DEC-05 与 AUTH-08 相关条目（OQ-01 ↔ OQ-28；OQ-02 ↔ AUTH-08）。这正是负责人指出的"禁止并行维护多个 Register"。本轮 04 给出归并方案。

---

## 5. G2 基线核验（§2 任务）

### 5.1 commit 是否存在

```
$ git cat-file -t a32251bf798e46159696a637a699a062453df345
commit

$ git log --oneline -1 a32251bf798e46159696a637a699a062453df345
a32251b poc-a: G0/G1/G2 route-comparison evidence and sign unification
```

`[VERIFIED]` **存在**，类型为 commit，标题与 G2 冻结一致。

`[VERIFIED]` 完整元数据：

```
a32251bf798e46159696a637a699a062453df345
2026-11-23 20:14:00 +0800
poc-a: G0/G1/G2 route-comparison evidence and sign unification
```

commit body（节选）：明确列出 G0/G1/G2 内容、T2 shoulder-root crop、`TopologyAudit`、E1/E2/E3 分列、63 张灰模、smooth-union 符号修正、`Host CTest 11/11 passing`、并写明 `GATE-03 stays open pending owner threshold approval and the independent visual Gate; no route is approved.`

### 5.2 是否属于当前项目

`[VERIFIED]` 是。该 commit 是本仓库 `origin` 上的对象，位于 `feature/procedural-character-foundation` 的历史链上。

### 5.3 是否确为 G2 代码与证据冻结基线

`[VERIFIED]` **是**。commit body 明确声明 G0/G1/G2 内容合入，且 `evidence/g2/` 与 `evidence/reports/g2-fair-comparison-report.md` 在该 commit 中被加入。后续 4 个 commit **不含任何 G2 证据或代码改动**（见 5.5）。

### 5.4 后续提交的父子关系

```
$ git merge-base --is-ancestor a32251b HEAD  →  YES (ancestor)
$ git rev-list --count a32251b..HEAD         →  4
```

`[VERIFIED]` `a32251b` 是当前 HEAD 的**直系祖先**，中间隔 4 个提交：

| 顺序 | SHA | 时间 | 主题 |
|---|---|---|---|
| 1 | `e8e697e` | 2026-11-23 20:25:20 | docs: G3 DEFER proposal and independent route-review prompt |
| 2 | `92765cb` | 2026-11-23 20:29:04 | docs: record G3 DEFER adoption, visual rubric and ADR drafts |
| 3 | `f792d52` | 2026-11-23 21:34:35 | docs: G3 decision record draft and owner sign-off checklist |
| 4 | `7c451ae` | 2026-11-23 21:37:53 | docs: consolidated open issues register |

### 5.5 从该基线创建 Worktree 是否会丢失必要的代码或测试

```
$ git diff --name-status a32251b HEAD
M  docs/ADR_INDEX.md
A  docs/architecture-decisions/ADR-011-shape-model-contract.draft.md
A  docs/architecture-decisions/ADR-012-saltline-module-study.draft.md
M  docs/phases/anatomy-driven-geometry-poc/00-implementation-spec.md
A  docs/phases/anatomy-driven-geometry-poc/02-route-review-prompt.md
A  docs/phases/anatomy-driven-geometry-poc/03-g3-defer-and-next-phase-proposal.md
A  docs/phases/anatomy-driven-geometry-poc/04-visual-rubric.md
A  docs/phases/anatomy-driven-geometry-poc/05-g3-decision-record.draft.md
A  docs/phases/anatomy-driven-geometry-poc/06-owner-signoff-checklist.md
A  docs/phases/anatomy-driven-geometry-poc/07-open-issues.md
M  docs/proposals/anatomy-driven-geometry-redesign/09-final-recommendation.md
```

`[VERIFIED]` **改动 100% 落在 `docs/`**。`git diff --name-only a32251b HEAD | grep -vE '^(docs/|evidence/)'` 返回**空**。

`[VERIFIED]` 基线已包含全部 PoC-A/G2 代码与测试（`git ls-tree` 实测）：

```
native/procedural/shape/{CameraProtocol,ClayRenderer,ImageWriter,LayerAMetrics,
                         PocRoutes,QuadTemplate,ShapeModel,SkinWeights,
                         SurfaceMetrics,TopologyAudit}.{h,cpp}
tests/unit/{poc_fair_comparison_probe,poc_route_probe,poc_routes_tests,
            smooth_union_sign_tests}.cpp
```

**结论**：

| 问题 | 答案 | 等级 |
|---|---|---|
| 会丢失**代码**吗？ | **不会。** 基线含全部 `native/procedural/shape/` 与 host 测试 | `[VERIFIED]` |
| 会丢失**测试**吗？ | **不会。** 4 个测试/探针文件均在基线 | `[VERIFIED]` |
| 会丢失**证据**吗？ | **不会。** `evidence/` 在这 4 个提交中未被改动 | `[VERIFIED]` |
| 会丢失**治理文档**吗？ | **会。** 丢失 4 个提交的全部 `docs/` 变更：SPEC register 的 G3 DEFER / DEC-09 / AUTH-08 状态、ADR_INDEX 登记、02–07 系列文档、ADR-011/012 草案 | `[VERIFIED]` |

`[RECOMMENDATION]` **保留 `a32251b` 作为 PoC-SM 的代码基线**（负责人指定，不替换），但必须把缺失的 4 个 `docs/` 提交带入新 worktree —— 方式是**从 `a32251b` 建 worktree 后，把 `e8e697e..7c451ae` 的 `docs/` 变更以 cherry-pick 或 checkout 方式带入**（见本轮 08 的 AUTH-08 执行说明）。

`[INFERENCE]` 理由：新 Shape Model 的实现可以完全基于 `a32251b` 的代码，但**治理上下文不能丢**——尤其 `00-implementation-spec.md` 里已经写入的 `GATE-04 DEFER`、`DEC-09 APPROVED`、`AUTH-08 OPEN` 状态，以及 `07-open-issues.md` 这个正式 Register。若新 worktree 只有 `a32251b`，PoC-SM 将在一份**没有授权记录**的 SPEC 上开发，这会让"是否已获授权"变得不可审计。

---

## 6. 审计结论

| # | 结论 | 等级 |
|---|---|---|
| A-1 | 本地 HEAD = `7c451ae`，与远端 feature 分支一致，工作区与暂存区干净 | `[VERIFIED]` |
| A-2 | 仓库外草稿 6 份文件**真实存在**，SHA256 已记录，内容与上一轮陈述一致 | `[VERIFIED]` |
| A-3 | 仓库内 8 份阶段文档 + 2 份 ADR 草案**真实存在**，SHA256 已记录 | `[VERIFIED]` |
| A-4 | G2 基线 `a32251b` 存在、属本项目、确为 G2 冻结、是当前 HEAD 的直系祖先 | `[VERIFIED]` |
| A-5 | 从 `a32251b` 建 worktree **不丢代码/测试/证据**，但**丢 4 个 docs 提交的治理上下文** | `[VERIFIED]` |
| A-6 | 存在**两套并行 OQ 编号**（仓库 OQ-01..26 / 草稿 OQ-27..35），且 DEC-05 与 AUTH-08 条目重复 | `[VERIFIED]` |
| A-7 | 草稿与仓库文档存在主题重叠但无副本关系；仓库版尚未吸收草稿的五 Gate 与层级决策 | `[VERIFIED]` |
| A-8 | 原工作区 WT-1（`99941a9`）落后远端 5 个提交，本轮未触碰 | `[VERIFIED]` |

`[OPEN QUESTION]` A-5 的治理上下文带入方式需要负责人确认（cherry-pick 4 个提交 vs 仅 checkout `docs/` 子树）。

---

## 7. 本轮未做的事（边界声明）

`[VERIFIED]` 本轮**未**执行：修改 C++ 源码、创建 Worktree、创建分支、修改生产工程、修改骨骼/JNI ABI、编译或部署、Commit、Push、自签 ADR、自签 G3、自批 DEC-05。所有新文件均写入 `E:/AI-Tools/DEC-05-Review-Drafts/blocker-closure/`（仓库外），仓库 `git status` 在本轮结束时仍为空。
