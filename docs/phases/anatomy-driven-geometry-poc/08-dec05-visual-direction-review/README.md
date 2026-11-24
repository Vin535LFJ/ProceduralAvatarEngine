# 08 — DEC-05 视觉方向重审（第 1 轮交付物）

> 状态：**全部为草案 / DRAFT**。本目录**不**代表 DEC-05 已批准，**不**代表任何 Gate 已 PASS。
> 签字栏一律留空；AI 未代签任何决策。

---

## 1. 这些文件是什么

2026-11-24 第一轮「DEC-05 视觉目标与授权参考体系重审」的 6 份交付物，
按任务书 §十 的顺序产出，**未修改任何 C++ 源码、未改项目文档、未建 worktree/分支、未 commit/push**。

| 文件 | 内容 | 对应任务书 §十 交付项 |
|---|---|---|
| `Deliverable-1-visual-direction.md` | 视觉方向决策、Level 1/2/3、五 Gate 分离及其与 G2 证据的对应 | Deliverable 1 |
| `Deliverable-2-reference-source-matrix.md` | 参考来源完整矩阵、三条通用证伪规则 E-1~E-3 | Deliverable 2 |
| `Deliverable-3-DEC-05-revised-draft.md` | DEC-05 修订草案（签字栏留空） | Deliverable 3 |
| `Deliverable-4-visual-rubric-revision.md` | rubric 修订建议：8 维度、视角×姿态正交、5 种图像证据、V-1~V-7 | Deliverable 4 |
| `Deliverable-5-open-questions.md` | 7 个指定问题 + OQ-27..OQ-35 + 对 Register 的建议状态更新 | Deliverable 5 |
| `Deliverable-6-final-recommendation.md` | 最终建议、参考取舍、DEC-05 可否提交审批、后续阶段条件 | Deliverable 6 |

---

## 2. ⚠️ 请先读这条：本目录已被第 2 轮部分取代

**第 2 轮（`../09-blocker-closure-auth08/`）做了新的一手核验，修正了本目录若干授权表述。**
以第 2 轮为准。

| 本目录中的表述 | 第 2 轮的实测结论 | 出处 |
|---|---|---|
| Blender 手册许可 = **CC BY-SA 3.0（已核验）** | **手册正文许可未核验**（两次抓取许可页均被 Cloudflare 拦截，无逐字证据）。**网站**内容 CC BY 4.0 已核验。且该手册页**正文本身**也未捕获 | `../09-blocker-closure-auth08/02-dec05-reference-update-matrix.md` §3.4 |
| MakeHuman 许可 = **AGPL（软件）** | **资产许可为 CC0**（FAQ 原文 "The asset license is CC0"）。软件与资产是两条不同的许可 | 同上 |
| Layer B 参考不足 | **已核验来源从 2 条增至 6 条**（新增 BodyParts3D / Thunder Cloud Studio / Blender Studio / Renderosity，均为实际抓取正文），并建立 **4 级权威阶梯** | 同上 §1、§3.1 |
| 参考 ID `R-C01` = Blender 手册 | 编号已修正：Blender 手册 = **`R-C02`**，MakeHuman = **`R-C06`**，BodyParts3D = **`R-B02`** | 同上 §3.4 |
| `OQ-28`…`OQ-35` 是独立阻塞项 | **已归并**：OQ-28/29/30/31/32 → **OQ-01 子项**；OQ-33 → **OQ-02 子项**。独立 BLOCKING 主项回到 **5** 个 | `../09-blocker-closure-auth08/04-oq-numbering-merge.md` |
| Canonical Sheet 仅描述方案 | **已产出 v0 候选**，并取得 ANSUR II / CDC NHANES **一手数值** | `../09-blocker-closure-auth08/03-canonical-acceptance-sheet-v0.md` |
| David Rig `UNVERIFIED` | **维持 `UNVERIFIED`**（第 2 轮再次核验失败：直接抓取 fetch failed + 站内检索无关联）。结论未变 | `../09-blocker-closure-auth08/02-dec05-reference-update-matrix.md` §2.1 |

`[VERIFIED]` **本目录的 Level 2 核心目标 / Level 3 长期方向 / 五 Gate 分离 / 相机轴×姿态轴正交
等方向性结论仍然有效**（第 2 轮逐条复核一致，见 `../09-blocker-closure-auth08/05-five-gate-matrix.md` §0），
被取代的是**授权表述与参考编号**。

---

## 3. 阅读顺序

| 你要做 | 先读 |
|---|---|
| 快速了解视觉方向结论 | `Deliverable-1` → `Deliverable-6` |
| 决定参考能否用 | `Deliverable-2`（**但先看上面第 2 节的修正表**） |
| 决定 rubric 怎么改 | `Deliverable-4` |
| 拿待决问题清单 | `Deliverable-5`（**编号已归并，看 `../09-.../04`**） |

---

## 4. 边界声明

`[VERIFIED]` 本目录内容在第 1 轮结束时位于仓库外的 `E:/AI-Tools/DEC-05-Review-Drafts/`，
本轮**按字节原样**复制入库 —— 可直接验证：

```
cd docs/phases/anatomy-driven-geometry-poc/08-dec05-visual-direction-review
sha256sum -c SHA256SUMS.txt     # 6 份 Deliverable 全部 OK
```

第 1 轮**未**修改任何 C++ 源码、**未**改项目文档、**未**建 worktree/分支、**未** commit/push、
**未**自签任何 ADR / G3 / DEC-05。

---

## 5. 入库说明（第 3 轮：同步到远端）

本目录在第 3 轮才被 commit 并 push 到 `origin/feature/procedural-character-foundation`。
第 3 轮的改动**仅限入库动作本身**：6 份 `Deliverable-*.md` 按字节原样入库，
另新增本 `README.md` 与 `SHA256SUMS.txt`（用于证明字节一致性）。

`[VERIFIED]` 第 3 轮**未**：修改任何 `Deliverable-*.md` 正文、修改 C++ 源码、创建 Worktree、创建分支、
签署 ADR-011/012、签署 G3 DEFER、批准 DEC-05、签发 AUTH-08、修改 `07-open-issues.md` 或 SPEC register。

> ⚠️ **入库 ≠ 批准。** 本目录进入 Git 历史只代表"材料已归档、可追溯、可被评审"，
> **不代表** DEC-05 已批准，**不代表** 任何 Gate 已 PASS。签字栏依然全部留空。
