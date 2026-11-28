# M2-POSE-FRAME-AUDIT-01 — Pose Coordinate Frame Audit, Minimal Fix & Regression

Date: 2026-11-28 · Branch `poc-sm/shape-model-v2` · 基线 `e636b8c`（含 ad76b7f+e636b8c，远端 0/0，已跟踪干净，`.paht` 哈希 d3bcbbe…417b 不变）
Authority: [`MASTER-PLAN`](../../roadmap/PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md) · 任务书 PAE-M2-POSE-FRAME-AUDIT-01
Scope: Host 白名单。未触 Android/DQS/权重/关节/网格/PAHT。未 commit/push。

## 0. 最终状态

```
POSE_FRAME_AUDIT = PASS        (骨骼局部变换数学正确性，Gate A)
M2 自然变形质量验收 = 仍需独立进行（本任务不等同于变形质量完成）
```

**核心结论**：证实并修复了局部/世界坐标轴混用缺陷（数学正确性问题）。修复后肘部极端折叠大幅改善（elbow_120 翻面 −52%），但**握拳/指屈的指间几何穿透被正确运动学暴露/加剧**（fist 自交 130→1088）。这是真实几何问题，需 M6 分离拓扑/姿态限位/corrective，非坐标修复能解决。**修复是进入可靠动画/IK/Android 动态运行时的基础关卡，不是最终变形质量。**

## 1. 矩阵约定审计（以源码为准）

- **列向量**约定。`Affine::apply(p)=M·p+t`；`(A*B).apply(p)=A.apply(B.apply(p))`（B 先作用）。
- `restGlobal[b]=T(head_b)·R_b`，`R_b` 列=(ax,ay,az)，是**局部→世界**静止旋转（`wp4_rig.cpp` setFrame）。
- `restLocal[b]=restGlobal[parent]⁻¹·restGlobal[b]`。
- `poseGlobals`：`local[b]=restLocal[b]·rl`，`globals[b]=globals[parent]·local[b]`。
- `skinMatrices`：`S[b]=globals[b]·restGlobal[b]⁻¹`；Bind 时 S=I（恢复误差 3.675e-08m）。

**三种轴（不可混用）**：
- A. Bone Local Axis：(1,0,0)/(0,1,0)/(0,0,1)（骨自身局部系）。
- B. Bone Rest World Axis：`rig.ax/ay/az = R_b·(局部轴)`（Bind 下世界方向）。
- C. Bone Posed World Axis：父运动后子局部轴的世界方向 = `G_pose[parent]` 旋转作用于 B。

## 2. 已证实的缺陷（先复现，后修复）

`poseGlobals` 旧式：`rl=Rot(rig.ax/ay/az, θ)`（**世界静止轴**），右乘 `restLocal·rl`。

**推导**：右乘的 rl 作用于骨局部系，其轴应为**局部单位轴**。用世界轴 ax 时，单骨 skin
`S = T(head)·R·Rot(ax,θ)·R⁻¹·T(-head) = T(head)·Rot(R·ax,θ)·T(-head)`，
即实际绕 **R·ax = R²·e_x** 旋转，而非 ax。仅当 R·ax=ax（轴对齐 R=I）时误差消失——这正是旧 16/16 轴对齐测试通过的原因。

**复现**（修复前，`logs/tests-before-fix.log`，24 过 6 败）：
- oblique_local_X：实际轴 (0.280,−0.960,0)=R·ax，应为 ax=(0.8,−0.6,0)，位置误差 0.632。
- oblique_local_Y：实际轴=R·ay，错。
- oblique_local_Z：通过（az=(0,0,1) 恰为 R 的 z 列，R·az=az，误差消失条件成立）。
- oblique_child_local_axis_pure：子骨自身旋转非纯局部轴（relAxis≠(0,0,1)）。
- oblique_mirror_symmetry：镜像运动错误。

## 3. 最小修复（`wp4_skin.cpp`，白名单内）

`rl` 改用局部单位轴：`axis = (1,0,0)/(0,1,0)/(0,0,1)`。则
`S = T(head)·R·Rot(e_k,θ)·R⁻¹·T(-head) = T(head)·Rot(R·e_k,θ)·T(-head) = T(head)·Rot(ax_k,θ)·T(-head)`，绕正确世界静止轴旋转。
保留 `rig.ax/ay/az` 原语义；未改骨骼数/父子/JointResolver/权重/PAHT/Android。

**镜像测试预期更正**：buildRig 用固定前向+(0,0,1) 右手系，L/R 共享同一世界 az、同手性（坐标架非镜像）。故镜像对称世界运动需**相反局部符号**（L+θ/R−θ），同号给同旋向。原测试同号期望镜像是预期错误（任务 §3.3 已警告），按约定更正，非掩盖缺陷。

## 4. Gate A：坐标数学正确性（修复后）

WP4 **30/30**（原 16 + 新增 14 非轴对齐测试）：单骨斜向 X/Y/Z 旋转轴/角度/中心、无尺度、Bind 恢复、三节斜向父子链继承+骨长守恒+子局部轴纯度、左右镜像（反号）。零新警告。

## 5. Gate B：22 姿态 Before/After（`data/pose-before-after.csv/md`，54 图）

同一网格/rig/关节/Region 权重/姿态/掩码/相机；Before=旧世界轴式，After=修复局部轴式。

| 姿态 | 翻面 B→A | 自交 B→A | 判读 |
|---|---|---|---|
| bind/neutral/elbow_0 | 0→0 | 0→0 | 不变 |
| **elbow_120** | **3138→1492（−52%）** | 0→0 | **坐标错误贡献，大幅改善** |
| elbow_90 | 28→26 | 0→0 | 略改善 |
| shoulder_abduction_90 | 64→62 | 122→126 | 翻面略降，自交略升 |
| **fist** | 1056→926 | **130→1088（8×）** | 翻面/峰值拉伸降，**穿透剧增** |
| **finger_curl** | 34→70 | **0→318** | **正确屈曲暴露穿透** |
| basic_squat | 1517→1487 | 433→184 | 自交改善 |
| wrist_extension | 20→16 | 34→8 | 改善 |
| hip_flexion | 19→29 | 24→103 | 略恶化 |
| arms_raised | 3112→3124 | 248→248 | 基本不变（仍严重） |

逐区域（fist）：指区翻面 528→463、峰值拉伸 4.99→3.96、面积比 1.006→0.941（正确屈曲更自然但体积损失），全局自交 130→1088（指间/掌指穿透剧增）。

## 6. 重新归因（区分五类，不混为一谈）

- **A 坐标系错误（已修复）**：elbow_120 极端折叠翻面减半证明旧轴错误贡献显著。修复后数学正确。
- **B 权重问题**：Region 权重纯度 ~1.00（前轮已验），非主因；本轮未改权重。
- **C Joint Position**：肩偏移前轮已证非解（权衡），本轮未改关节。
- **D LBS 方法限制**：肘/膝体积损失（areaRatio<1）属 LBS 限制；仅建议 DQS/corrective 评估，未引入。
- **E 基础网格几何**：**握拳/指屈穿透是真实几何干涉**，正确运动学使其暴露/加剧（自交 130→1088）。需指间分离拓扑/姿态限位/corrective。

## 7. M6 负结果重新评估（`07a-m6-finger-refinement-report.md`）

1. 原握拳异常部分与局部轴错误有关（修复后 fist 翻面 1056→926、峰值拉伸降）。
2. 但修复后指间**真实几何穿透加剧**（自交 130→1088）——穿透不是坐标假象，是真实几何问题。
3. 单纯统一细分仍非解（前轮负结果成立）：细分不阻止指间干涉。
4. **仍需**结构化指间分离拓扑 + FingerId 约束权重 + 自然握拳姿态（MCP/PIP/DIP 分别定义）+ 关节限位。
5. 当前 70°×3 指节测试代表性有限，应定义解剖合理的 MCP/PIP/DIP 角度。
6. 下一轮 `M6-FINGER-SEPARATION-02`（需授权）：手掌/手指结构化拓扑、指间分离接触、指节区域约束权重、自然握拳、关节限位。**不得再以单纯统一细分为主方案。**

## 8. 旋转/平移边界（§3.5）

当前系统支持：**局部旋转驱动**的骨骼变形。不支持：任意 Bone Local Translation。角色整体 Root Translation 与局部骨骼平移是两种能力。本轮未扩展 Pose/PoseState/平移 API；Root Motion 若有独立问题留待后续任务。

## 9. Gate C：工程安全

Android 零修改、LegacyRigV1 零修改、PAHT v3 零修改、`.paht` 哈希不变、M6 原证据未覆盖、未 commit/push、无未授权删除/路径修改。

## 10. 回归

WP4 30/30、WP3 33/33、Host 65/65、Engine CTest 22/22。

## 11. 白名单核对

修改：`wp4_skin.cpp`（修复）、`wp4_tests.cpp`（+14 测试）、`wp4_frame_audit_main.cpp`（新）、`CMakeLists.txt`、`docs/phases/VR-1B/02a-*`、`evidence/m2-pose-frame-audit-01/`。
未触：app/**、native/renderer|runtime|procedural/human_template/**、cpp/**、PAHT/JNI/LegacyRigV1/UBO[22]/64B/mode-3/4、WP3 求解器、M6 工具/历史报告。未引入 DQS/Muscle/Corrective/新骨骼/外部模型。

## 12. 交付物索引

1. 本报告（坐标审计+修复+回归+归因）。2. 复现测试（wp4_tests 非轴对齐）。3. Before/After 矩阵/轴对照（tests-before/after-fix.log + pose-before-after.csv）。4. 最小修复（wp4_skin.cpp）。5. 非轴对齐测试。6. 22 姿态 Before/After 数据。7. 54 图（9 重点姿态×Before/After×clay/wire/closeup）。8. 回归日志。9. M6 重新评估（§7）。10. 白名单核对（§11）。

## 13. 下一步（需授权）

1. 负责人审查本报告与 Before/After 图。2. 批准后入库。3. M6-FINGER-SEPARATION-02（指间分离拓扑+约束权重+自然握拳+限位）。4. DQS/corrective 评估（肘膝体积损失）。5. 数学正确性已就绪，可进入可靠动画/IK/Android 动态运行时准备（仍需各自授权）。