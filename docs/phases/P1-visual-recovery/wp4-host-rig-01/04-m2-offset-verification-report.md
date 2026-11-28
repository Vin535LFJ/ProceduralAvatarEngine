# M2 Shoulder Offset — Adoption Verification Report (CORRECTED CONCLUSION)

Date: 2026-11-28 · Branch `poc-sm/shape-model-v2` @ `47d462d`（本地=远端）
Authority: [`MASTER-PLAN`](../../../roadmap/PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md) · [`rig-v2 架构`](../../../architecture/human-rig-v2/01-rig-v2-architecture.md)
Scope: WP4 Host 白名单。未触 Android、未引入 DQS、未 commit/push。生产路径零 diff。

## 0. 结论（重要更正）

**肩 proximal-10mm 偏移不予默认采纳。** 严格验证（同一皮肤补丁掩码对照新旧 rig）表明它是**权衡而非明确改善**，且先前 03 号报告"边拉伸减半（12.75×→5.87×）"的结论是**测量掩码假象**，现予更正。

- 机制已实现并验证正确：`buildRig(..., anatomicalShoulder)` 可选参数，偏移沿肱骨轴近端 10mm，9 形态下偏移量精确 0.01000m、关节同步、bind 精确。
- 但变形质量净效果不支持默认采纳：**默认恢复 v2.1（helper 质心）**；v2.2（解剖偏移）保留为可选研究配置。

## 1. 假象根源（方法论更正）

先前 03 号报告的偏移扫描中，腋窝区域掩码由 `jointRegions(rig, js)` 生成，**掩码中心随偏移后的关节中心移动**。proximal 偏移把关节中心上移内移，掩码随之上移，**排除了下方被拉伸的皮肤**，于是测得 edgeMax 5.87×（看似改善）。

本轮验证对 legacy 与 adopted **使用同一固定掩码**（均以原始 helper 质心为中心，同一组皮肤顶点），结果反转：

| 外展 | 翻面 旧→新 | 腋窝翻面 旧→新 | 腋窝峰值拉伸 旧→新（固定掩码） |
|---|---|---|---|
| 30° | 8→2 ✓ | 4→1 ✓ | 5.34→6.11 ✗ |
| 60° | 24→14 ✓ | 10→5 ✓ | 9.36→10.83 ✗ |
| 90° | 64→70 ✗ | 18→24 ✗ | **12.75→14.82 ✗** |

物理机制：腋窝皮肤在关节中心**下方**；把旋转中心上移（近端）增大了腋窝皮肤到旋转中心的力臂 → 外展时拉伸**更大**。这与固定掩码实测一致。先前 5.87× 是掩码上移的假象，非真实改善。

## 2. 净评估

- 中等角度（30/60°）：翻面减少（可见伪影改善）。
- 所有角度：腋窝**峰值**边拉伸增大（皮肤撕裂风险上升）。
- 关键 90°：翻面与腋窝翻面均恶化。
- 全局 edgeP99 略降（被非肩部边主导，掩盖局部恶化）。

**综合：非明确改善，90° 关键姿态恶化，不予默认采纳。**

## 3. 9 形态关节同步（机制正确性，`data/adoption-sync.csv`）

9 形态（baseline/broader-shoulder/broader-ribcage/narrower-waist/wider-hip/taller 1.80/shorter 1.60/combo-athletic/combo-pear）全部：关节解析成功、rig 校验通过、bind 恢复 ≤3.69e-08m、0 未绑定、L/R 对称 0.0、**偏移量精确 0.01000m（随形态沿局部肱骨轴自适应）**。机制本身正确可靠，只是变形收益不支持采纳。

## 4. 全姿态回归（`data/pose-regression.csv`）

22 姿态 legacy(v2.1) vs offset(v2.2) 固定掩码对照。非肩部姿态（肘/髋/膝/腕/指/拳/蹲）两者完全一致（偏移只影响肩）。肩部姿态见 §1 表。

## 5. 代码状态

- `wp4_rig.h/.cpp`：新增 `kShoulderProximalOffsetM=0.010`、`kRigVersionAnatomical="pae-rig-v2.2"`、`buildRig(..., bool anatomicalShoulder=false)`。**默认 false（v2.1）**。
- `wp4_adopt_main.cpp`：验证运行器（9 形态同步 + 22 姿态固定掩码对照）。
- `wp4_main.cpp`/`wp4_q2_main.cpp`：显式钉 `false`，保持已提交 v2.1 证据可复现。
- 回归：WP4 16/16、WP3 33/33、ht 65/65、ctest 22/22。零新增警告。

## 6. 证据

`evidence/m2-adoption-verification/`：37 图（重压缩 0.71MB，像素往返校验）+ adoption-sync.csv / pose-regression.csv / adoption-verification.md + 5 日志。
图：shape-*-clay（9 形态）、adopted-*-clay/closeup（22 姿态）、legacy-shoulder_abduction_90-closeup（对照）。

## 7. 对先前文档的更正

- 03 号报告 §2 "proximal-10mm 减半腋窝边拉伸"结论**作废**（掩码假象），以本报告 §1 固定掩码实测为准。
- rig-v2 架构 §4.2 肩关节偏移注记更新为"已研究，权衡，未默认采纳"。

## 8. 下一步

1. 负责人审查本更正与固定掩码对照图。
2. 批准后入库（含对 03 号报告/架构的更正）。
3. 肩部变形质量改进需另寻路径：M6 肩腋窝几何细化（增加环线/结构化拓扑）或局部 corrective（DQS 后续授权）——单纯移动关节中心已证非解。