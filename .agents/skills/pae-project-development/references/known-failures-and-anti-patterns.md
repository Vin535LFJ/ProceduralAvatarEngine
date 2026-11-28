# Known Failures & Anti-Patterns

历史失败库。每个案例：背景/实际失败/根因/原始数据/修复或负结果/正确经验/不允许重复的行为/相关代码/报告/证据路径。未证实结论标 HYPOTHESIS，不标 CONFIRMED。

| 历史问题 | 以后必须遵守的规则 | 来源 |
|---|---|---|
| MeshData 合并不是真正几何融合 | 检查连通分量和真实共享拓扑 | R1/R2 PoC |
| 全身 Primitive/SDF 产生人偶感 | 不把连续曲面等同于自然人体 | 早期几何 |
| 身高缩放 Pivot 错误（足底 Y 当 X/Z 中心） | 验证 Root、足底和 XYZ 漂移；pivot=(rootX,soleY,rootZ) | WP3-H-R1 |
| affected-only 误当 whole-region mean | 同时报 meanAll/meanAffected/coverage/P95 | WP3-H-R1 |
| 绝对退化阈值产生尺度假象 | 进行尺度相关性验证 | WP3-H-R1 |
| 肩偏移移动测量 Mask 造成改善假象 | Before/After 必须固定 Mask | M2 肩偏移验证 |
| Core20 无手指却用零翻面评价握拳 | 确认当前 Rig 是否可表达动作 | WP4 Core/Full 对照 |
| 统一细分没有改善握拳（翻面率 7.89%→7.99%） | 不重复该方案作为主要解决路线 | M6 07a 负结果 |
| 细分权重平均降低指区纯度（0.9279） | 传递权重时保留区域和手指身份 | M6 07a |
| Bind Pose 正确但极端姿态失败 | 单独覆盖复杂姿态 | WP4 |
| 单元测试全过但视觉仍差 | 独立人眼 Gate | 全程 |
| 状态板落后最新代码 | 每轮重新核对 HEAD 和文档基线 | 治理 |
| poseGlobals 用世界轴构造右乘局部旋转 | 右乘 restLocal 的旋转用局部单位轴；非轴对齐测试验证 | M2-POSE-FRAME-AUDIT-01 |
| 正确运动学暴露指间穿透（fist 自交 130→1088） | 穿透是几何问题，需分离拓扑/限位，非坐标/细分能解 | M2 坐标审计 |
| parseFinger 误读指骨名索引（F 在 index 8 非 7） | 逐指识别须核对字符串索引；全局指标与逐指指标交叉验证 | M6 工具 |

## 重点案例详解

### 统一细分非解（M6，CONFIRMED 负结果）
背景：握拳 1056 翻面、130 自交，归因几何。假设：增加环线改善。实验：保形 4-way 细分 13,380→53,514 顶点。结果：翻面率 7.89%→7.99%（不变），自交 130→260（随分辨率放大），权重纯度 0.9279。根因：指间几何干涉与分辨率无关。正确经验：需指间分离拓扑 + FingerId 约束权重 + 自然握拳 + 关节限位。证据：`docs/phases/VR-1B/evidence/m6-finger-refinement/`。

### 坐标轴混用（M2，CONFIRMED 已修复）
背景：极端姿态翻面严重。假设：局部/世界轴混用。复现：非轴对齐单骨 X 旋转实际轴=R·ax（误差 0.632）。修复：rl 用局部单位轴。结果：WP4 30/30，elbow_120 翻面 3138→1492；但 fist 自交 130→1088（暴露真实穿透）。正确经验：数学正确≠变形质量；穿透需几何方案。证据：`docs/phases/VR-1B/evidence/m2-pose-frame-audit-01/`。

### 肩偏移 Mask 假象（M2，CONFIRMED）
背景：扫描显示 proximal-10mm 使腋窝拉伸 12.75×→5.87×。根因：扫描时腋窝 Mask 随关节中心上移，排除被拉伸皮肤。固定 Mask 重验：12.75×→14.82×（更差）。正确经验：Before/After 固定 Mask；偏移是权衡非改善。证据：`docs/phases/P1-visual-recovery/wp4-host-rig-01/evidence/m2-adoption-verification/`。