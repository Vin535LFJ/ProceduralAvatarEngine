# VR-1B / 06 — M5 Procedural Animation & IK Plan

Status: `M5_ANIMATION = NOT_AUTHORIZED`（M4 动态运行时稳定后）· Date: 2026-11-28
Authority: [`MASTER-PLAN`](../../roadmap/PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md) §8。

## Goal
动画全部由 PAE 自己的数学与骨骼控制生成，不导入 FBX 作运行时依赖。

## Scope
M5-A Pose Controller / M5-B IK / M5-C Procedural Motion / M5-D Motion Quality。

## Dependencies
M4 动态骨骼运行时稳定 + 授权。

## Allowed / Forbidden Paths
授权后白名单（动画控制器/IK 模块，Host 研究 + Android 动态路径）。冻结边界同 M4。不得导入 FBX 作正式运行时动画依赖。

## Input / Output Contract
输入：PoseState 目标/IK 目标（手/足位置、pole vector）。输出：可重复的骨骼姿态序列 + 运动质量证据。

## 实施顺序
- M5-A：Bone Local Rotation / Joint Constraints / Root Motion / Pose Blending。
- M5-B：Two-bone IK / Elbow-Knee Pole Vector / Foot Target / Hand Target。
- M5-C：Idle / Walk / Run / Turn / Reach / Squat / Foot Placement / Body Sway / Upper-body Actions。
- M5-D：Joint Limits / Foot Sliding / Contact / Balance / Root Motion / Pose Interpolation。

## Tests / Acceptance（M5 Gate）
每种动作可重复输入输出证据；关节限位/足滑/接触/平衡/根运动/插值达标。
## Evidence
动作序列图/视频帧、IK 收敛日志、足滑/接触量化、性能。
## Stop Conditions
关节超限/足滑严重/平衡失败；必须改冻结 ABI。
## Owner Sign-off
负责人审查动作自然度后签署。

## 未解决
程序化 walk/run 的平衡与足放置策略；动作自然度人眼标准。