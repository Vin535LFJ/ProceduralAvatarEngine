# VR-1B / 05 — M4 Android Dynamic Human Runtime Plan

Status: `M4_ANDROID = NOT_AUTHORIZED`（M3 Host 通过后单独授权）· Date: 2026-11-28
Authority: [`MASTER-PLAN`](../../roadmap/PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md) §7。

## Goal
将 M3 CharacterInstance 接入 Android GLES3 独立动态路径，旧 LegacyRigV1 完全独立保持。

## Scope
Dynamic Rig Palette / Human Rig V2 Runtime / Dynamic Skinning Data / Shape-Joint-Weight Runtime State / CPU-GPU Skinning / RenderMesh Upload / Cache-Lifecycle / APK Asset Loading / Device Capability Detection。

## Dependencies
M3 Host Gate 通过 + 正式授权白名单。

## Allowed Paths（授权后生效，当前 NOT_AUTHORIZED）
新增独立动态人体路径（新 JNI 加法接口、新 RenderThread mode、新顶点布局）。**白名单须正式批准。**
## Forbidden Paths（冻结，全程）
LegacyRigV1 / BoneId 0–21 / UBO[22] / 64B Vertex Packing / 旧 JNI ABI / mode-3 Canonical / mode-4 静态默认行为。PAHT v3 字段语义不得静默改变（新格式走版本升级 + 读旧策略）。

## Input / Output Contract
输入：CharacterInstance（契约）+ 设备能力。输出：真机动态渲染 + CPU/GPU parity 数据。

## 实施内容
- 独立版本化动态数据格式；PAHT v3 兼容。
- GPU 方案**实测目标设备能力**（GLES3.0 UBO 容量、palette 上限、顶点属性布局）；**不得假设 102 骨 palette 全设备可用**。
- CPU 路径作参考实现 + 必要回退。
- 动态骨骼数量按设备能力 LOD。

## Tests / Acceptance（M4 Gate）
静态/动态人体一致；CPU/GPU parity；肩臂髋膝动作；动态骨骼数；多设备能力检查；Surface recreate；APK 独立运行；内存/帧性能。真机证据（非 Host）。
## Evidence
真机截图/日志、parity 数值、设备能力表、性能/内存、生命周期测试。
## Stop Conditions
设备 UBO/palette 不足（需 CPU 回退或 LOD）；必须改冻结 ABI；parity 不达标。
## Owner Sign-off
负责人真机验收 + 多设备审查后签署。

## 未解决
目标设备 GLES3 UBO 实测；64B 复用与否；动态顶点布局；多设备覆盖范围。