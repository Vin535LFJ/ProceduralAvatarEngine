# VR-1B / 02 — M2 WP4 Rig V2 / JointResolver / Weight / CPU LBS Implementation Plan

Status: `M2_HOST = TECHNICAL PARTIAL`（WP4-HOST-RIG-01 已实现，极端姿态质量未达标）· Date: 2026-11-28
Authority: [`MASTER-PLAN`](../../roadmap/PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md) §4 · 细化：[`rig-v2 架构`](../../architecture/human-rig-v2/01-rig-v2-architecture.md) · 接口：[`CharacterInstance 契约`](../../architecture/human-character-instance/01-character-instance-contract.md)
代码授权来源：WP4-HOST-RIG-01 白名单（`wp4-host-rig-01/01-wp4-host-rig-plan.md` §1）。

## Goal
PAE 自有可变骨骼定义 + 形态后关节定位 + 权重生成 + CPU 骨骼变形，使人体在姿态下自然变形。

## Scope
M2-A RigDefinition / M2-B JointResolver / M2-C WeightGenerator / M2-D CPU LBS / M2-E Pose Quality。Host-only。

## Dependencies
M0 模板（CC0 joint 辅助组）；接口契约 §1.5–1.9；WP3 形态（并行）。

## Allowed Paths
`tools/human-template-poc/host/src/wp4_*`、host CMakeLists、`docs/phases/VR-1B/02-*`、`docs/architecture/human-rig-v2/**`、对应 evidence。
## Forbidden Paths
app/**、native/renderer|runtime|procedural/human_template/**、PAHT v3、JNI、LegacyRigV1、UBO[22]、64B、mode-3/4。不构建 APK、不部署、不自动 commit/push。

## Input / Output Contract
输入：ResolvedShapeState + HumanRigDefinition + PoseState。
输出：`ResolvedJointSet + RigDefinition + SkinWeightSet + CPUDeformedMesh`（契约 §1.5–1.9）。

## 已实现（WP4-HOST-RIG-01，提交 3c0ad02/340db63/d40e213）
- M2-A：语义表 Full92/Core20，rest/local 坐标架，校验（唯一/无环/无零长）。
- M2-B：CC0 joint 组质心解析 + root 锚点；L/R 对称 0.0；9 形态跟随。
- M2-C：Distance/Heat/Region 对照 + 诊断（跨侧/远绑/平滑/Top-4 丢弃）；Region 主用（0 跨侧/0 远绑）。
- M2-D：Bone Local Transform 合成 + 位置/法线 LBS；Bind 恢复 3.7e-08 m。
- M2-E：22 姿态 + 逐关节区域指标 + 184 图；方向断言 0 失败。

## 待实施（质量改进，需在本白名单内）
1. **M2-B 解剖关节中心**：区分 SurfaceLandmark/BoneJointCenter/RigJoint/GeometryAnchor；肩峰≠盂肱关节中心，需 PAE 自有偏移模型（不得退回 PENDING 文件）。
2. **极端姿态变形**：握拳 1056 / 屈肘120 3138 / 举臂 3112 / 深蹲 1517 翻面。先评估 LBS 权重改进；若证明是网格环线/方法硬限制，记录并提请 M6 几何细化或 DQS/corrective 授权（**本轮暂不引入 DQS/Muscle/Corrective**）。
3. 肩外展腋窝拉伸 12.7× 的权重/关节中心归因。

## Tests
Bind 精确恢复 / Top-4 合法 / 父子变换 / 法线 / 逐关节区域指标 / 多视角图 / 方向断言。回归 WP4 16 + WP3 33 + ht 65 + ctest 22。
## Evidence
逐关节区域 CSV、多视角 clay/wire/normal/heat/overlay、关节近景、性能日志。
## Acceptance（M2 Gate）
中等姿态自然 + 极端姿态质量达标或明确归因到 M6；Bind 精确；权重诊断达标。**Host ≠ Android。**
## Stop Conditions
握拳严重翻面（已触发，停在该项不扩权）；必须改基础网格/native 生产；必须引入 DQS（需新授权）。
## Owner Sign-off
负责人人眼看证据图（尤其握拳/肩外展）后签署。

## 未解决
极端姿态硬限制归属（网格 vs 方法）；肩关节中心解剖偏移；DQS 评估时机。