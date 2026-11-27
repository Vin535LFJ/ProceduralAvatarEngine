# WP4-HOST-RIG-01 — Rig V2 Skeletal Control Prototype Plan

Status: 执行中（本轮授权 Host-only）
Date: 2026-11-28
Base: `poc-sm/shape-model-v2` @ `bfcada7`（开始时本地=远端，已跟踪文件干净，无其他代理活动）
上位：Master Roadmap、`docs/architecture/human-rig-v2/01-rig-v2-architecture.md`。

## 1. 权限边界

允许：`tools/human-template-poc/host/**`（新增独立 CMake targets/tests/数据/报告）、`docs/architecture/human-rig-v2/**`、`docs/phases/P1-visual-recovery/wp4-host-rig-01/**`、`docs/roadmap/**`。
严禁：app/**、native/renderer|runtime/**、app/src/main/cpp/**、native/procedural/human_template/**、PAHT v3、JNI ABI、LegacyRigV1、BoneId 0–21、UBO[22]、64B packing、mode-3、mode-4 静态路径。
不构建 APK、不部署设备、不自动 commit/push。native/ 检查器（analyzeTopology/detectSelfIntersections）只读编入 host 工具（沿用 wp3_quality 先例）。

## 2. 工作包

| 包 | 内容 | 产物 |
|---|---|---|
| WP4-A | PAE RigDefinition（语义表 core20/full92）+ JointResolver（CC0 joint 组质心 + root 锚点）+ rest/局部坐标架 | `wp4_rig.h/.cpp`、骨架定义表、关节表、对称/层级诊断、vs 研究参照偏差表 |
| WP4-B | Distance / Heat / Region-constrained 权重对照 + Top-4 + 诊断（跨侧污染、空间有效性、平滑度、未绑定） | `wp4_weights.h/.cpp`、权重表、热力图、丢弃质量 |
| WP4-C | CPU LBS：Bone Local Transform 姿态合成、位置+法线蒙皮、Bind Pose 精确恢复 | `wp4_skin.h/.cpp` |
| WP4-D | 姿态套件（§3）+ 方向断言 | `wp4_poses.h/.cpp` |
| WP4-E | 逐关节区域质量 + 7 类图像 + 性能 | `wp4_quality.h/.cpp`、`wp4_main.cpp`、证据 |

复用（只读调用，不改）：ht_obj/ht_topology/ht_target/ht_proof（加载与形态应用）、ht_rig（Distance/Heat 生成器、labelRegions、lbsSkin）、ht_render（Clay/Wire/Normal/WeightHeat 渲染）、wp3_quality（全局拓扑/自交）、wp3_measure（截面）。

## 3. 姿态套件（静态 Bone Local Transform 输入，禁止手改网格）

肩：bind、forward_flexion、abduction 30/60/90；肘：0/45/90/120；髋：flexion、abduction；膝：45/90；腕：flexion、extension；手：finger_curl、fist；全身：neutral、arms_raised、one_arm_raised、squat、asymmetric。
每姿态方向断言（姿态无关判据，沿用 R1 经验）：如屈肘=手到肩距离缩短且手在肘前；外展=腕升高；对侧关节严格不动（<0.01 mm），对侧表面允许杂散渗漏但实测报告。

## 4. 质量验收（WP4-E）

每姿态输出：Clay / Wireframe / Normal / WeightHeat / SkeletonOverlay / Before-After / 关节近景。
全局指标：boundary/non-manifold/flipped/self-intersection/体积/边拉伸（wp3_quality + ht compareSurfaces）。
**逐关节区域**（shoulder+axilla、elbow、wrist、hip+groin、knee、fingers、toes）：区域内顶点掩码（关节邻域半径），局部边拉伸 p99/max、局部面积比、区域翻面、涉及区域的自交对、骨长守恒。
阈值政策：由 bind 基准与实测分布制定并写入报告；不得为 PASS 临时放宽。
性能：每姿态 CPU 变形耗时（ms）、权重生成耗时。

## 5. 形态同步测试

9 形态：baseline、broader_shoulder、broader_ribcage、narrower_waist、wider_hip、taller(1.80)、shorter(1.60)、combo_athletic、combo_pear。
每形态完整执行：形态应用→JointResolver→RigBuild→WeightGen→Skinning→PoseValidation（bind+代表姿态）。
其中 3 形态（baseline、combo_athletic、wider_hip）执行完整姿态套件。
回答：同一 RigDefinition 能否服务不同体型；关节/权重哪些可复用、哪些必须重算；缓存键与失效条件。

## 6. Core 20 vs Full 92 对照

姿态：neutral、shoulder_abduction_90、elbow_90、hip_flexion、knee_90、fist（core 无指骨→记录不可表达）。
比较：变形质量、关节连续性、失真、权重复杂度、CPU 耗时、Top-4 丢弃质量、可扩展性。
结论：Core 适用范围、Full 必要扩展、哪些骨真正改善变形、哪些多余。

## 7. 停止条件（触发即停对应工作，如实报告，不扩权）

1. Rig 数据依赖未确认许可的外部骨架资产（设计已规避：运行时链纯 CC0）。
2. JointResolver 无法可靠适应形态。
3. 大量顶点无法正确绑定。
4. 肩/髋明显穿透或变形失败。
5. 握拳严重翻面（**已知风险**：基础网格手指环线少 + 纯 LBS，先前研究 ~700 翻面；若复现，记录为 POSE_QUALITY 部分失败并停在该项，不改基础网格）。
6. Top-4/归一化无法保证。
7. Bind Pose 无法精确恢复。
8. 必须改 LegacyRigV1 / Android 生产代码才能继续。

## 8. 最终状态字段

`PAE_MASTER_ROADMAP / RIG_V2_ARCHITECTURE = READY_FOR_REVIEW|PARTIAL|BLOCKED`
`WP4_HOST_RIG = IMPLEMENTED|TECHNICAL_PARTIAL|BLOCKED`
`BIND_POSE / SKIN_WEIGHT_QUALITY / CPU_SKINNING / POSE_QUALITY = PASS|PARTIAL|FAIL`
`ANDROID_RIG_V2 / WP3_ANDROID = NOT_AUTHORIZED`；`FINAL_HUMAN_VISUAL_GATE = PENDING`。

## 9. 证据目录

`evidence/`：skeleton-definitions.md、joint-table.csv、joint-deviation-vs-reference.csv、weight-tables.csv、weight-diagnostics.md、pose-inputs.md、images/（重压缩）、deformation-metrics.csv、region-metrics.csv、shape-sync.csv、core-vs-full.csv、test-logs/、performance.md、README.md（supersession/索引）。