# 00 — INDEX：本轮交付物清单与 SHA256

> 状态：**索引 / INDEX**。本目录全部内容位于**仓库外** `E:/AI-Tools/DEC-05-Review-Drafts/blocker-closure/`。
> 仓库 `E:/AI-Tools/ProceduralAvatarEngine-PoC` 在本轮结束时**零改动**（`git status` 为空，HEAD `7c451ae`，worktree 仍 2 项）。
> **本目录所有文件均为草案 / 待签署材料。没有一份代表已获批准。**

---

## 1. 交付物清单（§10 的 10 项 + 2 份来源原件）

| # | §10 要求 | 文件 | 字节 | SHA256 |
|---|---|---|---:|---|
| 1 | 真实状态审计报告 | `01-state-audit-report.md` | 15200 | `82077d12cdaad08405bfcd20219ec05e05cbd75f1015dff32315c30cc6b077cb` |
| 2 | DEC-05 参考来源更新矩阵 | `02-dec05-reference-update-matrix.md` | 15915 | `030cdb249bc9a108c5d3491b2c856c65c5fd9ed93ce9d4a98f0b38a64f610421` |
| 3 | Canonical Acceptance Sheet v0 候选 | `03-canonical-acceptance-sheet-v0.md` | 19997 | `089cdee551afbbe8f7458f04154476314acfd5e9db20638f0572d11a5111eaba` |
| 4 | OQ 编号归并表 | `04-oq-numbering-merge.md` | 15667 | `a5795ec044ed00ffafda81149ab2d3730439d43cad340799e69a4641234824f4` |
| 5 | 完整五 Gate Matrix | `05-five-gate-matrix.md` | 11603 | `eaa435ee1a1a4878a5c435dda079da7fe7b0070064247243952b9186f0ad0fec` |
| 6 | ADR-011 / ADR-012 负责人签署稿 | `06-adr-011-012-signoff.md` | 9270 | `a66663455216c0844517bc329810a9dcae39e2b6e0660d09e2abdc7c9ca25ead` |
| 7 | G3 DEFER 负责人签署稿 | `07-g3-defer-signoff.md` | 9790 | `2096856f847673bcd1a4b892d27397fab7889adbf7d44628d269ba4d501a18fd` |
| 8 | AUTH-08 授权范围 + 3 个 Worktree 路径候选 | `08-auth08-scope-and-worktree-candidates.md` | 11041 | `97f1b6e030391bff7beaceed7b856a0d097c054cc44cafdc8f0ef4ca5529ee59` |
| 9 | 技术缺口优先级计划 | `09-technical-gap-priority-plan.md` | 9888 | `1907f9ef4a9a224321166735462f70c9bcb7c82811a480992dfbadfdd5148845` |
| 10 | PoC-SM 启动条件结论 | `10-poc-sm-readiness-conclusion.md` | 10405 | `b872d91548d11dc153f2f6843c64fd5772e8eeca93fede6f1f98310b25fb242f` |

### 1.1 来源原件（留存以供复核）

| 文件 | 内容 | 字节 | SHA256 |
|---|---|---:|---|
| `source/ANSUR-II-Anthropometry-Summary-Tables.pdf` | NC State Ergonomics Center《Anthropometric Data for U.S. Adults》(© 11/2024)，引用 Gordon et al. (2014) ANSUR II。**本轮下载的一手原件** | 542674 | `a1ea470e6d67d5870a1b35da9fdf4a7d60f1982ee0b187827b1b3a7a3f820ee6` |
| `source/CDC-NHANES-SR50-Anthropometric-Reference-Data.pdf` | CDC/NCHS《Anthropometric Reference Data… August 2021–August 2023》Statistical Report No. 50。**本轮下载的一手原件** | 1812869 | `ccb8d52b6e3f09bc50d5734493e512c348c85ad2ea98ea3cd559019da8f13f16` |

`[VERIFIED]` 完整清单见同目录 `SHA256SUMS.txt`（由 `sha256sum` 生成，可直接 `sha256sum -c` 校验）。
`[VERIFIED]` 本索引文件自身的 SHA256 在生成后单独计算，见 `SHA256SUMS.txt` 之外的口头交付说明。

---

## 2. 阅读顺序建议

| 若你要做… | 先读 |
|---|---|
| 决定"能不能开始写代码" | **10** → **08**（§1 签署块）→ **06** |
| 决定"视觉目标与验收" | **02** → **03** → **05** |
| 决定"审计/追溯" | **01** → **04** |
| 决定"缺口排期" | **09** → **05** |
| 签署 G3 | **07** |

---

## 3. 负责人需要做的 6 件事（全部材料已备齐）

| # | 动作 | 文件 | 关闭 |
|---|---|---|---|
| 1 | 签 ADR-011 | 06-A | OQ-04 |
| 2 | 签 ADR-012（RESEARCH ONLY） | 06-B | OQ-04 |
| 3 | 签 G3 DEFER 记录 | 07 §3 | OQ-05 |
| 4 | 签 ≤5° 性质决策 | 07 §1 | OQ-03 |
| 5 | 签 AUTH-08（基线 + 路径 + 分支名 + OQ-33 裁定） | 08 §1 | OQ-02 |
| 6 | 批 DEC-05（参考体系 C + Canonical Sheet 路径/原型 + 姿态/相机/动作集） | 02 + 03 + 04 §4 | OQ-01（含 5 子项） |

`[VERIFIED]` **第 1–5 项完成后 PoC-SM 即可开始编码；第 6 项不阻塞编码，只阻塞几何验收。**

---

## 4. 本目录的性质

`[VERIFIED]`

- 全部为**草案 / 待签署材料**，签字栏一律留空；
- **没有**任何一项决策被 AI 代签或代批；
- **没有**任何 Gate 被判为 PASS（`G-P1-GEO` = FAIL，`G-P1-ANIM` = PARTIAL，`G-P2-MOD` / `G-FINAL-VIS` = NOT RUN）；
- **没有**修改仓库内任何文件（04 §4 的 Register 替换稿是待批草案，未写回）；
- **没有**执行 `git worktree add`、没有创建分支、没有 commit / push；
- 仓库 `E:/AI-Tools/ProceduralAvatarEngine-PoC` 保持 HEAD `7c451ae18137316291a4a51ecd03e57d2bea2d0f`，`git status --short` 为空。
