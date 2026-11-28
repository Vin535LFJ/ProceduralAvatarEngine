# VR-1B / 07 — M6 Anatomical Visual Refinement Plan

Status: `M6_ANATOMY = PLANNED` · Date: 2026-11-28
Authority: [`MASTER-PLAN`](../../roadmap/PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md) §9。

## Goal
把已有合理拓扑与变形能力的基础人体，提升为自然半写实成人。**优先 PAE 自有 Shape Basis / 控制点 / 结构化拓扑 / 局部 Corrective。不允许退回全身 SDF/Primitive 拼接作为最终主要人体表达。**

## Scope（分区域）
Thorax/Ribcage · Pectoral · Abdomen/Oblique · Back/Scapula · Trapezius · Deltoid · Biceps/Triceps · Pelvis/Gluteal · Thigh/Calf · Face/Eyelids/Nose/Lips · Hand/Palm/Fingers · Foot/Arch/Toes。

## Dependencies
M2 变形质量基线；M3 实例。**手指/肩部极端姿态硬限制可能使 M6 优先级前移**（MASTER-PLAN §10.1）。

## Allowed / Forbidden Paths
授权后白名单（PAE shape basis / corrective 模块 + 必要的模板拓扑细化，需评估对 PAHT 的影响——走版本升级）。不得退回 SDF 拼接主路线；不得用未审核外部资产。

## Input / Output Contract
输入：基础人体 + 区域细化参数。输出：细化后网格（Bind + Pose 双态）+ 区域质量报告。

## 实施重点
- 手指环线密度（握拳翻面根因之一）；肩腋窝/肘窝/腹股沟/腘窝关节区形态。
- 肌肉群（三角肌前中后束、胸大肌、背阔肌等）结构化表达。
- 面部/眼睑/鼻/唇；足弓/脚趾。

## Tests / Acceptance（M6 Gate）
**Bind Pose 与动作 Pose 双态验收**，不能只优化静态正面肌肉线条。区域网格质量 + 变形后自然度。
## Evidence
Bind+Pose 双态前后对比、多视角、区域指标。
## Stop Conditions
细化破坏拓扑流形/变形；必须改 PAHT v3 语义（走版本升级授权）。
## Owner Sign-off
负责人双态人眼验收。

## 未解决
手指几何细化 vs corrective 的取舍；细化对模板版本/Android 加载的影响。