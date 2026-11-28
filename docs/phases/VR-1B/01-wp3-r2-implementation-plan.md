# VR-1B / 01 — M1 WP3-R2 Human Parameter Solver Implementation Plan

Status: `M1_HOST = DESIGN` · Date: 2026-11-28
Authority: [`MASTER-PLAN`](../../roadmap/PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md) §3 · 接口：[`CharacterInstance 契约`](../../architecture/human-character-instance/01-character-instance-contract.md)
代码授权来源：WP3-HOST-01 / WP3-H-R1 白名单（`tools/human-template-poc/host/**` + 对应 docs）。**不得超出原白名单，不得触 Android。**

## Goal
用户输入人体测量参数 → PAE 求解 Shape Target 组合 → 输出对应人体（`ResolvedShapeState`）。解决 WP3 已暴露的**组合非加性**与 **±1 authored 钳制**。

## Scope
M1-A Forward Model / M1-B Inverse Solver / M1-C Semantic Measurements / M1-D Parameter Profile。Host-only。

## Dependencies
- WP3-HOST-01 测量协议（`wp3_measure`，环分离围度）+ R1 锚点/联动修正（已入库）。
- 18 个模板 target 与源资产逐值一致（已验证）。
- 接口契约 §1.2/1.3/1.4。

## Allowed Paths
`tools/human-template-poc/host/src/wp3_r2_*`（新）、`tools/human-template-poc/host/CMakeLists.txt`、`docs/phases/VR-1B/01-*`、对应 evidence。
## Forbidden Paths
app/**、native/**（生产）、PAHT v3、JNI、LegacyRigV1、UBO[22]、64B、mode-3/4。不构建 APK、不部署、不自动 commit/push。

## Input Contract
`ShapeParameterVector` 或目标测量向量（Height/ShoulderBreadth/RibcageBreadth/ChestDepth/WaistBreadth/WaistCircumference/HipBreadth/HipCircumference）+ `HumanTemplateIdentity`。
## Output Contract
`ResolvedShapeState`：deformedFull/Body、actualMeasurements、alphaEffective（钳制后）、geometryQuality、profileHash。

## 实施内容
### M1-A Forward Model
`F(alpha, stature) -> Measurements + GeometryQuality`。每次从固定 Base Mesh 开始，**不叠加上一次输出**。复用 `applyVariant` + `wp3_measure`。
### M1-B Inverse Solver
候选（按复杂度递增，先简单后非线性）：
1. 多维查找表（实测 α 网格 → 测量，插值反查）。
2. 分段响应曲线（保留 incr/decr 非对称斜率）。
3. 有界非线性最小二乘（Levenberg-Marquardt 或 Trust Region），数值 Jacobian，α∈实测有效范围。
必须处理：参数可辨识性（哪些测量约束哪些 α）、非加性联动（迭代而非线性叠加）、不可达目标（返回最近可达 + 残差报告，不静默钳制假装命中）。
### M1-C Semantic Measurements
统一 8 项测量；区分 PAE 截面口径与 ANSUR 参考；围度环分离。
### M1-D Parameter Profile
可保存可复现 Profile（Baseline/BroadShoulder/BroadRibcage/NarrowWaist/WideHip/Tall/Short/Combined）。每 Profile 记录模板 SHA + Target SHA + 参数向量 + 测量结果。

## Tests
known-solution recovery（给定 α→测量→反求 α）/ holdout combination / unreachable target / multi-target non-additivity / repeat determinism / mesh quality（0 翻面/0 自交）/ shape profile comparison。
## Evidence
求解误差表、holdout 残差、不可达案例、Profile 文件、多体型图、CSV、测试日志。
## Acceptance（M1 Gate）
known-solution 误差 ≤ 约定阈值；holdout 残差报告；不可达显式标注；确定性逐字节；网格质量达标。**Host 通过 ≠ Android 通过。**
## Stop Conditions
求解器无法收敛/不可辨识；必须扩展 α 超出实测范围（需新授权）；必须改 native 生产代码。触发即停，不扩权。
## Owner Sign-off
负责人审查求解误差与 Profile 对比后签署 M1 Gate。

## 未解决
非加性程度需实测量化；ANSUR 口径对齐；±1 范围外需更多 CC0 target（另行授权）。