# PAE AI Workflow — 通用启动入口

任何 AI Coding Agent（WorkBuddy / Qoder / Codex / Claude Code / Cursor / Copilot 等）进入 PAE 仓库后，先读本文件，再按下列顺序工作。本文件只引导，不重复完整技术规范。

## PAE 是什么

ProceduralAvatarEngine：C++17 / Android NDK / OpenGL ES 3.0 程序化人体生成与实时控制系统。目标：由 PAE 自有代码按人体语义参数生成完整自然半写实成人，自动骨骼/权重/变形/动画，Android GLES3 实时渲染。运行时不依赖在线 AI / Blender / MakeHuman / MPFB2 / 外部完整人体模型。

## 第一步：读权威文件

1. `AGENT.md`（根目录，Round Acceptance Contract 权威，**不可覆盖/重写/削弱**）。
2. `.agents/skills/pae-project-development/SKILL.md`（核心工作流技能）。
3. `docs/architecture-decisions/ADR-013-round-acceptance-contract.md`、`ADR-014-offline-compiled-human-base-topology.md`。
4. `docs/roadmap/PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md`（M1–M8 唯一权威总计划）、`PAE-HUMAN-GENERATION-STATUS.md`（动态状态）。
5. 相关架构契约与阶段计划/报告/证据。

## 第二步：核对当前代码（每轮强制）

确认 repo root / branch / HEAD / remote HEAD / ahead-behind / tracked / untracked / ignored / 其他 Agent 活动 / 任务白名单。历史 SHA 只是参考点，不假设最新。状态板落后代码→记录冲突，不静默改写路线图/ADR。

## 第三步：按工程生命周期执行

EL-01 Intake → EL-02 Analysis → EL-03 Specification → EL-04 Decomposition → EL-05 Project Planning → EL-06 Phase Planning → EL-07 WP Design → EL-08 Task Breakdown → EL-09 逐点 Implementation → EL-10 Testing & Evidence → EL-11 Integration → EL-12 Gate → EL-13 Release & Handoff → EL-14 Replanning。

不得从需求直跳改代码。按 Task 逐点实现、每点验证、单变量、不降测试标准。详见 SKILL.md 与 references/lifecycle-and-stage-gates.md。

## 第四步：授权与安全

默认 READ_ONLY。建议≠授权；写计划≠代码授权；M1–M8≠各阶段已授权。冻结：LegacyRigV1 / BoneId 0–21 / UBO[22] / 64B packing / 旧 JNI / mode-3 / mode-4 静态默认 / PAHT v3 语义。M4 Android 须 M3 Gate + 授权。Git 未授权禁 reset/clean/merge/rebase/force/自动 commit/push。详见 references/authorization-and-safety.md。

## 第五步：证据与负结果

本轮真实证据，固定可比条件，翻面率，区分穿透/折叠/体积损失，负结果保存。查 references/known-failures-and-anti-patterns.md 不重复失败。技术 PASS≠视觉 PASS；人工 Gate 不代签。

## 禁止

不覆盖 AGENT.md / 既有 Skill；不创建平行路线图；不静默改 ADR；不重复已失败实验；不未授权改 Android 生产路径；不把用户未跟踪文件当垃圾。