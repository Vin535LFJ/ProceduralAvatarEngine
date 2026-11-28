# VR-1B / 03 — M1 ↔ M2 Integration Contract & Parallel Protocol

Status: `READY_FOR_REVIEW` · Date: 2026-11-28
Authority: [`MASTER-PLAN`](../../roadmap/PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md) §5 · 数据契约权威：[`CharacterInstance 契约`](../../architecture/human-character-instance/01-character-instance-contract.md)（本文件不重复定义数据类型，只规定并行协作协议）。

## Goal
M1（形态求解）与 M2（骨骼蒙皮）Host 并行开发，共享唯一接口，在 M3 汇合前统一数据契约。

## 并行规则
- M1、M2 代码各自在既有 Host 白名单内并行（M1: wp3_r2_*；M2: wp4_*）。
- **接口变更只能改权威契约文件**（human-character-instance/01），不得在各自代码里私自定义分叉版本。
- 并行 Agent 仅在隔离 Host 范围；生产写入单一权威写入者。

## 汇合点（M3 前置）
1. M1 输出 `ResolvedShapeState` 必须含 M2 JointResolver 所需的 deformedFull（含 helper/joint）。
2. M2 JointResolver 必须在 M1 变形后表面运行（严禁基础人体固定关节）。
3. 权重复用判定规则由 M3 定义并验证；当前默认形态改变→权重重生成。
4. 缓存键统一：`hash(baseMesh)+hash(shapeVector)+rigVersion+weightVersion+profileHash`。

## 同步测试（M3 验收预演）
≥9 体型（Baseline/BroadShoulder/BroadRibcage/NarrowWaist/WideHip/Tall/Short/ComboAthletic/ComboPear）；≥3 体型执行相同动作；证明换形态后关节/权重不停留旧人体。

## 版本兼容
所有交换数据携带契约 §2 六项版本/哈希。不同 Rig/Weight 版本不得共享缓存。

## Acceptance
M1+M2 各自 Gate 通过 + 接口一致性测试通过 → 授权 M3。
## Owner Sign-off
负责人审查接口一致性与同步测试后授权 M3。