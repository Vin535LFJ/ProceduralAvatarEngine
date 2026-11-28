# M2 Deformation Quality Improvement — Shoulder Offset + Extreme-Pose Attribution

Date: 2026-11-28 · Branch `poc-sm/shape-model-v2` @ `7282baa`（本地=远端）
Authority: [`MASTER-PLAN`](../../../roadmap/PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md) §10.1 · [`rig-v2 架构`](../../../architecture/human-rig-v2/01-rig-v2-architecture.md) §4.2
Scope: WP4 Host 白名单。未触 Android、**未引入 DQS**、未 commit/push。生产路径零 diff。

## 0. 结论摘要

1. **肩关节中心解剖偏移有效**：把肩旋转中心沿肱骨轴向**近端移 10mm**（PAE authored 偏移），肩外展 90° 腋窝边拉伸 **edgeMax 12.75×→5.87×、edgeP99 9.02×→5.66×、面积比 1.185→1.115**。这是关节中心放置带来的实测改善，建议采纳。
2. **极端姿态翻面归因 = 几何问题，不是权重绑定**：所有区域权重纯度 **0.98–1.00**，排除绑定污染。翻面来自穿透+局部折叠（自交定位证实）。**权重调参不能解决；需 M6 几何细化或 corrective（DQS 后续授权）。**

`M2_HOST` 仍为 **TECHNICAL PARTIAL**（肩部改善已实测；极端姿态仍失败，归因明确）。

## 1. 肩关节中心几何（实测）

| 量 | 值 (m) |
|---|---|
| joint-l-shoulder 辅助组质心 | (0.1677, 0.5246, 0.0146) |
| 肩峰体表标志（PAE 检测：肩带最高外侧体表顶点 v8226） | (0.1537, 0.5742, 0.0109) |
| 肩峰→辅助质心偏移 | (+0.014, −0.0496, +0.0037) |
| 肱骨轴（肩→肘）单位向量 | (0.638, −0.770, −0.006) |

**发现**：CC0 辅助质心已在肩峰**下方 49.6mm、外侧 14mm**（在内部，非 naive 贴体表）。因此不能断言一个未验证的"解剖修正"；本轮用**参数化偏移扫描实测**判定。

## 2. 偏移扫描（肩外展腋窝区域，`data/shoulder-offset.csv`）

| 偏移 | 外展90° 翻面 | edgeP99 | edgeMax | 面积比 |
|---|---|---|---|---|
| baseline | 18 | 9.02 | 12.75 | 1.185 |
| medial-15mm | 12 | 6.61 | 9.96 | 1.234 |
| inferior-15mm | 18 | 6.93 | 9.96 | 1.196 |
| medial-inferior-15mm | 9 | 7.91 | 11.56 | 1.163 |
| **proximal-10mm（沿肱骨轴近端）** | **17** | **5.66** | **5.87** | **1.115** |

外展 30°：proximal-10mm 翻面 1 / edgeMax 2.72（baseline 4 / 5.34）；外展 60°：4 / 4.33（baseline 10 / 9.36）。
**proximal-10mm 在所有角度显著降低边拉伸**，解剖上对应盂肱关节中心相对辅助点更近端。建议作为 PAE 肩关节偏移采纳（仍 Host，未进生产）。

## 3. 极端姿态翻面归因（`data/extreme-attribution.csv` / `selfint-localization.md`）

| 姿态 | 全局翻面 | 自交对 | 自交定位 | 权重纯度 | 归因 |
|---|---|---|---|---|---|
| **fist** | 1056 | 130 | **hand×hand 130**（指间穿透） | 指 1.000 | 手指几何穿透+折叠 |
| **elbow_120** | 3138 | **0** | 无穿透 | 肘 0.980 | **纯 LBS 折叠**（肘窝面积比 0.570） |
| **arms_raised** | 3112 | 248 | shoulder-girdle×upper-arm 174 / ×thorax 34 | 肩 0.989 | 肩带穿透上臂/胸廓 |
| **basic_squat** | 1517 | 433 | lower-leg×lower-leg 122 / pelvis×pelvis 114 | 膝 0.985 | 髋膝极端屈曲穿透 |

**逐指分解（fist，`data/per-finger.csv`）**：每指 79–126 翻面，L/R 完全对称（finger.02/03 最多 121/126）。对称性表明是系统性几何限制，非随机权重错误。

**关键判定**：权重纯度 0.98–1.00 → **绑定正确**；翻面与自交集中在关节/手指几何 → **基础网格环线密度 + 纯 LBS 的固有限制**。这与 MASTER-PLAN §10.1 假设一致：极端姿态需 M6 几何细化或局部 corrective，**不是权重调参或骨骼数量能解决的**。elbow_120 自交=0 证明是纯折叠（无穿透），DQS 可改善体积损失但翻面仍需几何/corrective。

## 4. 测试与回归

WP4 16/16、WP3 33/33、引擎 ctest 22/22。零编译警告。

## 5. 证据

`evidence/m2-quality/`：18 图（重压缩 0.27MB，像素往返校验）+ shoulder-offset.csv / shoulder-geometry.md / extreme-attribution.csv/md / selfint-localization.md / per-finger.csv + 4 日志。
图：shoulder-{baseline,medial-15mm,inferior-15mm,medial-inferior-15mm,proximal-10mm}-clay/closeup（外展90°）；extreme-{fist,elbow_120,arms_raised,basic_squat}-clay/closeup。

## 6. 边界与未做

- 白名单内：`wp4_q2_main.cpp` + host CMakeLists + wp4-host-rig-01 文档/证据。
- 生产路径零 diff；未引入 DQS/corrective；未触 Android。
- 肩 proximal 偏移目前仅 Host 实验，**未写入生产 RigDefinition**（需后续授权 + 形态同步验证）。
- 未 commit/push。

## 7. 下一步

1. 负责人审查肩偏移对照图 + 极端姿态归因。
2. 批准后入库。
3. 采纳肩 proximal 偏移进 RigDefinition（需验证 9 形态同步 + 全姿态回归）。
4. 极端姿态：申请 M6 手指/关节几何细化授权，或 DQS/corrective 评估授权（本轮未引入）。