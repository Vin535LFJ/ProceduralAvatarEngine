# Rig / Skinning Workflow

适用：骨骼/关节/权重/蒙皮/姿态任务（M2）。

1. **Preflight + 授权**：核对 Git/白名单。Host 研究 HOST_IMPLEMENTATION；改生产 Rig/Android 需授权。冻结 LegacyRigV1/BoneId 0–21/UBO[22]/64B/JNI。
2. **矩阵审计先行**：明确行/列向量、Affine 乘法、Rest Global/Local、Pose Global、Skin Matrix、Local Rotation 轴坐标空间。三种轴（Local/Rest World/Posed World）不混用。
3. **非轴对齐测试**：构造斜向骨骼测试验证局部 X/Y/Z 旋转、父子继承、镜像（反号）、Bind 恢复、旋转中心/骨长。**不能仅凭数学审查宣称缺陷，必须测试复现。**
4. **关节**：JointResolver 形态后重解析；区分 Surface Landmark/Bone Joint Center/Rig Joint/Geometry Anchor；肩峰≠盂肱中心。
5. **权重**：Region-constrained + Side Guard + Finger Identity + Top-4 + 归一化。查区域/手指身份与空间有效性（归一化≠解剖正确）。
6. **EL-09 单变量**：旋转轴 / Joint Position / Weight / 拓扑 / DQS 逐项验证，不同时改。
7. **姿态回归**：22 姿态 Before/After，固定 Mask，逐关节区域指标，翻面率，区分穿透/折叠/体积损失。Bind 过≠Extreme 过。
8. **DQS/Corrective**：独立授权任务，不自动引入。DQS 不解决几何穿透。
9. **交付**：implementation-report + Gate 评估 + STATUS。

查负结果库：肩偏移 Mask 假象、Core20 无手指评握拳、坐标轴混用、细分降纯度。