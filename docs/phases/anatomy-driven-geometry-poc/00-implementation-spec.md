# Anatomy-driven Geometry — Implementation SPEC（正式候选 / FORMAL CANDIDATE，仍为 DRAFT）

> **状态：DRAFT / FORMAL CANDIDATE — 未经项目负责人批准，不授权编码、不授权构建、不授权设备操作。**
> 本文件是评审报告与 PoC-A 投资决策 Gate 计划的整合版，提交负责人审核。所有 DEC-xx / AUTH-xx 未获签字者保持 OPEN。
> 阶段范围：**PoC-G0、G1、G2（PoC-A 对比实验）**；G3 为投资决策；G4 才允许完整人体扩展。P1-B/C/D、肩部/手臂完整生成、头发/服装/面部均不在本 SPEC 授权范围内。
> 依据：`ProceduralAvatarEngine-Anatomy-driven-Geometry-Development-Prompt-v1.0.md`；`docs/proposals/anatomy-driven-geometry-redesign/` README + 00–09 共 11 份；SPEC 独立技术评审报告；PoC-A 投资决策 Gate 计划。
> 文档日期：2026-11-23。标签：`[FACT]` / `[INFERENCE]` / `[RECOMMENDATION]` / `[OPEN QUESTION]`；状态：`VERIFIED` / `PARTIALLY VERIFIED` / `DECLARED ONLY` / `NOT IMPLEMENTED` / `UNVERIFIED` / `REJECTED`。

---

## 0. 本次修订说明（v2.0 FORMAL CANDIDATE）

| 项目 | 说明 |
|---|---|
| 修订目标 | 关闭 SPEC-01…SPEC-09；把评审报告与 Gate 计划整合为一份可批准候选规范 |
| 未改变 | DRAFT 状态；源码；CMake / Gradle；分支；Git 历史；用户文件 |
| 新增章节 | SPEC Closure Matrix（§5）、统一 Gate 体系（§6）、三类 Register（§7）、Fair Comparison Protocol（§8）、Parameter Traceability Matrix（§9）、Topology Capability Matrix（§10）、Skinning Parity（§11）、G0 Test Execution Matrix（§12）、视觉与算法验收标准（§13）、PoC-A Acceptance Matrix（§14）、Mermaid Gate Flow（§6.1） |
| 编号统一 | 旧 A1–A12 / 附录 A1–A11 → `DEC-xx`（架构/形态/范围）、`AUTH-xx`（操作/构建授权）、`GATE-xx`（验收状态），见 §7.4 映射表 |
| 变更清单 | `docs/phases/anatomy-driven-geometry-poc/01-spec-consolidation-change-report.md` |

---

## 1. Git 基线（关闭 SPEC-01 / SPEC-02 / SPEC-03）`[FACT]`

采集时间：2026-11-23 15:31 GMT+8（本次文档修改前）。**以下为实测，不是历史报告转述。**

### 1.1 已提交事实（Committed Facts）

| 项 | 实测值 |
|---|---|
| `git rev-parse --show-toplevel` | `C:/Users/Giant/Desktop/GITCODE/Procedural Avatar Engine` |
| `git rev-parse HEAD` | `6eab7f4603d3847cfd3dc360c7af670d7f7fabfd` |
| HEAD 标题 | `docs: archive historical design docs and add Anatomy-driven Geometry package` |
| HEAD 提交时间 | `2026-11-23 14:57:26 +0800` |
| 当前分支 | `feature/procedural-character-foundation` |
| upstream | `origin/feature/procedural-character-foundation`（`0 0` 同步） |
| remote | `origin https://github.com/Vin535LFJ/ProceduralAvatarEngine.git`（fetch/push） |
| main | `c539afbec299ed4a066429ea09f9d3d36e67f74d` |
| `merge-base(main, HEAD)` | `c539afbec299ed4a066429ea09f9d3d36e67f74d`；main 是 HEAD 的祖先 |
| `rev-list --left-right --count main...HEAD` | `0 7`（main 无独有提交；HEAD 领先 7） |
| `486a00d8…..HEAD` | 仅 1 个新提交 `6eab7f4`，**纯文档**：归档 M 路线/v1.0 资料、新增 11 份设计文档包、本 SPEC 草案、archive 索引；无 C++/JNI/Kotlin 改动 |
| 历史值 `486a00d8e9c5534d53480370be70015bc9c2f158` | 存在且为 HEAD 祖先，仅作历史快照，**不作当前状态** |

### 1.2 工作区未提交变更（Uncommitted, tracked）`[FACT]`

`git diff --stat`：4 files changed, 28 insertions(+), 1519 deletions(-)

| 状态 | 文件 |
|---|---|
| ` D`（未暂存删除） | `01_Procedural_Character_Foundation_Specification_v1.0.md`（658 行） |
| ` D` | `02_Procedural_Character_Development_Roadmap_v1.0.md`（483 行） |
| ` D` | `03_AI_Engineering_Execution_Prompt_v1.0.md`（341 行） |
| ` M` | `README.md`（65 行变动；仍记录旧 HEAD `486a00d…` 与旧分叉数 6） |

`git diff --cached --stat`：**空**（无 staged 变更）。

### 1.3 Untracked 用户文件 `[FACT]`（**未触碰、未 stash、未覆盖、未纳入 G0 Worktree**）

`.workbuddy-ai/memory/2026-11-23.md`；`00-current-state-audit.md`；`01-visual-shape-requirements.md`；`02-anatomical-shape-model.md`；`03-geometry-representation-study.md`；`04-shoulder-and-joint-study.md`；`05-topology-and-rigging-design.md`；`06-target-architecture.md`；`07-poc-roadmap.md`；`08-risk-and-decision-register.md`；`09-final-recommendation.md`。

### 1.4 三类事实冲突与处置（SPEC-03）`[FACT]` `[OPEN QUESTION]`

- committed `docs/archive/README.md:31-33` 称 3 份 v1.0 root 文档"保留原位"；工作区已删除它们；被改写的 root `README.md` 仍记录旧 HEAD 与旧分叉数 6。三者互相矛盾。
- **本 SPEC 不擅自恢复或删除**上述任何文件。归属处置保持 `AUTH-02 = OPEN`，由负责人决定；影响：决定 G0 baseline 是否"干净"、是否可作为可复现证据源。
- 明确区分：(a) 已提交事实 = §1.1；(b) 当前工作区未提交变更 = §1.2；(c) untracked 用户文件 = §1.3。禁止把 (b)(c) 当作 (a) 引用。

---

## 2. 设计包状态与历史定性 `[FACT]`

- 设计包总体为 **PROPOSAL**，不是已批准 Foundation Specification / ADR，也不是已验证技术路线。
- **M11**：完整 Loft/Sweep + 独立 Part 的 buffer 合并（merge ≠ fusion）；历史 14,065 tris / 50.6–53.2 FPS、视觉 `PASS WITH FOLLOW-UPS`（`docs/phases/M11-anatomy-visual-rebuild/05-test-and-acceptance.md:4,23,29-31`）属 REPORTED。
- **M12**：统一 SDF + Surface Nets 得 components=1 / boundary=0 水密表面，但场由椭球/胶囊/圆台构成；24,360 tris / ~51.7 FPS、8/8 47 cases、Gate A 待用户确认（`docs/phases/M12-unified-human-geometry/06-test-and-acceptance.md:8,16-27,36-38,87-89`）属 REPORTED。"水密 ≠ 自然人体"。
- **P1-A**：**REJECTED**（项目方 Gate）。工程 9/9 PASS、870 verts / 1664 tris / boundary=80 / gen 2.419ms / 46–55 FPS，视觉 **REJECTED**，7 项缺陷含 5 项 P0（`docs/phases/P1A-geometry-poc/05-implementation-report.md:33-36`；`06-visual-defect-report.md:5,12-18`）。原文："工程指标（拓扑/性能/测试）合格，但无法抵消视觉质量不达标"（:18）。**不继续修补，不进入 P1-B**。
- 三套几何路径在 HEAD 全部并存且开机全生成：`RenderThread.cpp:568,592,613`。这是"模块增加但质量无实质改善"的直接证据 `[FACT]` `[VERIFIED]`。
- 本 SPEC 不认定任一历史路线为最终技术选择；M12 提取器与 P1-A 失败证据仅作 G2 对照基线。

### 2.1 ADR / Contract 冲突表（保留，`[FACT]`）

| ID | 旧合同 / 决策 | 适用性判定 | 新路线处理 |
|---|---|---|---|
| C-01 | Foundation 03 主线：Surface Patch / Sweep / Patch Assembler | 仅适用 P1-A 旧 PoC；已 REJECTED | 不直接沿用；需新 ADR 裁决（DEC-04） |
| C-02 | ADR-009 统一 SDF 上半身（main Accepted） | feature 被 ADR-010 取代；M12 证据保留 | SDF/Surface Nets 作 R1 对照，不是默认终局 |
| C-03 | ADR-010 P1 Surface Patch 主线（feature Accepted，待用户 Gate） | PoC 视觉失败，ADR 文本仍存在 | 不改写；新 ADR 单独编号说明取代/兼容范围 |
| C-04 | Foundation 02：BoneId 0–21、父子链、坐标/米制 | 仍有效，ABI 冻结 | **必须保持** |
| C-05 | Foundation 04 / ADR-005：UBO `mat4[22]` 1408B、binding 0、64B packing | 仍有效 | **必须保持**；RegionId/UV/seam 先作 CPU metadata |
| C-06 | Foundation 05：视觉协议、AI 不得代批 Gate | 仍有效 | 严格执行；GATE-04 由负责人签字 |
| C-07 | Foundation 06：Worker/RenderThread 分离、GenerationVersion、CPU cache | 合同有效但**实现未满足**（§4.4） | G0 记录现状；本轮不重写 RenderThread |
| C-08 | JNI/Kotlin 对外接口 | 仍有效 | 保持函数签名；新增仅内部 previewMode 通道 |

---

## 3. 源码事实基线（`[FACT]`，逐行核对）

### 3.1 历史缺陷
- C0/G1 硬编码：`BodyPatchGenerator.cpp:331-350` 写死 `maxPositionDeviation=0`、`maxNormalDeviationDeg=0`、`smoothSeam=true`、`valid=true`，另 3 条为副本；`character_patch_tests.cpp:53-54` 断言这些零值 → 伪绿。**历史 C0/G1 证据 NOT MEASURED，不可采信**。
- `BodyPatchGenerator.cpp:97-101` 含 `const Skeleton& skeleton` 但未读取 → 非 Rig-aware。
- 符号约定：`SdfFusion.cpp:53` 为 `+ k*h*(1-h)`；`AnatomyField.cpp:109` 为 `- k*h*(1-h)`；均声称 negative inside。差异真实但生产路径只用 AnatomyField 版（`fusion::sdSmoothUnion` 仅测试调用）→ 休眠缺陷，禁止猜改符号，须先数学推导 + 双实现单测统一（列入 G2）。
- Muscle features：`AnatomyField.cpp:212-348` 构建 trap/pec/biceps/triceps/brachialis/rectus/intercostal/lumbar/flex/ext 原语，但 `side()`（:407-450）与 `regionFields()`（:458-483）**不消费**它们；`AnatomyFeatures.cpp` 未进 `native/CMakeLists.txt:10-32`，`baseMaleFeatures()` 零调用 → 死代码。
- PTF：`ParametricSurfaces.cpp:26-56` 无重复点清理与显式反平行分支；`BodyPatchGenerator.cpp:33-57` 同样；测试仅覆盖温和情形，未覆盖近 180° 反平行/重复点。

### 3.2 几何与拓扑
- 默认渲染路径是独立 mesh append（`Humanoid.cpp:451-466` buffer 拼接），不是统一曲面。
- P1-A patch 路径用共享顶点索引做桥接（`BodyPatchGenerator.cpp:244-318`），previewMode=2，历史 REJECTED。
- `MeshTopology.h:10-20` 仅提供 components / boundaryEdges / nonManifoldEdges / degenerateTriangles / duplicateVertices / invalidNormals / boundarySamples；**无** non-manifold vertex、winding、outward normal、self-intersection、aspect/sliver、UV/attribute、孤立点、区域边界。详见 §10。
- 无 stable vertex ID / face connectivity / joint loop / edge flow / reference-to-topology fitting；`surfaceQuads`（`SdfFusion.h:31-33`）仅为 dual-lattice 待分析数据。

### 3.3 Skeleton / Skinning
- BoneId 0–21 冻结（`Skeleton.h:10-34`），层级 `Skeleton.cpp:31-62`，bindLocal 仅平移（:64-68），inverseBind（:74-78）。
- 主路径 top-4 归一化权重（`WeightGenerator.cpp:82-106,132-139,166-173`）；anatomy 路径仅 2 权重（`HumanAnatomyGenerator.cpp:112-117`）。
- GLSL `layout(std140) uniform BoneBlock { mat4 uBones[22]; }`（`RenderThread.cpp:21`；outline :130）；UBO 1408B binding 0（`BonePaletteUbo.h:10`）；64B packing pos@0/normal@12/joints@24/weights@32/color@48（`SkinnedGpuMesh.cpp:23-50`），UV 未上传。
- CPU LBS（`Skinning.cpp:25-39`）与 GPU LBS（`RenderThread.cpp:32-39`）数学等价；**数值 parity 未经设备实测** → `[UNVERIFIED]`，见 §11。

### 3.4 Renderer / 线程 / 设备（G0 待复核项）
- 同步生成：`RenderThread.cpp:412 → 499-634`（:568/:592/:613）；`SetQuality` 亦在 :474 同步重建；无 geometry worker thread（`std::thread` 仅 :218）。
- 资源缺口：`releaseGpuResources()`（`RenderThread.cpp:636-643`）未释放 `anatomyMesh_` / `anatomyLines_`；`EglContextManager::recreate()` 定义但未调用；无 `EGL_CONTEXT_LOST` 分支。
- GenerationVersion：正式机制不存在；只有诊断计数器 `avatarRevision_`/`sceneRevision_`（`RenderThread.h:126-127`），不用于过期丢弃。
- 设备：仅 `nunai_max`（`MAX0019111000104`，Android 8.1/API27，arm64-v8a，GLES3.0 native context）在线（`docs/performance/device-matrix.md:7-8`）；Android 手机 `[UNVERIFIED]`。
- 图片：99 张 PNG 未逐张像素复核；P1-A 文档引用的 `M12/evidence/dv3_presetN.png` 不存在（M12 仅 dv0/dv1/dv2）→ 悬空引用，G0 标 `[UNVERIFIED]` 并重新采集。

---

## 4. 历史问题状态表（`[FACT]`）

| 问题 | 判定 | 状态 |
|---|---|---|
| C0/G1 指标硬编码 | 仍硬编码，测试使其变绿 | `VERIFIED`（缺陷仍在）/ `REJECTED`（作为证据） |
| SdfFusion vs AnatomyField 符号差异 | 代码差异真实，生产路径未组合 | `VERIFIED`（休眠） |
| Muscle features 进入最终几何 | 未进入；死代码 | `VERIFIED` / `NOT IMPLEMENTED` |
| RenderThread 同步生成 | 成立 | `VERIFIED` |
| anatomyMesh_/anatomyLines_ 生命周期 | 释放缺口仍在 | `VERIFIED` |
| GenerationVersion 缺失 | 正式机制缺失，有诊断计数器 | `PARTIALLY VERIFIED` |
| M12 dv3 图片引用 | 悬空 | `VERIFIED` |
| 99 张图逐张复核 | 未完成 | `UNVERIFIED` |
| CPU/GPU skin parity 数值 | 未测 | `UNVERIFIED` |

---

## 5. SPEC-01…SPEC-09 Closure Matrix

| ID | 原缺陷 | 关闭方式 | 关闭状态 | 残留 / 归属 |
|---|---|---|---|---|
| SPEC-01 | §1 Git 事实过时（HEAD 486a00d、分叉 0 6、无 tracked 修改） | §1.1 用实测 HEAD `6eab7f4`、分叉 `0 7`、tracked diff 重写；三类事实分列 | **CLOSED** `[FACT]` `[VERIFIED]` | 无 |
| SPEC-02 | 未反映 `486a00d..HEAD` 新提交 | §1.1 记录唯一新提交 `6eab7f4`，说明其纯文档、不改源码结论；源码结论仍成立 | **CLOSED** `[FACT]` `[VERIFIED]` | 无 |
| SPEC-03 | Archive README / root v1.0 / README 矛盾 | §1.4 明确三方冲突，分列三类事实，不恢复不删除用户文件 | **CLOSED（记录层）** `[FACT]` | **归属处置仍 OPEN → `AUTH-02`**；影响 G0 baseline 干净度 |
| SPEC-04 | R2/R3 无实现却当作可比路线 | §6 Gate 链规定 G1 必须先产出 R2/R3 最小实现；§8 公平比较协议；§10 拓扑能力矩阵 | **CLOSED（规则层）** `[RECOMMENDATION]` | R2/R3 代码仍 `NOT IMPLEMENTED`，需 `AUTH-07` 授权后才实现 |
| SPEC-05 | 缺两层比较框架 / counts 直接比较 | §8 定义 Layer A Reference Surface Quality + Layer B Canonical Mesh Quality；counts 仅成本指标 | **CLOSED** `[RECOMMENDATION]` | 阈值 TBD（§13），需负责人批准 |
| SPEC-06 | G0 缺执行字段 | §12 G0 Test Execution Matrix 列全命令/输入/输出/生成文件/工作区影响/风险/授权/失败处理 | **CLOSED** `[FACT]` `[RECOMMENDATION]` | 各命令需 `AUTH-03..06` |
| SPEC-07 | 参数真实性无清单 | §9 Parameter Traceability Matrix 全字段；未进顶点者标 NOT IMPLEMENTED / DECLARED ONLY | **CLOSED** `[FACT]` `[VERIFIED]` | 处理方式需 `DEC-06` 裁决 |
| SPEC-08 | 拓扑能力缺口未登记 | §10 Topology Capability Matrix 区分已实现与 PoC-A 需新增 | **CLOSED** `[FACT]` `[VERIFIED]` | stable ID/connectivity 等需新增设计 |
| SPEC-09 | CPU/GPU parity 未带入 Gate | §11 明确 `[UNVERIFIED]`；带入 GATE-03（G2）与 GATE-05（G4） | **CLOSED（标注层）** `[FACT]` | 阈值实测前不得写 VERIFIED |

**结论** `[FACT]`：9 项缺陷在文档层全部关闭；其中 SPEC-03 的**文件归属处置**、SPEC-04 的**R2/R3 实现授权**、SPEC-05 的**阈值批准**、SPEC-06 的**命令授权**、SPEC-07 的**参数处理决策**、SPEC-09 的**数值实测**仍为 OPEN，不构成"已批准"。

---

## 6. 统一 Gate 体系（正式冻结）

主链 **G0 → G1 → G2 → G3 → G4**，不可跳级。核心约束：**GATE-04（G3 投资决策）未经项目负责人签署，不允许扩展完整人体；AI 不得自行批准。**

### 6.1 Mermaid Gate Flow

```mermaid
flowchart TD
    S[START: DRAFT candidate SPEC] --> G0["GATE-01 / G0<br/>Evidence Baseline"]
    G0 -->|entry: worktree + AUTH-01/02/03/04/05/06| G0E{证据齐全<br/>可复现?}
    G0E -->|NO| STOP0[STOP: 补证据; 禁 G1]
    G0E -->|YES| G1["GATE-02 / G1<br/>Route Feasibility<br/>R1 已有; R2/R3 最小实现"]
    G1 -->|entry: AUTH-07; 单路线顺序| G1E{每条路线产出<br/>合规 MeshData?}
    G1E -->|NO| KILL1[KILL 该路线; 记录原因]
    G1E -->|YES| G2["GATE-03 / G2<br/>Fair Comparison<br/>Layer A + Layer B"]
    G2 -->|entry: 统一输入/采样/参数| G2E{kill 触发?<br/>P0 视觉 / 拓扑不稳<br/>/ 硬编码指标 / parity 未测}
    G2E -->|YES| STOP2[STOP: 回退; 禁 G3]
    G2E -->|NO| G3["GATE-04 / G3<br/>Owner Investment Decision<br/>唯一 approved route"]
    G3 -->|未签 = OPEN| WAIT[WAIT: 负责人签字; AI 不代批]
    G3 -->|REJECT / DEFER| BACK[回 G2 补证据 / 路线 KILL]
    G3 -->|签: approved route| G4["GATE-05 / G4<br/>Full Character Expansion<br/>PoC-B → C → D → 全身"]
    G4 -->|kill 触发| STOP4[STOP: 回退]
    G4 -->|通过| NEXT[提交下一阶段提案]
```

### 6.2 Gate 定义表

| Gate | Entry Criteria | Required Evidence | Exit Criteria | Owner | Failure Handling | Authorization Boundary |
|---|---|---|---|---|---|---|
| **GATE-01 / G0** Evidence Baseline | 负责人批准候选 SPEC；`AUTH-01` worktree 隔离；`AUTH-02` 工作区归属已定；`AUTH-03..06` 授权 | §12 全部 10 项产出；Git baseline 可复现；99 PNG manifest；dv3 标 UNVERIFIED | 证据来自真实计算，无硬编码 PASS/零偏差；固定相机协议被代码/测试引用；baseline 两次同 preset 差分在阈值内 | 工程执行，负责人复核 | 证据不全 → STOP，补证据；不进 G1 | 只读 + 明确构建/设备授权；不改源码；不 commit |
| **GATE-02 / G1** Route Feasibility | G0 通过；`AUTH-07` 授权；R2/R3 最小实现（**顺序**进行，不并发无约束扩展） | 每路线在同 Shape Model/参数/参考面下产出合规 `MeshData`；组件边界与生成耗时 | 三路线均能产出合规网格；不能者该路线 KILL 并记录 | 工程 | KILL 该路线；不影响其他路线 | 仅新增 previewMode 通道；不触碰遗留三路径；ABI 冻结 |
| **GATE-03 / G2** Fair Comparison | G1 齐备；§8 协议生效；§13 阈值已批 | Layer A 参考面质量报告 + Layer B 规范网格报告；重拓扑误差单列；stable ID/连接性记录 | 指标可复算；差异可归因于表示法；counts 仅成本；kill 项全清 | 工程实现，负责人复核 | 任一 kill 触发 → STOP，回退；不进 G3 | 不改 ABI；不改冻结合同 |
| **GATE-04 / G3** Owner Investment Decision | G2 报告齐备 | G3 Decision Record（§7.2 模板）：每路线证据、推荐、后果、kill 记录 | **负责人签署唯一 `approved route`**；AI 不得自批 | **项目负责人签字** | 未签 = OPEN；REJECT = 路线 KILL；DEFER = 补证据重开 G2 | 无签字不得进 G4 |
| **GATE-05 / G4** Full Character Expansion | G3 已签 approved route | 沿批准路线按 PoC-B→C→D→全身；每阶段独立 Gate | 单一批准路线扩展；任一 kill → 回退 | 项目负责人 | STOP，不静默带过 | 新模块必须归属批准路线；遗留冻结 |

---

## 7. Register 体系（DECISION / AUTHORIZATION / GATE）

### 7.1 DECISION Register（架构、形态、范围）— 全部 OPEN

| ID | Question | Options | Recommendation（**非批准**） | Rationale / Consequences | Validation | Approval Owner | 状态 |
|---|---|---|---|---|---|---|---|
| DEC-01 | 基础体型 archetype | ①单一中性轻度运动型 H≈1.70m；②男性化；③女性化；④多 profile | ①只锁一种，先不支持多 profile | 单一 baseline 可复现、可归因；多 profile 污染变量控制。换体型需重跑 | GATE-03 固定机位视觉审查 | 负责人 | `OPEN` |
| DEC-02 | PoC-A 几何范围 | ①胸腹腰盆上部+双侧肩连接锚点、不含上臂；②截面/单臂；③含完整肩 | ①接受躯干/腰盆/肩连接锚点，不做完整手臂 | 与"不提前开发肩/手臂"一致；crop 需显式登记 | 几何边界审查 + manifest | 负责人 | `OPEN` |
| DEC-03 | 设备策略 | ①TV Box + 手机；②仅 TV Box + 书面豁免；③补设备 | ②当前允许 TV Box，手机维持 `[UNVERIFIED]`；豁免由负责人确认 | 仅 `nunai_max` 在线；手机路径硬件 BLOCKED。TV Box 结果不得写成手机通过 | 真机 P50/P95/P99 + APK SHA | 负责人 | `OPEN` |
| DEC-04 | ADR-011 授权 | ①新建 ADR-011，不覆盖旧 ADR；②改写旧 ADR；③不建 | ①新建 ADR-011，不覆盖 ADR-008/009/010 | ADR-010 仍 Accepted（待 P0 Gate）；冻结文档不应被覆盖 | 负责人签字 ADR-011 后更新状态入口 | 负责人 | `OPEN` |
| DEC-05 | 目标参考形态与视觉验收数据 | ①负责人提供授权参考图 + rubric；②仅灰模协议无外部参考；③AI 自定 | ①由负责人确认目标形态和标准 | **没有负责人认可的目标形态，AI 只能做数值对照，不能宣布视觉合格** | 固定机位视觉 Gate，owner 独立否决 | 负责人 | `OPEN` |
| DEC-06 | 参数与 Muscle Feature 处理原则 | ①标记未实现、G1/G2 重建；②把半成品当已实现；③删除历史参数 | ①声明未进顶点者标 `NOT IMPLEMENTED`/`DECLARED ONLY`；muscle feature 归入 Shape Model 重新设计，不复用半成品 API | `AnatomyFeatures.cpp` 死代码；6+ 参数不进顶点（§9） | G0 Parameter Baseline + G2 参数组对照 | 负责人 | `OPEN` |
| DEC-07 | 遗留路线冻结原则 | ①三路径冻结为基线、不删不静默替换；②删除 legacy；③静默替换 | ①冻结为对照基线，新能力走独立 previewMode | ADR-010:22 / 缺陷报告:6 要求保留；避免"堆模块"重演 | `previewMode_` 回归 0/1/2；Git diff 审计 | 负责人 | `OPEN` |

### 7.2 G3 Decision Record 模板（GATE-04 签署物）

字段：选项 R1 / R2 / R3（+可选 R4 Dual Contouring，无理由不扩大）；每选项 Layer A/B 证据、截图、kill 记录；推荐项；后果（ABI/主线/遗留影响）；kill criteria；**`approved route`（唯一，默认空）**；owner 签字 / 日期 / 状态 `OPEN`。
`[RECOMMENDATION]` 未签即 OPEN，不得进 G4。REJECT = 该路线 KILL 并记原因；DEFER = 补指定证据后重开 G2，不得默认为通过。

### 7.3 AUTHORIZATION Register（操作与构建授权）— 全部 OPEN

| ID | 操作 | 范围 / 命令 | 生成文件 | 修改工作区? | 风险 | 状态 |
|---|---|---|---|---|---|---|
| AUTH-01 | Git Worktree 隔离 | 基于**重新核实**的 baseline commit 建独立 worktree；当前未提交变更与 untracked 不纳入、不覆盖 | worktree 目录 | 否（独立目录） | 低 | `OPEN` |
| AUTH-02 | 未提交文件处置 | 3 份 root v1.0 删除 + README 改写：保留原状 / 恢复 / 负责人另行处置；**AI 不擅自恢复、删除、覆盖** | 无 | 视决策 | 中（决定 baseline 干净度） | `OPEN` |
| AUTH-03 | G0 Host Tests | `cmake -S tests -B <worktree/build> && ctest --test-dir <worktree/build> --output-on-failure` | build 目录 + 输出 | 否（worktree 内） | 低 | `OPEN` |
| AUTH-04 | Android Build | `./gradlew assembleDebug` | APK + SHA256 | 否（worktree 内） | 中（SDK/网络） | `OPEN` |
| AUTH-05 | TV Box 安装与证据采集 | `adb install -r <apk>` + 固定 preset 截图 + `adb logcat` | evidence 截图/日志 | 设备状态 | 中 | `OPEN` |
| AUTH-06 | 其他可能产生工作区修改的命令 | 逐条列明后单独授权；未列明者禁止执行 | 视命令 | 可能 | 视命令 | `OPEN` |
| AUTH-07 | G1/G2 实现（新建 R2/R3 最小实验） | 新 previewMode 通道；不触碰遗留三路径；不改 ABI | 新源码/测试文件（worktree 内） | 是（worktree 内新增） | 中 | `OPEN` |

### 7.4 GATE Register（验收状态）— 全部 NOT RUN

| ID | Gate | 状态 | 签署人 |
|---|---|---|---|
| GATE-01 | G0 Evidence Baseline | `NOT RUN` | 负责人复核 |
| GATE-02 | G1 Route Feasibility | `NOT RUN` | 工程 |
| GATE-03 | G2 Fair Comparison | `NOT RUN` | 负责人复核 |
| GATE-04 | G3 Owner Investment Decision | `OPEN / NOT SIGNED` | **负责人** |
| GATE-05 | G4 Full Character Expansion | `NOT RUN` | 负责人 |

### 7.5 旧编号 → 统一编号映射（消除 A1–A12 / 附录 A1–A11 冲突）

| 用户建议/旧编号 | 统一编号 | 说明 |
|---|---|---|
| 建议 DEC-01 目标体型 | `DEC-01` | 架构/形态决策 |
| 建议 DEC-02 PoC-A 范围 | `DEC-02` | 范围决策 |
| 建议 DEC-03 设备策略 | `DEC-03` | 范围决策 |
| 建议 DEC-04 ADR | `DEC-04` | 治理决策 |
| 建议 DEC-05 视觉参考 | `DEC-05` | 形态决策 |
| 建议 DEC-06 Git 隔离 | `AUTH-01` | **操作授权**，按分类归 AUTH，避免与 DEC-06（参数）冲突 |
| 建议 DEC-07 未提交变更 | `AUTH-02` | **操作授权** |
| 建议 DEC-08 投资 Gate | `GATE-04` | **验收状态**，按分类归 GATE |
| 旧主计划 A1–A12 / 附录 A1–A11 | 分配至 `DEC-01..07` / `AUTH-01..07` / `GATE-01..05` | 全部保持 OPEN，见 §7.1/7.3/7.4 |

---

## 8. R1/R2/R3 Fair Comparison Protocol（关闭 SPEC-04 / SPEC-05）

### 8.1 恒定输入（必须统一，否则对比无效）

同一 Shape Model / 同一 `AnatomicalLandmarks` / 同一解剖参数集 / 同一 Reference Surface 定义 / 同一 seed / 同一单位与坐标系（米；右手系 +Y Up / +Z Forward / +X Character Left）/ 同一相机-光照-灰模协议（7 固定视角）/ 同一 `MeshData` 契约 / **同一误差计算方法与采样密度**。
`[FACT]` 现状：R1 有代码（`SdfFusion.cpp:64-242`、`AnatomyField.cpp`、`HumanAnatomyGenerator.cpp`）；R2 / R3 `NOT IMPLEMENTED`，仅文档。故 G1 必须先产出最小实现。
**任一路线单独调参即判定对比无效。**

### 8.2 误差分解（三项必须分别记录）

| 误差项 | 定义 | 归属 |
|---|---|---|
| E1 R1 原始表面误差 | R1 输出 **Reference Surface** 对目标 Shape Model/参考面的采样距离（Hausdorff / RMS），与拓扑无关 | Layer A |
| E2 Reference Surface → Canonical Topology 拟合误差 | 把参考面投影/重采样到固定 quad template 产生的偏差，**单独记录，不与 E1 合并** | Layer B 前置 |
| E3 最终统一拓扑质量 | 归一化到同一拓扑后的 edge flow / joint loop / 面质量 / 权重稳定性 | Layer B |

重拓扑操作（E2）产生的误差**必须单列并归因**，不得混入 E1 或 E3。

### 8.3 两层指标体系

**Layer A — Reference Surface Quality（与拓扑表达无关）**
1. 双向 Hausdorff / RMS 到同一参考面（统一采样方法、样本数）。
2. 解剖 landmark 位置误差（同一集合、同一单位）。
3. 轮廓/截面偏差（front/side/45 数值 contour 差）。
4. 法线场一致性（同一参考法线，角度分布）。
5. 视觉 gray-clay 固定机位（owner Gate，独立否决）。

**Layer B — Canonical Mesh Quality（先归一化拓扑）**
1. 先把三路线重采样到**同一固定 quad template**，统一顶点数与连接关系；之后才比较 edge flow、joint loop、面质量、权重稳定性。
2. Stable Vertex ID / Face Connectivity 稳定性：同一 ID 在参数变化前后是否指向同一解剖位置；face 邻接是否恒定。
3. manifold / boundary / degenerate / self-intersection / winding / outward normal 全报告。
4. **Triangle Count / Vertex Count 仅作成本指标，不作质量排名**（`06-visual-defect-report.md:18`：工程指标不能替代视觉 Gate）。

### 8.4 样本与归因
每路线 7 视角 + ≥2 组参数（Baseline / Chest Width-Depth / Waist-Pelvis Ratio）；禁止参数特调；差异只能归因于表示法。R4 Dual Contouring 可选，无理由不扩大范围。

---

## 9. Parameter Traceability Matrix（关闭 SPEC-07）

列定义：Parameter → Declaration → Read Location → Geometry Dependency → Vertex Impact → Region Impact → Validation / Status。
依据 `HumanAnatomyParameters`（`AnatomyField.h:18-51`）、`buildPrimitives`（`AnatomyField.cpp:212-348`）、`makeUpperBodyField`（`AnatomyField.cpp:386-456`）、`regionFields`（`:458-483`）。

| Parameter | Declaration | Read Location | Geometry Dependency | Vertex Impact | Region Impact | Validation / Status |
|---|---|---|---|---|---|---|
| torsoHeight | `AnatomyField.h:20` | 0 处 | 无 | 无 | 无 | `NOT IMPLEMENTED` |
| shoulderWidth | `h:21` | `cpp:261-263`（trapRa/trapRbBelly/trapRb） | trap 原语 | **无**：trap 未被 `side()`/regionFields 消费 | 无 | `DECLARED ONLY`（构建后丢弃） |
| chestWidth | `h:22` | `cpp:219`（ribCageR.x）、`:270`（pecRbBelly） | ribCage → `side():413` | **有**（经 ribCage） | Thorax | `VERIFIED`；`anatomy_tests.cpp:169` 胸宽变化 >3cm |
| chestDepth | `h:23` | `cpp:221`、`:267`(pec)、`:324`(rectus) | ribCage → `:413` | 有 | Thorax | `VERIFIED`（pec/rectus 部分丢弃） |
| waistWidth | `h:24` | `cpp:224`、`:338`(lumbar) | abdomen → `:415` | 有 | Abdomen / Waist | `VERIFIED`（lumbar 丢弃） |
| waistDepth | `h:25` | `cpp:226`、`:325`(rectus) | abdomen → `:415` | 有 | Abdomen / Waist | `VERIFIED` |
| pelvisWidth | `h:26` | `cpp:229`、`:339`(lumbar) | pelvis → `:416` | 有 | Pelvis | `VERIFIED` |
| ribCageVolume | `h:29` | `cpp:222` | ribCage → `:413` | 有 | Thorax | `VERIFIED` |
| abdominalVolume | `h:30` | `cpp:227` | abdomen → `:415` | 有 | Abdomen | `VERIFIED` |
| backDepth | `h:31` | `cpp:233`（backCarveC.z） | back carve → `:419` | 有 | Back | `VERIFIED` |
| waistCurvature | `h:32` | `cpp:224`、`:338` | abdomen → `:415` | 有 | Waist | `VERIFIED` |
| clavicleWidth | `h:35` | `cpp:238,241` | clavicle → `:425` | 有 | ShoulderRoot | `VERIFIED` |
| deltoidVolume | `h:36` | `cpp:248,250-251` | deltoid → `:424` | 有 | ShoulderRoot | `VERIFIED` |
| shoulderSlope | `h:37` | `cpp:239,249,274` | clavicle/deltoid/arm → `:425,:424,:430` | 有 | ShoulderRoot / Arm | `VERIFIED` |
| axillaDepth | `h:38` | `cpp:405`（kShoulder） | shoulder blend → `:427` | 有（blend 锐度，**非真实凹腋窝**） | Axilla | `PARTIALLY VERIFIED`；凹面需 G2 验证 |
| upperArmLength | `h:41` | 0 处 | 无 | 无 | 无 | `NOT IMPLEMENTED` |
| upperArmVolume | `h:42` | `cpp:276,286,292` | upperArm → `:430` | 有 | Arm | `VERIFIED`（biceps/triceps 丢弃） |
| elbowWidth | `h:43` | `cpp:275,277,279` | upperArm/elbow → `:430-431` | 有 | Elbow | `VERIFIED` |
| forearmLength | `h:44` | `cpp:303` | forearm → `:433` | 有 | Forearm | `VERIFIED` |
| forearmVolume | `h:45` | `cpp:304,307,314,319` | forearm/spindle → `:433,:439` | 有 | Forearm | `VERIFIED` |
| wristWidth | `h:46` | `cpp:305` | forearm → `:434` | 有 | Wrist | `VERIFIED` |
| stylization | `h:49` | 0 处 | 无 | 无 | 无 | `NOT IMPLEMENTED` |
| smoothness | `h:50` | `cpp:404-405,411-448` | 所有 smooth-union k | 有（全局混合） | 全局 | `VERIFIED` |
| BodyParameters::stylization | `Humanoid.h:82` | clamp `Humanoid.cpp:136` | 无 | 无 | 无 | `NOT IMPLEMENTED` |
| TorsoShape::backFlatness | `Humanoid.h:21` | clamp `Humanoid.cpp:144` | 无 | 无 | 无 | `NOT IMPLEMENTED` |
| CharacterDefinition::armDepth | `CharacterDefinition.h:19` | 0 处 | 无 | 无 | 无 | `NOT IMPLEMENTED` |
| MuscleShape.{sectionScale,bellyAt,bellyGain,squash,roll,groove,blend} | `AnatomyFeatures.h:23-39` | `baseMaleFeatures()`（`.cpp:5`），零调用；文件未进 CMake | 无 | 无 | 无 | `NOT IMPLEMENTED`（死代码） |
| AnatomyFeatureSet.* | `AnatomyFeatures.h:44-69` | 同上 | 无 | 无 | 无 | `NOT IMPLEMENTED` |

**结论** `[FACT]`：6+ 高层参数与整个 muscle feature 系统未进入最终顶点。处理方式待 `DEC-06`。

---

## 10. Topology Capability Matrix（关闭 SPEC-08）

| 能力 | 当前实现 | 证据 | PoC-A / G2 需要 | 新增 or 已有 |
|---|---|---|---|---|
| Stable Vertex ID | 无 | 全局无符号 | 必须（跨参数稳定） | **新增** |
| Stable Face Connectivity | 无 | 全局无符号 | 必须 | **新增** |
| Joint Loops | 无 | 仅文档提案 | 必须（肩/肘环线） | **新增** |
| Edge Flow | 无 | 无强制；`surfaceQuads` 仅 dual-lattice 数据（`SdfFusion.h:31-33`） | 必须（动画拓扑） | **新增** |
| Reference-to-Topology Fitting | 无 | 无投影实现 | 必须（E2 误差单独记录） | **新增** |
| Manifold（edge） | 有 | `MeshTopology.h:15`、`MeshTopology.cpp:64` nonManifoldEdges | 必须 | 已有（保留） |
| Manifold（vertex） | 无 | 校验器无 vertex link audit | 必须 | **新增** |
| Boundary | 有 | `MeshTopology.h:14` boundaryEdges；crop 需显式登记 | 必须 | 已有（补 crop 语义） |
| Degenerate Face | 有 | `MeshTopology.h:16` degenerateTriangles | 必须 | 已有 |
| Self-intersection | 无 | 校验器无 | 必须 | **新增** |
| Winding / Outward Normal | 部分 | `SdfFusion.cpp:173,223` gradient winding；无通用校验 | 必须 | **新增（通用校验）** |
| Duplicate Vertices / Invalid Normals | 有 | `MeshTopology.h:17-18` | 必须 | 已有 |
| Aspect / Sliver | 无 | 校验器无 | 建议（G2 记录） | **新增** |
| UV / Attribute | 未上传 | `MeshData.h:14` 有 UV；64B packing 无 UV；C-05 保持 | CPU metadata 优先 | 保持（不改 ABI） |
| Region Boundary | 无 | 校验器无 | 必须（参数影响区域） | **新增** |

---

## 11. CPU/GPU Skinning Parity（关闭 SPEC-09）`[FACT]`

- 实现状态：CPU LBS（`Skinning.cpp:25-39`）、GPU LBS（`RenderThread.cpp:32-39`）；UBO `mat4[22]` 1408B binding 0；64B packing；BoneId 0–21。
- 数学关系：对刚体、归一化权重二者等价（CPU `1/transformed.w` 对 Σw=1 的仿射矩阵为 no-op）。
- **数值 parity 尚未实测** → `[UNVERIFIED]`。不得写 VERIFIED。
- 带入 Gate：GATE-03（G2）记录同一 PoseFrame 的 CPU/GPU 顶点差与阈值；GATE-05（G4）前必须完成；阈值由设备精度实验确定，未测前 TBD。
- 权重约束：主路径 ≤4、非负、和=1±1e-3、ID∈[0,21]；anatomy 路径仅 2 权重（`HumanAnatomyGenerator.cpp:112-117`），属现状差异，G2 需记录。

---

## 12. G0 规范与 G0 Test Execution Matrix（关闭 SPEC-06）

### 12.1 隔离策略 `[RECOMMENDATION]`
保留独立 Worktree。Baseline Commit 必须先重新核实（当前 HEAD `6eab7f4`，授权时重新 `git rev-parse HEAD` 确认）。**当前未提交工作区变更与 untracked 文件不得自动纳入 Worktree 或覆盖**（AUTH-01 / AUTH-02）。

### 12.2 Execution Matrix

图例：Mode = 只读 / 构建 / 设备。所有命令在 worktree 内执行；未获对应 AUTH 者禁止执行。

| # | 项 | 命令 / 工具 | 输入 | 预期输出 | 生成文件 | 修改工作区 | Mode | 风险 | 授权 | 失败处理 |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Git Baseline | `git rev-parse HEAD`；`git status -sb`；`git worktree list` | 当前仓库 | HEAD/分支/worktree 路径记录 | 报告文本 | 否 | 只读 | 低 | 无需额外（只读） | 若 HEAD 与 §1.1 不同 → 更新基线并重核 |
| 2 | Host Tests | `cmake -S tests -B <worktree/build> && ctest --test-dir <worktree/build> --output-on-failure` | `tests/unit/*` | 真实 ctest 输出（PASS/FAIL 明细） | build 目录 | 否（worktree 内） | 构建 | 低 | AUTH-03 | FAIL 如实记录，不改弱断言 |
| 3 | Android Build | `./gradlew assembleDebug` | `app/` | APK + SHA256 | `app/build` | 否（worktree 内） | 构建 | 中（SDK/网络） | AUTH-04 | 记录错误与环境，不猜测 |
| 4 | 历史图片清单 & 证据完整性 | 只读目录遍历 + manifest；逐张人工查看 | `docs/**/evidence/*.png`（99 张） | 清单：commit/preset/debugMode/device/APK SHA；dv3 标 UNVERIFIED | manifest 文本 | 否 | 只读 | 低 | 无需额外 | 缺字段标 `[UNVERIFIED]`，不沿用 dv3 |
| 5 | R1 Geometry Baseline | 现有 generator + 导出工具（工具存在性需先确认） | `HumanAnatomyGenerator` 参数集 | components/boundary/nonmanifold/winding 真实报告 | 报告 | 否 | 只读/运行 | 中 | AUTH-06（若需工具） | 工具不存在 → 标 NOT RUN，先授权 |
| 6 | Topology Baseline | `MeshTopology` 分析器 | 同上 | 顶点/面/连通/边界/退化统计 | 报告 | 否 | 只读/运行 | 低 | AUTH-06 | 缺失能力按 §10 标 NOT IMPLEMENTED |
| 7 | Parameter Baseline | 参数扫描（只读运行） | §9 参数 manifest | 每参数影响区域/顶点位移表 | 报告 | 否 | 只读/运行 | 中 | AUTH-06 | 未进顶点者标 NOT IMPLEMENTED |
| 8 | Performance Baseline | `RenderStats` + logcat | TV Box | gen_ms/upload_ms/FPS/P50/P95/P99/bytes；APK SHA | 日志 | 设备状态 | 设备 | 中 | AUTH-05 | 无设备 → 标 UNVERIFIED |
| 9 | CPU/GPU Parity 状态 | 同一 PoseFrame 对照（本阶段只记录状态） | §11 | 状态记录：数值未测 | 报告 | 否 | 只读 | 低 | 无需额外 | 写 `[UNVERIFIED]`，不得 VERIFIED |
| 10 | 异常与失败项清单 | 汇总 1–9 | 上述产出 | FAIL / UNVERIFIED / NOT RUN 全列表 | 报告 | 否 | 只读 | 低 | 无需额外 | 不隐藏、不改写 |

### 12.3 G0 Exit Criteria（全部满足才通过）
1. Git baseline 可复现（记录 HEAD/分支/worktree/SHA）。
2. 相关测试结果可复现（真实输出+环境+退出码）。
3. 历史截图完整且有索引（99 PNG manifest；dv3 标 UNVERIFIED）；**99 张逐张检查未完成不得写成已完成**。
4. Mesh / Topology / Parameter 基线已采集。
5. Performance 数据有实际来源（设备/分辨率/APK SHA；REPORTED 与 MEASURED 分列）。
6. CPU/GPU Parity 状态如实（未测 = UNVERIFIED）。
7. 所有测试失败均有记录；不存在未经解释的基线差异。
8. 硬编码指标清零（任何 0/PASS 必须来自真实计算）。
G0 未过 → 不开始 G1。

### 12.4 设备分级（必须分别记录）
- Host Validation：ctest/CMake（可真实验证）。
- Android GLES3 Validation（TV Box `nunai_max`）：设备可用，Android 8.1/API27，GLES3.0 native context。
- TV Box Validation：历史有 REPORTED 数据；本次 MEASURED 待采集。
- Low-end Android Phone Validation：`UNVERIFIED`（无硬件）；豁免需 `DEC-03` 负责人书面确认。

---

## 13. 视觉及算法验收标准（阈值不臆造）

原则 `[RECOMMENDATION]`：**不凭经验臆造精确阈值**。下列除"合同已有"者外一律 `TBD`；阈值产生办法 = 先用 G0/G2 采集真实基线分布，由负责人结合 `DEC-05` 授权参考形态批准数值；未批准前该指标不得判 PASS。

| # | 指标 | 量化测量方法 | 阈值 | 阈值批准办法 |
|---|---|---|---|---|
| 1 | Thorax Silhouette | 固定 front/side/45 机位，轮廓采样点到授权参考轮廓的有向距离分布（p50/p95/max） | TBD | G2 基线分布 + DEC-05 参考图 |
| 2 | Abdomen Volume | 封闭曲面体积（MeshData 三角体积和）与参考体积的差 | TBD | G0 Volume Baseline |
| 3 | Waist Compression | 腰/胸横截面宽度比随参数变化的曲线；侧/背轮廓一致性 | TBD | G2 参数组基线 |
| 4 | Pelvis Proportion | 髂嵴宽/盆深 landmark 距离比 | TBD | DEC-05 参考形态 |
| 5 | Landmark Error | 对应 landmark 欧氏距离（同一集合、同一单位） | TBD | G0 Landmark Baseline |
| 6 | Reference Surface Distance | 双向 Hausdorff / RMS（§8 E1），记录样本数 | TBD | G2 Layer A 基线 |
| 7 | Normal Deviation | 采样点法线与参考法线夹角分布（p50/p95/max）；smooth/hard seam 分类 | `≤5°`（smooth seam，合同已有 `docs/foundation/03`）；其余 TBD | 合同值复用；其余 G2 基线 |
| 8 | Surface Volume Delta | 参数变化前后体积差与区域限定差 | TBD | G0 Parameter Baseline |
| 9 | Parameter Region Impact | 目标区域顶点位移 >0 且 mustRemainStableRegions 位移在阈值内；非法向影响计数 | TBD | G2 参数组基线 |
| 10 | Topology Stability | 同 ID 跨参数指向同一解剖位置的比例；face 邻接恒定比例；components/boundary/nonmanifold | TBD（components=1、boundary=0 除 crop、nonmanifold=0 为合同目标） | G2 Layer B + 负责人 |

**视觉 Gate** `[FACT]`：负责人保留独立否决权；AI 不得自标 VISUALLY ACCEPTED。任一 P0 轮廓/融合/动作缺陷 → REJECTED，停止，不进 G4。

---

## 14. PoC-A 工程边界与 Acceptance Matrix（关闭 SPEC-04 范围部分）

### 14.1 硬边界 `[FACT]`
- 下界：骨盆/髂嵴上部，可见腰盆过渡。
- 上界：胸廓上缘、锁骨根部、颈基底。
- 覆盖：前胸、侧胸、胸腔侧面、后背、肋弓、上腹、下腹、侧腰、腰背、髂嵴、骨盆前后体积。
- **双侧肩连接锚点**必须存在；**不生成完整上臂**；**不执行肩部 PoC**；不做头发、服装、面部细节。
- **Crop Boundary 必须显式定义**（位置、顶点/边索引映射、封口方式）。
- **不允许独立肩球或圆管掩盖接缝**；**不允许简单 Mesh Append 冒充连续曲面**（`Humanoid.cpp:451-466` 是反例）。
- 一个 Shape Model / Shared Skin Envelope 负责整个表面；多 field 贡献只允许一个最终外表面。
- **本 PoC-A 与历史被 REJECTED 的 P1-A 不是同一个实施任务**：P1-A 是 5 站位 Loft + bridge + Sweep 的固定补丁，已 REJECTED 且仅作失败证据；本 PoC-A 是在 G1/G2 中对 R1/R2/R3 表示法进行同输入对比实验，代码不继承 P1-A 路径。

### 14.2 Acceptance Matrix

| 维度 | GATE-02（G1） | GATE-03（G2 / PoC-A） | GATE-04（G3） | GATE-05（G4） |
|---|---|---|---|---|
| front/side/45 silhouette | 不适用（仅可行性） | 必须（7 视图） | 复核 G2 | 必须 |
| back / scapular contour | 不适用 | 必须 | 复核 | 必须 |
| gray Lambert clay | 必须 | 必须 | 复核 | 必须 |
| wireframe / normals | 必须 | 必须 | 复核 | 必须 |
| C0/G1 measured（真实计算） | 不适用 | 必须（禁赋值） | 证据齐备 | 必须 |
| topology manifold audit | 合规 MeshData | 必须（§10 全项） | 证据齐备 | 必须 |
| stable ID / connectivity | 记录状态 | 必须评估 | 证据齐备 | 必须 |
| semantic weight heatmap | 记录状态 | torso regions | 复核 | 全部 |
| neutral pose | 不适用 | 必须 | 复核 | 必须 |
| arm raise / forward | 不要求 | 不要求 | 不要求 | 必须 |
| CPU/GPU parity | 不适用 | 状态记录 `[UNVERIFIED]` | 不得据此批准 | 必须实测 |
| Android frame stats | 记录 | 记录 | 复核 | 必须实测 |
| Visual Gate approval | 不适用 | 工程/视觉分离 | 负责人签字 | 负责人 |

---

## 15. 测试与开发流程 `[RECOMMENDATION]`

流程：`grill-with-docs → to-spec → to-tickets → implement + tdd → 独立 code-review → retro`。本阶段只到 to-spec（candidate）。TDD 顺序：先写失败测试（参数校验、landmark 约束、真实 C0/G1、topology、权重、确定性、极端参数），再写最小实现；不通过改弱断言让测试变绿。报告状态只用 `PASS / FAIL / PARTIAL / NOT VERIFIED`；未执行项 `NOT RUN`，不预写 PASS。

## 16. 文件边界与回滚 `[FACT]` `[RECOMMENDATION]`

- 本阶段新增：`00-implementation-spec.md`（本文件，修订）、`01-spec-consolidation-change-report.md`（变更清单）。
- 未来实施阶段拟新增（**尚未创建**）：approved route 对应的代码/测试（worktree 内），G0/G1/G2 报告与 manifest。
- 不修改：`native/procedural/character/*`（P1-A）、`native/procedural/humanoid/*`（M11/M12）、`native/renderer/RenderThread.*`、JNI/Kotlin 对外签名、任何冻结合同/ADR/project-status、3 份 root v1.0 文档、README（工作区已有改动）、untracked 用户文件。
- 回滚：新增能力走独立 previewMode 通道；失败时 `previewMode_` 回 0/1/2，历史路径不受影响。
- 无 commit / push / merge / rebase / reset / clean / stash。

---

## 17. 状态声明与五个必答问题

**状态声明** `[FACT]`：本 SPEC 为 **DRAFT / FORMAL CANDIDATE / NOT RUN**，不含任何 PASS 声明。G0/G1/G2 代码、测试、截图、报告均 `NOT VERIFIED` / `NOT RUN`。GATE-04 未签字。下一步等负责人审核。

1. **九项 SPEC 缺陷是否全部关闭？** 文档层全部关闭（§5）；其中 SPEC-03 归属、SPEC-04 R2/R3 实现、SPEC-05 阈值、SPEC-06 命令授权、SPEC-07 参数处理、SPEC-09 数值实测仍 OPEN —— 关闭 ≠ 批准。
2. **是否仍存在相互冲突的 Gate 或 Decision？** 无未解决编号冲突：旧 A1–A12 / 附录 A1–A11 已映射到 DEC/AUTH/GATE（§7.5）。Gate 链唯一且线性（§6）。残留依赖：`AUTH-02`（工作区归属）影响 G0 baseline；`DEC-05`（参考形态）影响 G2/G3 视觉判定。
3. **是否达到负责人批准条件？** **尚未**。本文件是正式候选，不是已批准规范；需负责人逐项签 §7.1 DEC-01..07、§7.3 AUTH-01..07 并确认 §6 Gate 冻结。
4. **哪些事项仍需负责人亲自决定？** DEC-01..07 全部；核心：DEC-05 目标形态与视觉验收数据（没有它 AI 只能做数值对照，不能宣布视觉合格）、DEC-02 范围、DEC-03 设备豁免、DEC-04 ADR-011、DEC-06 参数处理、DEC-07 遗留冻结；以及 GATE-04 唯一 approved route 的最终签字。
5. **下一阶段 G0 需要什么授权？** `AUTH-01`（worktree，基于重新核实的 HEAD）、`AUTH-02`（工作区归属）、`AUTH-03`（Host Tests）、`AUTH-04`（Android Build）、`AUTH-05`（TV Box 安装/采集）、`AUTH-06`（其他命令逐条列明）、`AUTH-07`（G1/G2 实现，仅 G0 通过后）。

**最终停止条件** `[FACT]`：本次仅修订文档；未执行 G0 / Host Tests / Android Build / APK 安装；未开始 R2/R3 或 PoC-A 源码开发；未 commit/push；未修改源码；SPEC 保持 DRAFT。等待负责人审核。
