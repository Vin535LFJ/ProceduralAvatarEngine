# SPEC Consolidation — 文档变更清单与 Git 状态报告

> 配套文件：`00-implementation-spec.md`（v2.0 FORMAL CANDIDATE）。
> 本报告为只读审计 + 指定 Markdown 文档修订的记录；**未修改源码、未修改 CMake/Gradle、未操作 Git 分支/提交、未执行构建或设备测试**。
> 标签：`[FACT]` / `[INFERENCE]` / `[RECOMMENDATION]` / `[OPEN QUESTION]`。

---

## 1. 本次修改的文件（仅 1 个）

| 文件 | 修改前 | 修改后 | 性质 |
|---|---|---|---|
| `docs/phases/anatomy-driven-geometry-poc/00-implementation-spec.md` | 341 行，DRAFT 草案（HEAD `6eab7f4` 版本） | 约 600+ 行，DRAFT / FORMAL CANDIDATE，整合版 | 仅 Markdown 设计文档；`git diff --stat` = `417 insertions(+), 278 deletions(-)` |

**未修改**：任何 C++ / JNI / Kotlin / Shader 源码；`native/CMakeLists.txt`；`tests/CMakeLists.txt`；`app/build.gradle.kts`。
`git diff --name-only -- native app tests CMakeLists.txt app/build.gradle.kts` = **空** `[FACT]`。

## 2. 新增内容（对应 §8 交付物 1–9）

| # | 交付物 | 位置 |
|---|---|---|
| 1 | 修订版 `00-implementation-spec.md` | 文件本身；状态 DRAFT / FORMAL CANDIDATE，§17 状态声明 |
| 2 | SPEC-01..09 Closure Matrix | SPEC §5（9 项全部 CLOSED on document layer；6 项残留 OPEN 归属明确） |
| 3 | DECISION Register | SPEC §7.1（DEC-01..07，全部 OPEN）+ §7.2 G3 Decision Record 模板 |
| 4 | AUTHORIZATION Register | SPEC §7.3（AUTH-01..07，全部 OPEN） |
| 5 | G0 Test Execution Matrix | SPEC §12.2（10 项；含命令/输入/输出/生成文件/工作区影响/Mode/风险/授权/失败处理） |
| 6 | R1/R2/R3 Fair Comparison Protocol | SPEC §8（恒定输入、E1/E2/E3 误差分解、Layer A/B、counts 仅成本） |
| 7 | Parameter Traceability Matrix | SPEC §9（27 行；含 6+ 未进顶点参数与 muscle 死代码） |
| 8 | PoC-A Acceptance Matrix | SPEC §14.2（按 Gate 分列） |
| 9 | Mermaid Gate Flow | SPEC §6.1（G0→G4 + KILL/STOP/WAIT 分支） |

第 10 项（本文档）记录文档变更清单与 Git 状态。另：Topology Capability Matrix（SPEC §10）、Skinning Parity（§11）、视觉与算法验收标准（§13）、旧编号映射（§7.5）为额外配套表。

## 3. Git 状态：修改前 → 修改后 `[FACT]`

| 项 | 修改前（15:31 采集） | 修改后（本次复核） |
|---|---|---|
| `git rev-parse HEAD` | `6eab7f4603d3847cfd3dc360c7af670d7f7fabfd` | `6eab7f4603d3847cfd3dc360c7af670d7f7fabfd`（**未变**） |
| Branch | `feature/procedural-character-foundation` | 同（**未变**） |
| Upstream | `origin/feature/...`（0 0 同步） | 同 |
| `main...HEAD` | `0 7` | 同 |
| Staged diff | 空 | 空（**未暂存任何内容**） |
| Tracked diff | 4 files, 28+/1519- | 5 files, 445+/1797-（新增 SPEC 修改 417+/278-） |
| Untracked | 11 项（`.workbuddy-ai/` + 根目录 00–09 副本） | 同 11 项（**未触碰、未纳入、未覆盖**） |

修改前已按任务要求记录 HEAD / Branch / Tracked Diff / Staged Diff / Untracked Files（见 SPEC §1.1–1.3）。

### 3.1 用户原有未提交变更（未触碰）`[FACT]`
- ` D 01_Procedural_Character_Foundation_Specification_v1.0.md`
- ` D 02_Procedural_Character_Development_Roadmap_v1.0.md`
- ` D 03_AI_Engineering_Execution_Prompt_v1.0.md`
- ` M README.md`
- Untracked：`.workbuddy-ai/memory/2026-11-23.md` + `00-current-state-audit.md` … `09-final-recommendation.md`（10 份）

处置为 `AUTH-02 = OPEN`；本 SPEC §1.4 记录三方矛盾但**不恢复、不删除、不覆盖**。

## 4. 未执行的操作（停止条件核对）`[FACT]`

| 禁止项 | 状态 |
|---|---|
| 修改 C++ / JNI / Kotlin / Shader 源码 | 未执行 |
| 修改 CMake / Gradle | 未执行 |
| 创建/修改分支；Commit / Push / Merge / Rebase / Reset / Clean / Stash | 未执行 |
| 覆盖或删除用户文件 | 未执行 |
| 执行 G0 构建与测试（Host Tests / Android Build / 设备） | 未执行 |
| 开始 R2/R3 实现或 PoC-A 源码开发 | 未执行 |
| 将 DRAFT 改为 APPROVED | 未执行（仍为 DRAFT / FORMAL CANDIDATE） |

## 5. 关键事实修订（相对旧 SPEC）

| 项 | 旧值 | 新值（实测） |
|---|---|---|
| HEAD | `486a00d8…` | `6eab7f4603d3847cfd3dc360c7af670d7f7fabfd` |
| `main...feature` | `0 6` | `0 7` |
| tracked 修改 | "无" | 3 删除 + README 修改（用户原有） |
| 决策编号 | D-01..D-10 / A1-A12 / 附录 A1-A11（冲突） | DEC-01..07 / AUTH-01..07 / GATE-01..05（§7.5 映射） |
| 参数状态 | 叙述性 | §9 逐参数 Traceability，未进顶点者标 `NOT IMPLEMENTED` / `DECLARED ONLY` |
| 拓扑状态 | 叙述性 | §10 Capability Matrix，区分已实现与需新增 |
| CPU/GPU Parity | 隐含未验证 | §11 明确 `[UNVERIFIED]` 并带入 GATE-03 / GATE-05 |

## 6. 负责人审核入口 `[OPEN QUESTION]`

需逐项签：DEC-01..07（§7.1）、AUTH-01..07（§7.3）、确认 Gate 冻结（§6）与 G0 Exit Criteria（§12.3）。
未签前：SPEC 保持 DRAFT；G0 不执行；G3 未签不得扩展完整人体；AI 不代批视觉 Gate。
