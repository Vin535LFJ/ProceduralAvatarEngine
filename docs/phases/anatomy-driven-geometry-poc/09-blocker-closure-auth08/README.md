# 09 — Blocker Closure & AUTH-08 Readiness（第 2 轮交付物）

> 状态：**全部为草案 / DRAFT — 待负责人签署**。
> 本目录**不**代表 DEC-05 已批准、**不**代表任何 Gate 已 PASS、**不**构成 AUTH-08 授权。
> 签字栏一律留空；AI 未代签任何决策。

---

## 1. 这批材料要解决的问题

第 1 轮（`../08-dec05-visual-direction-review/`）确立了方向与结构，但留下了阻塞：
DEC-05 的参考来源未定、AUTH-08 无基线无路径、ADR 未签、G3 未签、OQ 编号存在两套并行。
第 2 轮的任务是**尽可能扫除决策与资料阻塞，为 PoC-SM 建立可审计的启动条件**。

---

## 2. 交付物

| 文件 | 内容 | §10 对应项 |
|---|---|---|
| `00-INDEX.md` | 索引 + 全部 SHA256 | — |
| `01-state-audit-report.md` | **真实状态审计**（实测 git / worktree / 远端 / 草稿 / 基线 ancestry） | 1 |
| `02-dec05-reference-update-matrix.md` | DEC-05 参考来源更新矩阵 + **4 级权威阶梯** + 4 条新核验 Layer B 来源 | 2 |
| `03-canonical-acceptance-sheet-v0.md` | **Canonical Acceptance Sheet v0 候选**（含一手人体测量数值与 TBD 登记） | 3 |
| `04-oq-numbering-merge.md` | OQ 编号归并表 + `07-open-issues.md` **完整替换稿**（待批） | 4 |
| `05-five-gate-matrix.md` | **唯一五 Gate Matrix** + 5 条禁止的可执行条款 | 5 |
| `06-adr-011-012-signoff.md` | ADR-011 / ADR-012 **负责人签署稿** | 6 |
| `07-g3-defer-signoff.md` | OQ-03 / OQ-05 收敛建议 + **G3 DEFER 负责人签署稿** | 7 |
| `08-auth08-scope-and-worktree-candidates.md` | AUTH-08 授权书 + **3 个 Worktree 路径候选**（含 5 项预检） | 8 |
| `09-technical-gap-priority-plan.md` | 技术缺口优先级计划（OQ × 6 阶段阻塞矩阵） | 9 |
| `10-poc-sm-readiness-conclusion.md` | **PoC-SM 启动条件明确结论** | 10 |

---

## 3. 核心结论摘要（详见 `10-`）

- **真正关闭的阻塞项：0 项。** 关闭需要负责人签字，AI 不能代签。
- **6 个主项材料齐备可签**：OQ-01 / OQ-02 / OQ-03 / OQ-04 / OQ-05 + OQ-33。
- **签 OQ-04 + OQ-02（含 OQ-33 裁定）→ PoC-SM 即可开始编码。**
- **DEC-05 批准不阻塞编码，只阻塞几何验收。**
- 几何验收 / G2 重测 / G3 重新批准 / G4 **均不具备条件**。

---

## 4. ⚠️ 关于 `source/` 目录：两份一手原件**未入库**

`00-INDEX.md` 与 `03-canonical-acceptance-sheet-v0.md` 中引用的两份一手测量表原件
（ANSUR II 汇总表、CDC NHANES SR50）**不在本目录**，按 URL + SHA256 记录，详见 `source-manifest.md`。

`[VERIFIED]` 理由：两者均为公开可下载的第三方出版物，NC State 表标注 "© 11/2024 The Ergonomics Center"
且未声明开放许可。**入库再分发存在授权不确定**，因此只记录地址与校验值——
任何人按 URL 下载后 `sha256sum` 即可验证与本轮读到的是同一份文件，可复现全部数值。

`[VERIFIED]` 因此在仓库内执行 `sha256sum -c SHA256SUMS.txt` 时，`source/` 两项会报 "No such file"，
**这是预期行为**；其余 11 份 `.md` 与 `SHA256SUMS.txt` 本身全部 `OK`。

### 4.1 换行符固定（`.gitattributes`）

`[VERIFIED]` 仓库根目录新增了 `.gitattributes`，把 `08-` 与 `09-` 两个目录下的 `*.md` 固定为
`text eol=lf`。原因是仓库全局 `core.autocrlf=true`，若不固定，Windows 上的评审者重新 clone 后
工作区会被签出为 CRLF，导致 `sha256sum -c SHA256SUMS.txt` 出现 13 处**纯换行符**误报不匹配。

固定后，Windows / macOS / Linux 上 `sha256sum -c` 的结果一致：11 份 `.md` 全部 `OK`，
`source/` 两项按上文预期报 "No such file"。该规则**只作用于这两个新增目录**，
未改变任何既有文件的签出行为。

---

## 5. 阅读顺序

| 你要做 | 先读 |
|---|---|
| 决定"能不能开始写代码" | `10` → `08`（§1 签署块）→ `06` |
| 决定"视觉目标与验收" | `02` → `03` → `05` |
| 决定"审计/追溯" | `01` → `04` |
| 决定"缺口排期" | `09` → `05` |
| 签署 G3 | `07` |

---

## 6. 边界声明

`[VERIFIED]` 本批材料在第 2 轮结束时位于仓库外的 `E:/AI-Tools/DEC-05-Review-Drafts/blocker-closure/`，
本轮**按字节原样**复制入库。第 2 轮**未**：修改 C++ 源码、创建 Worktree、创建分支、
修改生产工程、修改骨骼/JNI ABI、编译或部署、commit/push、自签 ADR、自签 G3、自批 DEC-05、
修改仓库内既有文档（含 `07-open-issues.md` 与 SPEC register —— `04` 中的替换稿是**待批草案**，未写回）。

---

## 7. 入库说明（第 3 轮：同步到远端）

本目录在第 3 轮才被 commit 并 push 到 `origin/feature/procedural-character-foundation`。
第 3 轮的改动**仅限入库动作本身**，没有触碰任何决策内容：

| 项 | 状态 |
|---|---|
| `00-`…`10-` 共 11 份 `.md` | **按字节原样**入库，SHA256 与第 2 轮一致（见 `SHA256SUMS.txt`） |
| `SHA256SUMS.txt` | **按字节原样**入库，自身校验值见 `00-INDEX.md` |
| `README.md` | 第 3 轮新增（索引 + 交付物地图 + §4 + §4.1 + §7） |
| `source-manifest.md` | 第 3 轮新增（两份一手原件的 URL / SHA256 / 复现命令） |
| `source/*.pdf` | **仍未入库**，理由见 §4 |

`[VERIFIED]` 第 3 轮新增的两个文件（`README.md`、`source-manifest.md`）的校验值见
`SHA256SUMS-round3.txt`，在本目录内执行 `sha256sum -c SHA256SUMS-round3.txt` 可验证。

`[VERIFIED]` 第 3 轮**未**：修改任何 `.md` 交付物正文、修改 C++ 源码、创建 Worktree、创建分支、
签署 ADR-011/012、签署 G3 DEFER、批准 DEC-05、签发 AUTH-08、修改 `07-open-issues.md` 或 SPEC register。

> ⚠️ **入库 ≠ 批准。** 本目录进入 Git 历史只代表"材料已归档、可追溯、可被评审"，
> **不代表** DEC-05 / ADR-011 / ADR-012 / G3 DEFER / AUTH-08 中任何一项已被批准，
> **不代表** 任何 Gate 已 PASS。签字栏依然全部留空。
