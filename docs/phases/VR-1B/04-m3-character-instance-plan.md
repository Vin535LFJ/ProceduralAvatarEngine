# VR-1B / 04 — M3 Integrated Character Instance Plan

Status: `M3_INTEGRATION = NOT_STARTED`（需 M1+M2 Gate 通过 + 新授权）· Date: 2026-11-28
Authority: [`MASTER-PLAN`](../../roadmap/PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md) §6 · 接口：[`CharacterInstance 契约`](../../architecture/human-character-instance/01-character-instance-contract.md) §4。

## Goal
把形态、关节、骨骼、权重、姿势、网格整合为完整一致的 `CharacterInstance`，证明换形态后关节/权重同步更新。

## Scope
CharacterInstanceBuilder / CharacterInstance / CharacterInstanceQualityReport；缓存失效；权重复用判定；稳定 ID 映射。Host-only（M3 不涉及 Android）。

## Dependencies
M1 ResolvedShapeState（Gate 通过）+ M2 ResolvedJointSet/RigDefinition/SkinWeightSet/CPUDeformedMesh（Gate 通过）+ 集成契约（VR-1B/03）。

## Allowed Paths
`tools/human-template-poc/host/src/m3_*`（新）、host CMakeLists、`docs/phases/VR-1B/04-*`、evidence。**需正式授权白名单后实施。**
## Forbidden Paths
Android 生产路径、冻结 ABI、LegacyRigV1、PAHT v3 语义。

## Input / Output Contract
输入：ShapeParameterVector/测量目标 + PoseState + HumanTemplateIdentity。
输出：CharacterInstance（契约 §1 全部状态 + 缓存）+ QualityReport。

## 实施内容
1. Shape Solver 输出 → 2. JointResolver → 3. RigBinding → 4. WeightGeneration/ReuseDecision → 5. PoseState → 6. CPU LBS → 7. RenderMesh → 8. CacheInvalidation → 9. StableIDMapping。
- 权重复用规则：定义误差阈值（关节位移/骨长变化超过阈值→重算），不得因拓扑相同默认复用。
- 缓存键绑定六项版本/哈希；版本不符→失效。

## Tests
≥9 体型实例化；≥3 体型同动作；换形态后关节/权重不停留旧人体（断言）；缓存失效正确；稳定 ID 映射一致；确定性。
## Evidence
多体型同动作对比图、关节/权重同步断言日志、缓存命中/失效日志、QualityReport。
## Acceptance（M3 Gate）
同步性证明 + 缓存正确 + 多体型动作一致。**Host 通过 ≠ Android 通过。**
## Stop Conditions
关节/权重停留旧人体；缓存错误共享；必须改 Android 生产。
## Owner Sign-off
负责人审查同步证据后授权 M4。

## 未解决
权重复用误差阈值需实测标定；RenderMesh 动态布局留待 M4。