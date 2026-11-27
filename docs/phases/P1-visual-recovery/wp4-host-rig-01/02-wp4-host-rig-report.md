# WP4-HOST-RIG-01 — Rig V2 Skeletal Control Prototype Report

Date: 2026-11-28
Branch: `poc-sm/shape-model-v2` @ `bfcada7`（开始时本地=远端 ls-remote 一致，已跟踪文件干净，无其他代理活动）
Scope: Host-only。未构建 APK、未部署设备、未 commit/push。生产路径零改动（git diff 证明），`.paht` 资产哈希不变 `d3bcbbe…417b`。
上位：`docs/roadmap/PAE-PROCEDURAL-HUMAN-GENERATION-ROADMAP.md`、`docs/architecture/human-rig-v2/01-rig-v2-architecture.md`、`01-wp4-host-rig-plan.md`。

## 0. 最终状态

| 字段 | 值 |
|---|---|
| `PAE_MASTER_ROADMAP` | **READY_FOR_REVIEW** |
| `RIG_V2_ARCHITECTURE` | **READY_FOR_REVIEW** |
| `WP4_HOST_RIG` | **IMPLEMENTED** |
| `BIND_POSE` | **PASS**（恢复误差 3.675e-08 m，float 权重舍入底噪） |
| `SKIN_WEIGHT_QUALITY` | **PASS**（Region 法：0 未绑定 / 0 跨侧 / 0 远绑） |
| `CPU_SKINNING` | **PASS**（位置+法线，局部变换合成，单骨/混合/继承单元测试全过） |
| `POSE_QUALITY` | **PARTIAL**（中等姿态可用伴轻微关节伪影；极端姿态严重失败，见 §5） |
| `ANDROID_RIG_V2` | NOT_AUTHORIZED |
| `WP3_ANDROID` | NOT_AUTHORIZED |
| `FINAL_HUMAN_VISUAL_GATE` | PENDING |

触发停止条件 #5（握拳严重翻面）：已在该项停止并如实报告，未扩大权限、未改基础网格。

## 1. WP4-A：RigDefinition + JointResolver

PAE 自有语义骨骼表（`wp4_rig.cpp`，冻结 authored 数据）：
- **Full Deform Profile v1 = 92 骨**，**Body Core = 20 骨**。语义 ID 与 GPU palette 索引解耦。
- 关节**全部从 CC0 `base.obj` 的 `joint-*` 辅助组质心解析**（变形后坐标），运行时链不读 PENDING 的 `default.mhskel`/`default_weights.mhw`。
- Root head = PAE authored 锚点（静止时距 pelvis 质心+(0,−0.015,−0.092)m 最近的 body 体表顶点，稳定 ID 跟踪）。
- 局部坐标架 Y=head→tail、Z=前向投影、X=Y×Z；rest 全局/局部矩阵显式计算。

实测：
- full92 / core20 均通过校验（语义 ID 唯一、无环、父存在、无零长骨）；最短骨 0.00847 m。
- L/R 对称：55 对关节，**最大不对称 0.000000 m**。
- 9 形态全部关节解析成功（无未解析）。

**与研究参照（PENDING mhskel）偏差**（`joint-deviation-vs-reference.csv`，仅 Host 研究）：26 个共享骨。
- head 偏差 ≤ 0.083 m：来自“joint 组质心”与“源文件顶点均值”的口径差异（mhskel 关节顶点集与 OBJ 立方体组不完全重合）。
- tail 偏差最大 0.341 m（thigh）：**有意合并**所致——PAE v1 把 upperleg01+02 合并为整条 thigh（tail=膝），参照的 upperleg01 tail 在大腿中段。forearm/shin/neck 同理。这是设计差异，不是解析错误。
- 结论：PAE 解析器自洽（bind 精确、对称完美），与参照的差异已逐项归因。

## 2. WP4-B：权重生成器对照

| 方法 | 未绑定 | 跨侧质量 | 跨侧顶点 | 远绑 | 平滑 L1 | 生成 ms |
|---|---|---|---|---|---|---|
| Distance | 0 | **34.857** | 多 | **1077** | 0.1106 | 23 |
| Heat | 0 | 0.081 | 少 | 29 | 0.1261 | 339 |
| **Region（主用）** | **0** | **0.000** | **0** | **0** | 0.1195 | 23 |

- Distance 在中线附近严重跨侧污染（34.86 质量单位）且 1077 个顶点主导骨落在错误区域——**“全部有权重”≠绑定正确**的实证。
- Region 约束（区域允许集 + 左右守卫 + 空顶点回退到最近允许骨）消除跨侧与远绑，平滑度相当，速度与 Distance 同。
- 本轮姿态蒙皮主用 **Region**；Distance/Heat 仅作对照。Top-4 丢弃质量：core 0.042 / full 0.112。

## 3. WP4-C：CPU LBS

- 姿态 = Bone Local Transform（语义骨 ID + 局部轴 + 角度），`G_pose[b]=G_pose[parent]·restLocal[b]·R_local[b]`，`S[b]=G_pose[b]·restGlobal[b]⁻¹`。
- 位置 + 法线蒙皮（刚体逆置=旋转）；每次从 rest 出发，不叠加。
- **Bind Pose 恢复误差 3.675e-08 m**（float 权重舍入；双精度合成单元测试 < 1e-12）。
- 单元测试 16/16：bind 恢复、单骨旋转、加权混合、法线旋转、父子继承、零长/环拒绝、质量守恒、跨侧诊断、区域掩码。

## 4. WP4-D：姿态套件与方向断言

22 个静态姿态（肩前屈/外展30/60/90、肘0/45/90/120、髋屈/外展、膝45/90、腕屈/伸、指屈/握拳、站立/举双臂/举单臂/深蹲/不对称）。
**方向断言 0 失败**（首轮抓到前屈/屈肘/屈膝符号反了，已按断言修正姿态定义，而非放宽阈值）。断言判据姿态无关：屈肘=手向肩靠近且手在肘前；外展=腕升高；屈膝=踝向后；髋外展=膝向外；举单臂=对侧腕严格不动。

## 5. WP4-E：变形质量（诚实图景）

全局（`deformation-metrics.csv`）+ **逐关节区域**（`region-metrics.csv`）。身体平均会掩盖坏关节，逐区域暴露：

| 姿态 | 全局翻面 | 关键区域实测 |
|---|---|---|
| bind / neutral / elbow_0 / elbow_45 / knee_45 | 0 | 干净 |
| shoulder_abduction_90 | 64 | **腋窝边拉伸 max 12.7× / p99 9.0×，18 翻面，面积比 1.185** |
| elbow_90 | 28 | 肘窝面积比 **0.72**（体积损失），2 翻面 |
| knee_90 | 32 | 膝区面积比 **0.67**，minNormalDot 0.267 |
| hip_flexion | 19 | 腹股沟轻微 |
| finger_curl | 34 | 指区轻度 |
| **fist** | **1056** | **指区 528 翻面/侧，边拉伸 max 4.99× / p99 2.02×** |
| **elbow_120** | **3138** | 前臂折回上臂 |
| **arms_raised** | **3112** | 肩区严重 |
| **one_arm_raised** | **1556** | 肩区严重 |
| **basic_squat** | **1517** | 髋膝复合 |

结论：
- **中等姿态可用**，伴关节区轻微 LBS 伪影（肘/膝体积损失、肩腋窝拉伸）。
- **极端姿态严重失败**（握拳/屈肘120/举臂/深蹲）：基础网格手指环线密度不足 + 纯 LBS 的固有限制，与先前研究（握拳 ~700 翻面）一致。这是几何/蒙皮方法问题，**不是权重调参或骨骼数量能解决的**。
- 肩外展质量不随骨骼数增加而改善（见 §6）→ 指向权重/LBS 方法，需 DQS/corrective 或 M6 几何细化（后续授权）。

## 6. Core 20 vs Full 92 对照（`core-vs-full.csv`）

| 姿态 | core 翻面 | full 翻面 | 说明 |
|---|---|---|---|
| neutral | 0 | 0 | 一致 |
| shoulder_abduction_90 | 34 | 64 | core 反而更少（full 权重更复杂） |
| elbow_90 | 28 | 28 | 前臂相同 |
| hip_flexion | 24 | 19 | full 略好（pelvis 骨） |
| knee_90 | 32 | 32 | 相同 |
| **fist** | **0（不可表达）** | **1056** | **core 无指骨，fistExpressible=0；“0 翻面”是假象** |

- CPU：core ~0.09 ms / full ~0.12 ms（均 trivial）；Top-4 丢弃 core 0.042 / full 0.112。
- **推荐**：Core 20 适用于躯干+四肢粗大运动；**手指/脚趾/髋细节必须 Full**。Full 的额外骨骼并未显著改善肩部——肩部质量是权重/LBS 问题。骨骼数量应由变形质量与设备性能决定，本表不是最终生产定案。

## 7. 形态同步（`shape-sync.csv`）

9 形态（baseline / broader-shoulder / broader-ribcage / narrower-waist / wider-hip / taller 1.80 / shorter 1.60 / combo-athletic / combo-pear）全部：关节解析成功、rig 校验通过、bind 精确（≤3.69e-08）、0 未绑定、L/R 对称 0.0。
**回答契约问题**：同一 RigDefinition（语义表）可服务所有体型；**关节位置必须每形态重解析，权重必须每形态重生成**（本轮即如此）。缓存键 = `hash(BaseMesh)+hash(ShapeVector)+rigProfileVersion+weightGenVersion`；形态改变→关节重解析→权重重算。3 形态（baseline/combo-athletic/wider-hip）执行了完整 22 姿态套件。

## 8. 测试与性能

| 套件 | 结果 |
|---|---|
| WP4 专项 | **16/16** |
| WP3 回归 | **33/33** |
| Host 全量 | **65/65** |
| 引擎 CTest（WP1/WP2/Legacy） | **22/22** |

CPU 蒙皮 ~0.2–0.3 ms/姿态（13,380 顶点）；权重生成 Distance/Region ~23 ms、Heat ~339 ms。峰值内存 Cygwin 下 NOT MEASURED（沿用前轮限制）。

## 9. 证据

`evidence/`：184 张图（重压缩 4.4 MB，像素往返校验通过）+ 11 份数据表/报告 + 5 份日志。
- `data/`：skeleton-definitions.md、joint-table.csv、joint-deviation-vs-reference.csv、weight-diagnostics.md、weight-table.csv、pose-inputs.md、deformation-metrics.csv、region-metrics.csv、shape-sync.csv、core-vs-full.csv、assertions.md。
- `images/`：base-*（22 姿态 × clay/wire/normal/heat/overlay + 近景）、shape-*-bind、combo-athletic-* / wider-hip-*（各 22）、cmp-core20/full92-*（各 6）。
- `logs/`：experiment-run、wp4-tests、wp3-tests、ht-tests、engine-ctest。

## 10. 边界与未做

- 白名单内：仅 `tools/human-template-poc/host/CMakeLists.txt`（改）+ 12 个 wp4 源文件（新）+ 3 个 docs 目录（新）。
- 生产路径（app/、native/renderer|runtime|procedural/human_template/、cpp/）**零 diff**；PAHT v3/JNI/LegacyRigV1/UBO[22]/64B/mode-3/4 未触。
- 未做：DQS/corrective/肌肉模拟、Android GPU Rig V2、动画控制器、IK、材质。
- 未 commit/push（按授权）。

## 11. 下一步授权清单

1. **审查本报告 + 路线图 + Rig V2 架构**，人眼看 `evidence/images/`（尤其 base-fist-*、base-shoulder_abduction_90-*、base-elbow_90-* 与 overlay）。
2. 批准后按任务分组 commit/push（工具 / 文档+证据）。
3. **WP3-R2 参数求解器**（M1，与 WP4 并行）：组合非加性逆求解。
4. **变形质量改进**（需新授权）：肩/肘/膝 DQS 或局部 corrective 评估；握拳需 M6 手指几何细化或 corrective——纯 LBS 已证不足。
5. 退化判定尺度相对阈值（native 白名单）。
6. M3 整合契约统一后，再申请 M4 Android 动态路径（先做目标设备 palette/UBO 能力测试）。