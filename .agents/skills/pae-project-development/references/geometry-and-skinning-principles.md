# Geometry & Skinning Principles

权威：`docs/architecture/human-rig-v2/01-rig-v2-architecture.md`、`docs/phases/VR-1B/02a-m2-pose-frame-audit-report.md`、`07a-m6-finger-refinement-report.md`。

## 几何原则

- 拓扑连通 ≠ 几何融合 ≠ 人体自然度。检查连通分量与真实共享拓扑。
- 全身 Primitive/SDF 不自动产生自然面部/手足；不作最终人体主线。
- 结构化手指关注：Palm / Thenar / Hypothenar / MCP / PIP / DIP / Finger Identity / Finger Separation / Natural Pose Limits / Inter-finger Collision。
- 按区域处理：Torso/Ribcage/Pelvis/Shoulder/Axilla/UpperArm/Elbow/Forearm/Hand/Fingers/Thigh/Knee/Calf/Foot/Toes/Head/Face。
- Bind + Pose 双态验证；不只优化静态正面肌肉线条。

## 形态（M1）强制规则

从固定 Base Mesh 开始；不叠加上次形态；保留 incr/decr 非对称；alpha 不超实测有效范围（除非重授权）；不可达目标显式报告；测量口径固定；关节形态后重解析；缓存绑定版本/哈希。

## 骨骼/蒙皮（M2）矩阵纪律

明确：行/列向量约定 / 矩阵存储 / Affine 乘法语义 / Rest Global / Rest Local / Pose Global / Skin Matrix / Local Rotation 轴的坐标空间。

**三种轴**：Bone Local Axis (1,0,0)/(0,1,0)/(0,0,1)；Bone Rest World Axis rig.ax/ay/az；Bone Posed World Axis（父运动后）。右乘 restLocal 的局部旋转必须用局部单位轴（M2 坐标审计已修复世界轴混用）。

需要时构造非轴对齐骨骼测试。**不能仅凭数学审查推测就宣称真实人体矩阵有缺陷，必须用测试验证。**

## 权重

Region-constrained + Side Guard + Finger Identity + Top-4 压缩 + 归一化。权重归一化≠解剖正确；须查区域/手指身份与空间有效性。细分权重传递须保留区域/手指身份（朴素平均降纯度）。

## LBS / DQS / Corrective

先 LBS 建立基准。DQS / Corrective / Muscle Simulation 都是独立研究/实施授权任务，**不得自动引入**。DQS 改善体积损失/糖纸效应，但**不解决几何穿透**（指间碰撞需分离拓扑/姿态限位）。

## 已确认的几何事实

- 肩偏移（proximal-10mm）是权衡非明确改善（固定 Mask 验证），不予默认采纳。
- 统一细分不改善握拳翻面率（7.89%→7.99%），非解。
- 正确运动学暴露真实指间穿透（fist 自交 130→1088）→ 需 M6 分离拓扑/限位。