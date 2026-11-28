---
name: pae-project-development
name_en: PAE Project Development
name_zh: PAE 项目开发工作流
description: ProceduralAvatarEngine (PAE) project-level AI engineering workflow. Enforces the EL-01..EL-14 lifecycle (requirement -> spec -> decomposition -> planning -> work packages -> tasks -> incremental implementation -> evidence -> integration -> gate -> handoff), authorization/frozen-path safety, multi-agent collaboration, and the known-failures library. Use when working on the PAE repo (ProceduralAvatarEngine, poc-sm/shape-model-v2 branch), or when the user mentions PAE human generation, rig/skinning, shape parameterization, PAHT template, Android human runtime, or asks to plan/implement/review/commit any PAE task.
description_en: ProceduralAvatarEngine (PAE) project-level AI engineering workflow. Enforces the EL-01..EL-14 lifecycle, authorization/frozen-path safety, multi-agent collaboration, and the known-failures library. Use when working on the PAE repo or any PAE human-generation / rig / skinning / shape / PAHT / Android task.
description_zh: ProceduralAvatarEngine（PAE）项目级 AI 工程工作流。强制执行 EL-01..EL-14 生命周期（需求→规格→分解→计划→工作包→任务→逐点实现→证据→集成→Gate→交接）、授权与冻结路径安全、多 Agent 协作、负结果库。在 PAE 仓库（ProceduralAvatarEngine，poc-sm/shape-model-v2 分支）工作，或用户提到 PAE 人体生成/骨骼蒙皮/形态参数化/PAHT 模板/Android 人体运行时，或要求规划/实现/审查/提交任何 PAE 任务时使用。
argument-hint: Describe the PAE task (feature / bug / research / review / commit) or attach the task spec
argument-hint-en: Describe the PAE task (feature / bug / research / review / commit) or attach the task spec
argument-hint-zh: 描述 PAE 任务（新功能/Bug/研究/审查/提交）或附上任务书
user-invocable: true
---

# PAE Project Development

PAE 是 C++17 / Android NDK / OpenGL ES 3.0 的程序化人体生成与实时控制系统。目标：由 PAE 自有代码按人体语义参数生成完整自然半写实成人，自动构建骨骼/权重/变形/动画，Android GLES3 实时渲染。运行时不依赖在线 AI、Blender、MakeHuman、MPFB2 或外部完整人体模型。

本技能规定**流程与行为**，不复制项目技术权威。所有技术事实以仓库权威文件为准（见下）。

## 0. 激活后第一步（强制）

1. **Repository Preflight**（只读）：确认 repo root / branch / HEAD / remote HEAD / ahead-behind / tracked / untracked / ignored / 其他 Agent 活动 / 任务白名单。不得假设工作区干净，不得把用户未跟踪文件当垃圾。
2. **读取权威文件**（按需，渐进）：
   - `AGENT.md`（根目录，Round Acceptance Contract 权威，**绝对不可覆盖/重写/削弱**）。
   - `docs/architecture-decisions/ADR-013-round-acceptance-contract.md`、`ADR-014-offline-compiled-human-base-topology.md`。
   - `docs/roadmap/PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md`（M1–M8 唯一权威总计划）。
   - `docs/roadmap/PAE-HUMAN-GENERATION-STATUS.md`（动态状态板）。
   - `docs/architecture/human-character-instance/01-character-instance-contract.md`、`human-rig-v2/01-rig-v2-architecture.md`、`target-architecture.md`、`module-contracts.md`。
   - 相关 `docs/phases/VR-1B/` 与 `docs/phases/P1-visual-recovery/` 计划/报告/证据。
3. **核对当前事实**：最新提交、最新报告、状态板、测试结果。历史 SHA 只是参考点，每轮重新核对。状态板落后代码时记录冲突，不静默改写正式路线图/ADR。
4. **判定授权级别**（默认 READ_ONLY）与任务所处生命周期阶段。

权威关系：AGENT.md > ACCEPTED ADR > MASTER-PLAN > STATUS > Architecture Contracts > Phase Plans > WP Specs > Source Code > Test/Evidence。冲突时记录版本，不静默择一覆盖。

## 1. 工程生命周期 EL-01..EL-14（强制）

```
EL-01 Requirement Intake      接收需求，记录 ID/意图/约束/未知/初始授权
EL-02 Requirement Analysis    调查代码、复现问题、Before 基线、区分现象与根因、HYPOTHESIS
EL-03 Requirement Specification  可验证规格 + 唯一 REQ-ID + 验收准则 + 追踪链
EL-04 Project Decomposition   Project→Phase→Milestone→WP→Task + 依赖图 + 风险登记
EL-05 Project-Level Planning  路线/依赖/边界/里程碑（沿用 MASTER-PLAN，不另建平行路线）
EL-06 Phase Planning          进入阶段前复核详细计划 + Phase Readiness Review
EL-07 Work Package Design     WP 输入/输出契约/白名单/验收/停止条件/授权
EL-08 Task Breakdown          有序 Task + 前置/验证命令/通过准则
EL-09 Incremental Implementation  按 Task 逐点实现，每点验证，不一次性大改
EL-10 Testing & Evidence      匹配目标的验证 + 本轮真实证据 + 固定可比条件
EL-11 Integration             四层集成（模块/接口/子系统/阶段）
EL-12 Stage Acceptance        独立 Gate；AI 给评估，不代签人工 Gate
EL-13 Release & Handoff       交付包 + 状态更新 + 交接（新 Agent 仅靠仓库即可继续）
EL-14 Maintenance & Replanning  据实调整，记录原计划/触发/证据/影响，不静默覆盖
```

默认不得从 EL-01 直接跳到改代码。小任务可合并流程但须记录简化依据，保留授权/基线/验证。详见 [references/lifecycle-and-stage-gates.md](references/lifecycle-and-stage-gates.md)。

## 2. 逐点实现纪律（EL-09，最重要）

每个 Task：读源码 → 核对契约 → 核对前置 Task → 确认白名单 → 明确最小修改 → 修改 → 跑指定测试/探针 → 查 Git Diff → 存真实日志 → 更新状态 → 决定下一步。

失败：存证据 → 停依赖任务 → 析根因 → 修复/返回设计/BLOCKED → 保留原始失败信息。**不得降低测试标准使其通过。**

**不得同时改多个独立变量**（旋转轴 / Joint Position / Weight / 网格拓扑 / DQS 必须逐项验证，否则无法归因）。

## 3. 授权与冻结边界（安全）

授权级别：READ_ONLY / RESEARCH_ONLY / DESIGN_ONLY / HOST_IMPLEMENTATION / ANDROID_IMPLEMENTATION / INTEGRATION / GOVERNANCE。默认 READ_ONLY。**建议≠授权；写计划≠获代码授权；MASTER-PLAN M1–M8≠各阶段已授权。**

未经负责人明确批准不得改：LegacyRigV1 / BoneId 0–21 / UBO[22] / 64B Vertex Packing / 旧 JNI ABI / mode-3 Canonical / mode-4 静态默认 / PAHT v3 字段语义。M4 Android 动态须 M3 Gate 通过 + 正式授权。

Git 未授权禁止：reset / clean / 覆盖工作区 / 自动 stash / 删证据 / 删分支 / merge / rebase / force push / 自动 commit / 自动 push。提交须核对暂存/路径/属性/远端，按任务分组，推后 ls-remote 核实。详见 [references/authorization-and-safety.md](references/authorization-and-safety.md)。

## 4. 架构摘要（当前主路线，ADR-014 后）

CC0 Base Topology/Targets（逐文件许可证确认）→ 离线编译 → PAE PAHT 模板 → Shape Parameterization → JointResolver → HumanRigV2 → WeightGenerator → Skinning → RenderMesh → Android Runtime → Animation/IK → Anatomy/Appearance → Final Acceptance。

`default.mhskel` / `default_weights.mhw` 仍 PENDING_CONFIRMATION，不得作分发/运行时依赖。早期 Primitive/SDF/Sweep/Bridge 路线保留为历史对照，**不自动恢复为主线**。详见 [references/architecture-map.md](references/architecture-map.md)、[references/project-history.md](references/project-history.md)。

## 5. 任务类型路由（渐进加载）

识别任务类型 → 加载对应 reference + workflow → 读相关权威文件 → 按流程执行：

| 任务类型 | Workflow | 关键 References |
|---|---|---|
| 新功能 | [workflows/new-feature-workflow.md](workflows/new-feature-workflow.md) | requirements-management, lifecycle |
| Bug/质量修复 | [workflows/bug-fix-workflow.md](workflows/bug-fix-workflow.md) | known-failures, test-and-evidence |
| 技术研究 | [workflows/research-workflow.md](workflows/research-workflow.md) | known-failures, geometry-and-skinning |
| 几何/解剖/视觉 | [workflows/geometry-visual-workflow.md](workflows/geometry-visual-workflow.md) | geometry-and-skinning, test-and-evidence |
| 骨骼/蒙皮 | [workflows/rig-skinning-workflow.md](workflows/rig-skinning-workflow.md) | geometry-and-skinning, known-failures |
| Android | [workflows/android-workflow.md](workflows/android-workflow.md) | authorization-and-safety, architecture-map |
| 集成 | [workflows/integration-workflow.md](workflows/integration-workflow.md) | agent-collaboration, lifecycle |
| 发布/交接 | [workflows/release-workflow.md](workflows/release-workflow.md) | agent-collaboration, lifecycle |

模板见 [templates/](templates/)（requirement-intake、requirements-specification、project-breakdown、phase-development-plan、work-package-specification、task-checklist、experiment-plan、implementation-report、integration-report、review-report、gate-assessment、agent-handoff、replanning-record、status-reconciliation 等）。

## 6. 证据与质量（EL-10）

区分：编译成功≠功能正确；单测过≠复杂姿态正确；Bind 正确≠Extreme 正确；权重归一化≠解剖正确；Host 过≠Android 过；拓扑连通≠视觉自然；技术 PASS≠视觉 PASS。

几何/人体/骨骼/视觉任务必须生成本轮真实证据（固定视角 Clay/Wireframe/Normal/Heatmap/Closeup/CSV/日志/Before-After/Diff），保持相同网格/参数/姿态/相机/Mask/指标口径。不得用旧图冒充、不得移动 Mask 掩盖问题。**负结果必须保存。** 详见 [references/test-and-evidence-protocol.md](references/test-and-evidence-protocol.md)。

## 7. 负结果库（不重复失败）

历史失败与防范规则见 [references/known-failures-and-anti-patterns.md](references/known-failures-and-anti-patterns.md)。关键：MeshData 合并≠几何融合；全身 SDF≠自然人体；身高 Pivot 错误；affected-only≠whole-region；绝对退化阈值的尺度假象；肩偏移移动 Mask；Core20 无手指却评握拳；统一细分不改善握拳；细分权重降纯度；Bind 过≠Extreme 过；单测过≠视觉过；状态板落后代码。未证实结论标 HYPOTHESIS，不标 CONFIRMED。

## 8. 多 Agent 协作

并行前定义：WP 分工/输入输出契约/共享类型/文件白名单/禁改路径/依赖顺序/写入所有权/集成负责人/交接标准。同一生产文件同一时间唯一权威写入者。交接按 [templates/agent-handoff.md](templates/agent-handoff.md)。独立审查须核对被审 SHA/分支/任务书版本/Diff/源码/日志/证据/ADR/Gate，不审旧快照套新版本。详见 [references/agent-collaboration.md](references/agent-collaboration.md)。

## 9. 动态状态

项目当前状态见 [references/current-project-state.md](references/current-project-state.md)（注明日期/分支/SHA/证据/是否核实），但它**不替代正式状态板**，每轮仍重新读仓库。快照过期须提醒复核。路线见 [references/development-roadmap.md](references/development-roadmap.md)（链接回 MASTER-PLAN，不另建权威）。

## 10. 跨 AI 入口

不支持自动加载 Skill 的 AI：读根目录 `PAE_AI_WORKFLOW.md`（引导）与 `AGENTS.md`（精简入口），再读本 SKILL.md。`AGENT.md` 是已批准 Round Acceptance Contract，不可动。既有 `.workbuddy-ai/skills/round-acceptance/`（若存在）保留、引用、不覆盖。