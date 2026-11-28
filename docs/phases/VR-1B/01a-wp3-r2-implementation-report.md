# VR-1B WP3-R2 (M1) — Human Parameter Solver Implementation Report

Date: 2026-11-28 · Branch `poc-sm/shape-model-v2` @ `d40e213`（本地=远端）
Authority: [`MASTER-PLAN`](../../roadmap/PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md) §3 · 计划：[`01-wp3-r2-implementation-plan.md`](01-wp3-r2-implementation-plan.md) · 接口：[`CharacterInstance 契约`](../../architecture/human-character-instance/01-character-instance-contract.md)
Scope: Host-only（WP3 既有白名单）。未触 Android、未构建 APK、生产路径零 diff。本轮未 commit/push。

## 0. 状态

`M1_HOST = IMPLEMENTED`（forward model + 有界逆求解 + Profile + 全部验收测试通过）。
M1 Gate 技术项通过；**最终视觉/口径对齐仍待负责人**（ANSUR 口径对齐是 WP3-Production 前置）。

## 1. 实现

- **Forward Model** `F(alpha, stature) -> Measurements + GeometryQuality`（`wp3_r2.cpp`）。每次从固定 Base Mesh 开始，先应用 Shape Target（保留 incr/decr 非对称）再 PAE 足底锚点均匀缩放，**不叠加上一次输出**。复用 WP3-HOST 测量协议（`measureAll`，环分离围度）+ `rigOnSurface` 形态后关节重解析。
- **Inverse Solver**：有界 Levenberg-Marquardt，中心差分数值 Jacobian，α∈[-1,+1]（实测有效范围，未扩展）。**Tikhonov 正则（1e-3）取最小范数解**，处理零空间参数。相对残差判据（1% 阈值）。不可达目标返回最近可达 + 残差，显式标记 `UNREACHABLE_NEAREST`，不静默钳制假装命中。
- **8 项语义测量**：stature / shoulderBreadth(biacromialProxy) / ribcageBreadth / chestDepth / waistBreadth / waistCircumference / hipBreadth / hipCircumference。stature 由均匀缩放精确达成，其余 7 项由 shape target 求解。
- **Parameter Profile**：可保存可复现，记录 templateSha + shapeTargetHash + alpha + stature + achieved + profileHash。

## 2. 实测验收（`data/solve-cases.csv`）

基准测量（m）：stature 1.6659 / shoulderBreadth 0.3712 / ribcageBreadth 0.2704 / chestDepth 0.1972 / waistBreadth 0.2958 / waistCirc 0.7911 / hipBreadth 0.3601 / hipCirc 1.0036。

| 测试 | 结果 |
|---|---|
| **Known-solution recovery**（3 例） | reachable，rms 0.13%–0.45%，**maxAlphaErr 0.013–0.077**，0 翻面 |
| **Holdout combination**（3 例） | reachable，rms 0.13%–0.30%，0 翻面 |
| **Unreachable target**（腰围减半 / 肩宽×1.8） | **正确标记 reach=0**，rms 27.6% / 12.8%，跑满 60 迭代仍不可达，返回最近可达，0 翻面 |
| **Multi-target non-additivity** | 组合目标恢复 rms 0.22%，reachable |
| **Repeat determinism** | 两次求解 alpha 逐位一致 |
| **Mesh quality** | 全部解 0 翻面 |
| **Shape profile comparison** | 8 Profile（broad-shoulder/ribcage/narrow-waist/wide-hip/tall 1.80/short 1.60/combo-athletic/combo-pear），全 0 翻面，哈希记录 |

## 3. 两个如实说明

### 3.1 参数可辨识性（已处理）
`armShape` 只影响上臂围，**不在 8 项语义测量内** → 处于 Jacobian 零空间。无正则化时 LM 把它漂移到边界（首轮 maxAlphaErr=1.0，但测量仍恢复）。加 Tikhonov 正则后零空间参数保持近 0，maxAlphaErr 降到 0.013–0.077。**结论：测量恢复是验收判据；alpha 恢复仅对可辨识参数有意义。** 若需约束 armShape，须把上臂围纳入目标测量（另行扩展）。

### 3.2 非加性程度（实测，不夸大）
shape-shape 耦合在这些宽度测量上**很小**：shoulderWidth+ribcageWidth @α0.7 的 nonAdditiveDelta ≤ 0.1mm（`data/non-additivity.md`）。WP3 早前显著的_non-additivity 主要来自**身高×形态缩放耦合**（tall-athletic 肩宽请求 +60mm 实际 +81mm），由前向模型“先 target 后缩放”捕获。求解器用迭代处理任意耦合（组合目标恢复 rms 0.22% 证明），不依赖线性叠加。

## 4. 测试与回归

| 套件 | 结果 |
|---|---|
| WP3 测量回归 | 33/33 |
| WP4 回归 | 16/16 |
| Host 全量 | 65/65 |
| 引擎 CTest | 22/22 |

求解性能：known/holdout 2–3 迭代、15–28 次前向评估；不可达 60 迭代 ~836 次评估。

## 5. 证据

`evidence/wp3-r2/`：18 图（重压缩 0.37MB，像素往返校验）+ 数据表（solve-cases.csv / base-measures / known-solution / non-additivity / determinism / profiles.csv）+ 8 Profile 文件 + 5 日志。

## 6. 边界与未做

- 白名单内：`wp3_r2.h/.cpp`、`wp3_r2_main.cpp`、host CMakeLists、VR-1B 文档/证据。
- 生产路径（app/、native/）零 diff；PAHT v3/JNI/LegacyRigV1 未触。
- 未做：ANSUR 口径对齐、±1 范围外扩展（需更多 CC0 target 授权）、Android（NOT_AUTHORIZED）。
- 未 commit/push。

## 7. 下一步

1. 负责人审查求解数据 + Profile 对比图（`evidence/wp3-r2/images/`）。
2. 批准后入库（工具 / 文档+证据分组）。
3. M1 可与 M2 变形质量改进并行推进；M3 整合需 M1+M2 Gate 通过后授权。
4. 可选扩展：上臂围纳入目标以约束 armShape；更多 CC0 target 扩展可达范围（需授权）。