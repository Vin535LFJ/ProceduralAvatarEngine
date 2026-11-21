# M11 — 02 现有实现审计（全量代码审计结论，含行号）

## 几何生成层

**ParametricSurfaces.cpp（122 行）**
- Sweep（cpp:55-88）：无样条插值——1 输入点=1 环（cpp:63）；半径逐环常量（cpp:64），锥度只靠环间连线；截面仅正圆（cpp:66-68）；无截面旋转/扭转；PTF 真实（cpp:23-53，swing 四元数+重正交化），但 `binormal` 用无回退 `normalized()`（cpp:42,50，退化时塌缩为零向量）；**法线纯径向，无锥度倾斜**（cpp:71）；无端盖（开口环）；UV u 从不到 1.0、接缝四边形回绕反向（cpp:72-73）；winding 朝外 ✓（cpp:84）。
- Loft（cpp:90-120）：截面线性排布（cpp:96）；椭圆轴硬编码世界 X/Z，无旋转（cpp:103）；法线为解析椭圆法线但 y=0（无锥度倾斜，cpp:104）；**UV v 恒 0**（cpp:105）；winding 假设截面沿 +Y 排布，下行时翻面（cpp:109-118）。
- 计数公式：V=N*S，三角=2*(N-1)*S（扫掠/放掠）；V=(rings+1)*S（球/椭球）；圆角盒 V=6*(subdiv+1)²。

**Primitives.cpp（217 行）**
- 椭球法线为解析非均匀法线 ✓（cpp:92-95）；球/椭球/胶囊极点为重合环（每极 S 个简并三角形，validator 容忍）；胶囊盖法线解析半球（cpp:140-142），winding 反向约定（cpp:145，仍朝外）；圆角盒 = 核心盒+球 Minkowski（cpp:169-198），面间不焊接，corner 顶点法线一致 ✓。
- 生成器不写 color/joints/materialId——全部在 Humanoid.cpp:48-99/425-429 烘焙；无切线字段（MeshData.h:11-18）。

**MeshValidator.cpp（88 行）**：有限性/单位法线/权重和 1±1e-3/索引<22 骨/简并三角形占比>12.5% 失败/AABB 一致/submesh 范围。**不检查**：winding 朝向、流形/边邻接、UV/颜色有限性、重复顶点。

## 骨骼/蒙皮/重定向层

**Skeleton.cpp（:20-74）**：bindLocal 纯平移（:61）→ 恒等旋转=绑定姿势（测试锁定）；bindGlobal=parent*local（:66）；inverseBind 失败时静默代单位矩阵（:70）。A-Pose：上臂 24°/前臂 10°（Humanoid.cpp:119-120）。Head 尾=头顶+15cm 人工桩（Skeleton.cpp:36）；Toe 尾+8cm（:51）。

**Skinning.cpp**：computePose=global*inverseBind（:7-19，标准）；palette std140 1408B（测试锁定）；CPU 参考蒙皮跳过 w<=0 或越界关节（:30）。

**Retargeting.cpp（全文）**
- **置信度死代码**：:102 `weight=0.4+0.6*conf*visibility` → :103 `(void)weight`。
- 仅 10 个肢体骨 + Spine（髋中→肩中）+ Chest（肩中→鼻）有目标；**Root/Hips/Neck/Head/Shoulder/Hand/Toe 永无目标**，经父逆链（:161-169 `worldAccum[parent].inverse()*target`）**每帧重置回恒等**；无目标骨不保持上一帧。
- 根/髋平移从不驱动；无髋线坐标系；无关键点缺失保持。
- convertPoseFrameToLocal（:58-88）：镜像单次应用 ✓（:80）；**ProviderWorld 直通未转换**（:64-68 改标签返回）；**PixelImage 的 z 未归一化**（:84 一律 ×0.25）；无输出钳制。
- constrainJoint（:13-54）：单一角度上限（肘 2.6/膝 2.4/肩 1.9/髋 1.6/脊柱 0.5）+ 粗糙铰链轴吸附（|z|>0.2→±Z，|x|>0.2→±X）；无摆动/扭转分解、无解剖方向约束（膝可反向弯）。
- MotionRuntime：conf<0.25 丢帧（:61）、0.8s 渐退（:66）、平滑率 2+8*conf（:75）——置信度仅有的生效点。

**OneEuroFilter.h**：标准 One Euro；reset 仅清 initialized_；非有限输入直通不污染状态。

**测试契约（必须维持）**：skinning（恒等=绑定 1e-3、单骨刚体绕 bone head、UBO 1408B）；humanoid（钳制、父<子、IBM 逆、>1000 顶点、≥10 submesh、bounds.y>1.4、权重和 1±1e-3、joints[0..1]<22、memcmp 确定性、热力图变化）；motion（镜像左右、抬臂 wrist.y+0.12、reset 后 2e-2 恒等、渐退）。

## 与目标差距汇总（15 项确认）

1. 拼装式人体 ✓；2. 躯干截面不足 ✓；3. 肢体扫掠仅 3 点 ✓；4. 关节靠球体 ✓；5. 合并≠融合 ✓；6. 面部=基础几何拼接 ✓；7. 无服装外壳 ✓；8. Shader 简单 ✓；9. 扫掠/放掠法线数学缺陷（无锥度倾斜、PTF 退化风险）✓；10. 蒙皮=路径距离两骨插值 ✓；11. 置信度未参与 ✓（:102-103）；12. Z=0 ✓（Codec 显式置零）；13. 重定向=纯方向向量 ✓；14. 镜像/旋转需实证 ✓（Adapter 恒 mirrored=true，无后置分支）；15. 文档一致性 → M11-I 统一核对。
