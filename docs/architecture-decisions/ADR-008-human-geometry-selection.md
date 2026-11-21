# ADR-008 人体几何生成选型（已定案）

- 状态：**Accepted**（2026-11-21，M11-B PoC 数据定案）
- 关联：M11（docs/phases/M11-anatomy-visual-rebuild/03-technical-design.md §1.4）

## PoC 数据（host 实测，肩部+上臂样本，6/6 ctest 通过）

| 方法 | 三角形 | 生成耗时 | 接缝质量 |
|---|---|---|---|
| A 现状：Loft+Sweep+关节球 | 316 | 0.03ms | 臂端环中心距躯干面 **0.171m**（需 1.22× 关节球遮盖 → 凸起疙瘩） |
| B 扫掠起点嵌入躯干 | 56 | 0.00ms | 无缝（接缝藏于躯干体积内），但截面无法独立塑形 |
| C 局部 SDF 融合（0.8×0.7×0.7m @28³，smooth-union k=0.12） | 2350 | 0.64ms | **单连续曲面**，等参误差 max 0.010m，k 可控混合 |

数据来源：`tests/unit/fusion_tests.cpp::poc_shoulder_fusion_comparison`（stderr POC-SUMMARY 输出）。

## 决定

**Parametric Surface + Spline Sweep 为主体 + SDF 融合补丁仅用于高价值关节区**：
1. 四肢/躯干/颈 = SplineSweep/SplineLoft（拓扑规整、LBS 权重友好、每部位独立截面参数）；
2. 肩部（三角肌-胸大肌融合带）等视觉最关键关节用 SDF 补丁（2350 tris/patch）；
3. 其余关节（肘/膝/髋）用扫掠嵌入法（0 成本无缝）；
4. 关节球仅在扫掠端帽不适用时兜底（半径贴近局部扫掠半径，overshoot ≤10%）。

## 理由

- 全身 SDF 会产生非规则拓扑与权重灾难点（§5.1 警告），且全身融合估计 >14k 三角形（超 Spec §12.1 预算）；
- B 法零成本无缝但截面控制不足；
- C 法表面质量最好但需限制用量；
- 混合方案兼得，符合 Spec §5.2"参数化曲面+样条扫掠+局部 SDF 融合"的优先建议。

## 影响

- `generateSplineSweep/generateSplineLoft`（新增）成为 M11-C 的主体 API；旧 API 保留兼容；
- SDF 补丁顶点权重按区域回查赋值；
- 回滚：各部位生成器独立，可按部位回退。
