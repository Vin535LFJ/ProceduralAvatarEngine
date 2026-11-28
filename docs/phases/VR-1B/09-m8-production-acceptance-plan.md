# VR-1B / 09 — M8 Production Quality & Final Acceptance Plan

Status: `M8_PRODUCTION = PLANNED` · Date: 2026-11-28
Authority: [`MASTER-PLAN`](../../roadmap/PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md) §11。

## Goal
用固定版本的模板/参数 Profile/Rig/Pose/设备/相机形成完整验收矩阵，负责人人眼最终签署自然半写实成人。

## Scope
最终验收矩阵 + 性能/内存/稳定性 + APK 独立分发 + 人眼视觉 Gate。

## Dependencies
M1–M7 全部 Gate 通过。

## 验收矩阵（至少覆盖）
- 多种体型 × 多种成人形态 Profile。
- 正面/侧面/背面/45°。
- Shoulder/Elbow/Hip/Knee/Wrist/Finger 动作。
- Walk/Turn/Reach/Squat。
- Clay/Wireframe/Normal/PBR。
- Mesh Quality / Skinning Quality。
- Android CPU/GPU × 多设备。
- 内存 / 冷启动 / 稳态性能 / Surface 生命周期 / APK 独立分发。

## Input / Output Contract
输入：固定版本全套资产 + 设备矩阵。输出：验收矩阵报告 + 负责人签署记录。

## Tests / Acceptance（M8 Gate）
矩阵全项达标 + **负责人人眼最终视觉签署**。**技术指标通过 ≠ 视觉通过；不得自动等同。**
## Evidence
固定版本哈希清单、矩阵结果、多设备性能/内存、人眼签署记录。
## Stop Conditions
任一关键项失败；视觉未获负责人签署。
## Owner Sign-off
负责人最终签署（AI 仅转录，不代签）。

## 未解决
多设备覆盖清单；最终视觉标准的具体人眼判据。