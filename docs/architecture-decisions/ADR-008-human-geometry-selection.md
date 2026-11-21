# ADR-008 人体几何生成选型（PoC 待定案）

- 状态：**Proposed → M11-B PoC 后定案**（2026-11-21）
- 关联：M11（docs/phases/M11-anatomy-visual-rebuild/03-technical-design.md §1.4）

## 背景

现有人体 = 独立椭球/圆管/圆角盒 + 关节球拼装；Sweep 无样条（1 点=1 环）、截面仅正圆、法线无锥度倾斜；Loft 无旋转。合并 MeshData 不等于表面融合。

## 候选

A. 现状 + 关节球（基线）
B. Spline Sweep 起点嵌入躯干（中心线上探 + 锥度融合，无额外体素）
C. 局部 SDF 融合补丁（躯干椭球 ∪ 肢体胶囊，smooth-min，24³ marching cubes）

## 决定（暂定）

**Parametric Surface + Spline Sweep + Local SDF Fusion 混合**：全身以样条扫掠/放掠为主（拓扑规整、LBS 权重友好），肩/髋/颈融合区用 SDF 补丁；若 PoC 显示 SDF 补丁三角形数/耗时/权重质量不达标，则回退方案 B 并记录。

## PoC 度量（M11-B 执行）

接缝法线偏差均值、三角形数、生成耗时（host steady_clock）、蒙皮后视觉（设备截图对比）。

## 影响

- 兼容：`generateHumanoid` 签名与测试契约不变；顶点格式不变（64B）；
- 回滚：各部位生成器独立，可按部位回退。
