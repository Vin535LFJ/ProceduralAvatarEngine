# SPEC Consolidation — 文档变更清单与 Git 状态报告（v2.1）

> 配套文件：`00-implementation-spec.md`（**v2.1 FORMAL CANDIDATE，仍为 DRAFT**）。
> 本报告为只读审计 + 指定 Markdown 文档修订的记录；**未修改源码、未修改 CMake/Gradle、未操作 Git 分支/提交、未执行构建或设备测试、未创建 Worktree**。
> 标签：`[FACT]` / `[INFERENCE]` / `[RECOMMENDATION]` / `[OPEN QUESTION]`；状态：`VERIFIED` / `PARTIALLY VERIFIED` / `DECLARED ONLY` / `NOT IMPLEMENTED` / `UNVERIFIED` / `REJECTED`。

---

## 1. 本次（v2.1）实际修改文件

| 文件 | 操作 | 相对基线 `bd3ad72` 的差异 | 性质 |
|---|---|---|---|
| `docs/phases/anatomy-driven-geometry-poc/00-implementation-spec.md` | 修改 | `175 insertions(+), 88 deletions(-)`（`git diff --numstat`） | 仅 Markdown；关闭 FIX-01…06 + 一致性检查；版本标记 v2.1 |
| `docs/phases/anatomy-driven-geometry-poc/01-spec-consolidation-change-report.md` | 修改（本文件） | `77 insertions(+), 52 deletions(-)`（`git diff --numstat`） | 仅 Markdown；新增 FIX Closure Matrix、前后差异、遗留 OPEN、Git 状态 |

**未修改**：任何 C++ / JNI / Kotlin / Shader 源码；`native/CMakeLists.txt`；`tests/CMakeLists.txt`；`app/build.gradle.kts`。
`git diff --name-only -- native app tests CMakeLists.txt app/build.gradle.kts` = **空** `[FACT]`。

## 2. FIX-01…FIX-06 Closure Matrix

| FIX | 问题 | 修订章节 | 关闭方式 | 状态 | 残留 / OPEN |
|---|---|---|---|---|---|
| FIX-01 | PoC-A Topology Scope 冲突：§10 要求 Shoulder/Elbow Joint Loops 但 PoC-A 不含肩/臂 | 新增 §10.1；改 §10 Joint Loops 行、§14.1、§14.2、DEC-02、§6.2 G1/G2、Mermaid | 拓扑分 T1–T5 五层：T1 Torso Canonical + T2 Shoulder-root Connection = PoC-A 必须；T3 肩变形环线→PoC-B；T4 肘环线→Limb PoC；T5 完整肢体→G4/专项。明确 **Shoulder-root Connection ≠ 完整 Shoulder Joint**；禁独立肩球/圆管 | **CLOSED** `[FACT]` `[RECOMMENDATION]` | T3–T5 属后续 Gate，不在本 SPEC 授权范围（非缺陷，是延后） |
| FIX-02 | CPU/GPU Parity Gate 冲突：G2 parity UNVERIFIED 曾被同时当 Kill 条件 | 新增 §11.1；改 §6.1 Mermaid、§6.2 Gate 表、新增 §6.3 Kill/Stop、§7.2 G3 模板、§14.2、SPEC-09 行 | 分阶段责任：G0 记录 UNVERIFIED / G1 契约 / G2 权重合法性+CPU 变形（GPU parity 可 UNVERIFIED，**非 Kill**）/ G3 风险与计划 / G4 数值 Parity + GLES3 必须。Mermaid 与 Gate 表 kill 项去除 parity | **CLOSED** `[FACT]` | G4 数值 parity 未实测 `[UNVERIFIED]`；阈值 TBD |
| FIX-03 | Layer A 比较不对称：E1 用 R1 专属描述 | 改 §8.1（新增 8 项对称表）、§8.2（E1/E2/E3 重定义 + 每路线单独测量）、§8.3、§8.4 | R1/R2/R3 各自输出 Reference Surface；Target Reference / Sample Distribution / Sample Count / Distance Metric / Normal Metric / Landmark Set / Coordinate System / Parameter Set 八项完全相同；禁止 R1 特权；E1/E2/E3 三层分离，E2 每路线单独测 | **CLOSED** `[RECOMMENDATION]` | 阈值 TBD（§13），需 DEC-05 参考形态获批 |
| FIX-04 | G0 Worktree 与相机协议：未区分 L1–L4；相机协议可能隐含要求改源码 | 改 §12.1（新增 L1–L4 四层表 + worktree 细节）、§12.2 行 1、§12.3、§7.3 AUTH-01、GATE-01 行 | `git worktree add` 改变 Git 公共元数据（L2）明确列入 AUTH-01；基线 SHA `bd3ad72`（授权时重核）+ 命令 + 路径 + 清理边界 + 失败处理；现有 8 preset + gray-clay debug view（`RenderThread.cpp:726-744`、JNI、shader）可复用，**不以源码修改为前提**；缺失能力转 AUTH-06 | **CLOSED** `[FACT]` `[RECOMMENDATION]` | worktree 路径与基线 SHA 待 AUTH-01 负责人确认；未创建 worktree |
| FIX-05 | Parameter Traceability 证据等级过高：仅代码依赖标 VERIFIED | 改 §9（新增六级证据规则 + 全表状态）、§9 结论、SPEC-07 行 | 六级证据链：声明→读取→几何计算→顶点影响→可测 Mesh Delta→自动化差分；仅 ①–④ 者降为 `PARTIALLY VERIFIED`；chestWidth 单项断言仍 `PARTIALLY VERIFIED`；无参数记录删除 | **CLOSED** `[FACT]` | 升级 VERIFIED 需 G0 Parameter Baseline / G2 差分证据 |
| FIX-06 | G1 Route Kill/Exit 不一致：未定义 FEASIBLE/KILLED 与最少路线数 | 改 §6.2 G1/G2 行、§6.1 Mermaid、§7.2 G3 模板、新增 §6.3 | 每路线 FEASIBLE 或 KILLED；KILLED 需可复现失败证据+原因；仅 FEASIBLE 进 G2；**≥2 FEASIBLE 才比较**，仅 1 条 → G1 STOP + 负责人重评（不宣布比较完成）；G2 仅比较统一输入/参考/采样的 FEASIBLE 路线 | **CLOSED** `[RECOMMENDATION]` | 实际路线判定未执行（`NOT RUN`） |

## 3. 修改前后关键差异（v2.0 → v2.1）

| 项 | v2.0（BASELINE `bd3ad72`） | v2.1（工作区未提交） |
|---|---|---|
| 版本标记 | `正式候选 / FORMAL CANDIDATE` | `v2.1 FORMAL CANDIDATE，仍为 DRAFT` |
| §1 Git 基线 | HEAD `6eab7f4`、分叉 `0 7`（v2.0 提交后已过时） | HEAD `bd3ad72`、分叉 `0 8`、两段提交链 `486a00d→6eab7f4→bd3ad72` |
| 拓扑要求 | Joint Loops "必须（肩/肘环线）"（与 PoC-A 冲突） | §10.1 T1–T5 分层，PoC-A 仅 T1+T2 |
| Parity Gate | G2 隐含 parity UNVERIFIED 可能触发 kill | §11.1 五 Gate 责任表；G2 parity 非 kill；G4 必须实测 |
| Layer A | E1 为 `R1 原始表面误差` | E1 为所有路线统一的 Reference Surface → Target 误差；8 项对称前提 |
| Worktree | "基于重新核实 HEAD"，未提公共元数据 | L1–L4 四层；`git worktree add` 改 L2 列入 AUTH-01；现有相机能力复用 |
| 参数状态 | 多项 `VERIFIED`（仅代码依赖） | 全部降为 `PARTIALLY VERIFIED`；六级证据规则 |
| G1 判定 | "三路线均能产出合规网格；不能者 KILL" | FEASIBLE/KILLED + 可复现证据；≥2 FEASIBLE 才进 G2；1 条 → STOP 重评 |
| Kill 定义 | 散见各处，无统一表 | §6.3 Kill/Stop 触发条件表（含 parity 非 kill 明示） |

## 4. 全文一致性检查结果（任务第三节 9 项）

| # | 检查项 | 结果 |
|---|---|---|
| 1 | PoC-A Scope 与 Topology Requirements | **一致**：§10.1 T1–T5；§14.1/§14.2；DEC-02；G1/G2 Exit 均限定 T1+T2 |
| 2 | G0 Exit Criteria 与 G0 Authorization | **一致**：§12.3 复用现有相机（`§12.1`）；AUTH-01 含 L2；缺失转 AUTH-06；无源码修改前置 |
| 3 | G1 Kill Criteria 与 G1 Exit Criteria | **一致**：§6.2 G1 + §6.3 + Mermaid 均 FEASIBLE/KILLED、≥2 条、KILLED 需证据 |
| 4 | G2 Entry Criteria 与 G2 Acceptance | **一致**：G2 entry = ≥2 FEASIBLE；§8 协议统一输入/参考/采样；§6.3 明确 parity 非 kill |
| 5 | CPU/GPU Parity 各 Gate 职责 | **一致**：§11.1 表；Mermaid/Gate 表/§14.2/SPEC-09 行同步；G4 必须实测 |
| 6 | R1/R2/R3 比较对称性 | **一致**：§8.1 八项对称表；§8.2 每路线单独测 E1/E2/E3；无 R1 特权 |
| 7 | DEC/AUTH/GATE 编号引用 | **一致**：§7.1/7.3/7.4/7.5；新增 §6.1/§6.3 引用 GATE-xx 与 AUTH-xx 均有效 |
| 8 | DRAFT / NOT RUN 状态 | **一致**：标题/§0/§17 均为 v2.1 DRAFT；GATE Register 全 NOT RUN / OPEN |
| 9 | 用户原有文件保护要求 | **一致**：§1.2/§1.3/§1.4、§12.1 L1、§16、AUTH-02 均声明不恢复/不删除/不覆盖 |

发现并一并修复的新冲突（不属于 FIX-01…06）：
- **N-01**：v2.0 提交后 §1.1/§12.1 的 HEAD 与分叉数过时（`6eab7f4`/`0 7` → `bd3ad72`/`0 8`）—— 已修。
- **N-02**：Mermaid 中 KILLED 路线为终止节点，未回到 FEASIBLE 计数判断 —— 已加 `KILL1 → G1C` 分支。
- **N-03**：§11.1 的 G1 "数据契约不符 → KILLED" 与 FIX-02 "非 parity Kill" 需区分 —— 已在表中明确标注"非 parity"。

## 5. Git 状态：修改前 → 修改后 `[FACT]`

| 项 | 修改前（v2.1 前，15:56 采集） | 修改后（本次复核，2026-11-23 16:05:29 +0800） |
|---|---|---|
| `git rev-parse HEAD` | `bd3ad72f9541020f90d48a90dbfd7ee2a0142f85` | `bd3ad72f9541020f90d48a90dbfd7ee2a0142f85`（**未变，未提交**） |
| Branch | `feature/procedural-character-foundation` | 同（**未变**） |
| Upstream | `origin/feature/...`（0 0 同步） | 同 |
| Staged diff | 空 | 空（**未暂存**） |
| Tracked diff | 4 files（用户原有 3 删 + README） | **6 files**：用户原有 4 files 未动 + 本轮 2 份文档（SPEC `175+/88-`；报告 `77+/52-`） |
| Untracked | 11 项（`.workbuddy-ai/` + 根目录 00–09 副本） | 同 11 项；其中内部工作日志 `.workbuddy-ai/memory/2026-11-23.md` 追加 v2.1 记录，其余 10 项未触碰；均未 stash / 覆盖 / 纳入 Worktree |

### 5.1 用户原有未提交变更（未触碰）`[FACT]`
- ` D 01_Procedural_Character_Foundation_Specification_v1.0.md`
- ` D 02_Procedural_Character_Development_Roadmap_v1.0.md`
- ` D 03_AI_Engineering_Execution_Prompt_v1.0.md`
- ` M README.md`
- Untracked：`.workbuddy-ai/memory/2026-11-23.md`（内部工作日志，v2.1 结束时追加记录；非设计交付物）+ `00-current-state-audit.md` … `09-final-recommendation.md`（10 份用户副本，**未触碰**）

处置为 `AUTH-02 = OPEN`；SPEC §1.4 记录三方矛盾，**不恢复、不删除、不覆盖**。

## 6. 未执行的操作（停止条件核对）`[FACT]`

| 禁止项 | 状态 |
|---|---|
| 修改 C++ / JNI / Kotlin / Shader 源码 | 未执行 |
| 修改 CMake / Gradle | 未执行 |
| 创建/修改分支；Commit / Push / Merge / Rebase / Reset / Clean / Stash | 未执行 |
| 覆盖或删除用户文件 | 未执行 |
| 内部工作日志追加 | 已执行：仅 `.workbuddy-ai/memory/2026-11-23.md`（会话记录；非设计交付物，未暂存 / 未推送） |
| 创建 Git Worktree | 未执行 |
| 执行 Host Tests / Android Build / APK 安装 / 设备操作 | 未执行 |
| 开始 G0 / G1 / G2；R2/R3 或 PoC-A 源码开发 | 未执行 |
| 将 DRAFT 改为 APPROVED；批准 G0 | 未执行（仍为 v2.1 DRAFT） |

## 7. 遗留 OPEN 事项

- **DEC-01..07**：全部 OPEN（架构/形态/范围/设备/ADR/视觉/参数/遗留）。
- **AUTH-01..07**：全部 OPEN；其中 AUTH-01 须明确 `git worktree add` 改 L2 元数据与基线 SHA 核实。
- **GATE-01..05**：全部 `NOT RUN` / `OPEN-NOT SIGNED`；G3 未签不得进 G4。
- **阈值 TBD**：§13 十项指标除合同值外均为 TBD，需 G0/G2 基线 + DEC-05 参考形态由负责人批准。
- **参数升级**：§9 全部 `PARTIALLY VERIFIED`，升级 `VERIFIED` 需逐参数 Mesh Delta + 自动化差分（G0/G2）。
- **G4 parity**：CPU/GPU 数值 Parity + Android GLES3 在 G4 必须完成，当前 `[UNVERIFIED]`。

## 8. 负责人审核入口 `[OPEN QUESTION]`

逐项签：DEC-01..07（SPEC §7.1）、AUTH-01..07（§7.3）、确认 Gate 冻结（§6）与 G0 Exit Criteria（§12.3）。
未签前：SPEC 保持 v2.1 DRAFT；G0 不执行；G3 未签不得扩展完整人体；AI 不代批视觉 Gate。
