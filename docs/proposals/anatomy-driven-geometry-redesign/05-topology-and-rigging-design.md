# 05 — Topology and Rigging Design

## 1. 两种动画网格架构比较

### A. 直接生成动画友好的参数化拓扑

Shape Model 先确定 edge flow / base cage，再以 landmarks、曲面约束和 profile functions 生成控制网格；高密度细分曲面作为最终表面评价。

**优点**：拓扑稳定、Region ID 可直接写入顶点/面、权重可按拓扑参数域生成、角色参数变化不会随意重排顶点、UV seam/边界合同可控。

**缺点**：肩腋/肘等复杂交汇必须一开始就设计 cage；几何形状迭代速度可能慢于 implicit prototyping；需要专业拓扑设计经验。

### B. 连续高质量表面后受控重拓扑

先由 implicit / parametric hybrid 生成参考表面，再把固定 quad topology template 投影到目标 surface；投影后做局部 relaxation、edge length、normal、intersection 和 semantic flow 优化。

**优点**：形态研究自由，能先把人体轮廓做对；可用同一参考表面对比不同 shape backends。

**缺点**：任意自动 quad remesh 往往不理解肩部环线和肘部压缩方向；投影可能改变局部体积；参数变化时模板可能自交或翻面；重拓扑质量仍需人工定义 flow。

### 推荐：A/B 分层混合，而不是二选一

用统一连续表面作为形态目标（B），用人工设计且可程序化实例化的解剖 quad cage/patch template 作为动画拓扑（A）。重拓扑不是“自动修复”，而是**受语义模板约束的投影/拟合过程**。

模板固定拓扑索引与 RegionId；Shape Model 改变 control cage / surface target，再更新 template vertex positions 和必要的局部密度，不对每次 shape 重新生成随机拓扑。

## 2. 为什么 Surface Mesh 连通并不等于动画拓扑正确

水密 Surface Nets 可以提供单连通、无边界、无非流形边的几何表面；但它按体素 cell 产生顶点，不知道：

- 肩关节应该有环线围绕肱骨头并接入胸部；
- 腋窝需要支撑压缩和外展的环线密度；
- 肘窝在屈曲侧要有连续变形带；
- 腰部要沿骨盆/胸廓的自然体块方向组织边环；
- UV seam 应避开 deformation hot spot；
- 面片密度应与曲率和形变共同规划。

因此 `components=1`、`boundaryEdges=0`、`nonManifoldEdges=0` 是必要的拓扑完整性指标，但不代表 `skinning-ready`。

## 3. 动画拓扑模板设计

### 3.1 Torso Cage

- 中央纵向 loop chain 从胸骨/胸廓、腹部、腰部延伸到骨盆。
- 环线按肋弓、腰部最窄处、髂嵴和臀上缘自然变化，不强制每层对应同一水平截面。
- 胸廓前后面密度可不同；背部肩胛区预留肩胛运动变形带。
- 腹部中线可留较规整的 quad grid，腹外斜肌/侧腰区的边线向髂嵴过渡。

### 3.2 Shoulder / Axilla Cage

- 采用环绕肩关节的主环线、胸侧连接环线、肩胛后侧连接环线与腋窝底部曲线。
- 腋窝区域需要扇形或分层 edge flow，但避免把一个高-valence pole 放在最大形变点。
- Shoulder bone / UpperArm bone 的权重交界尽量沿功能性环线走；Chest 权重参与肩带根部，而非全部用 UpperArm 驱动。
- 主拓扑 seam 不必与肩关节边界完全重合；UV seam 可以独立选择。

### 3.3 Elbow Cage

- 肘部至少预留多环过渡带；关节中心附近需支持弯曲内侧压缩和外侧拉伸。
- 在肘尖/鹰嘴的后侧高曲率区配置适当环线，但不要在关节屈曲折线处形成孤立单环硬折。
- 外侧环线适合沿伸展方向分布；内侧需要更多变形过渡面。
- 前臂旋转应让 edge flow 与骨骼 twist 方向兼容。

### 3.4 Torso-Pelvis / Hip

- 腰环与骨盆环线连续，髋关节根部从 pelvis cage 分流为左右大腿环线。
- 不允许两个大腿 sweep 插入骨盆 mesh 后依靠遮挡。
- 未来髋部采用与 shoulder 类似的共同 shape envelope + 固定拓扑模板策略。

## 4. Boundary / Continuity 的实测定义

所有指标必须从实际几何和对应关系计算，禁止在 report 结构中默认写零。

### 4.1 C0 Position Continuity

对具有参数对应关系的两侧边界点 `pA(i), pB(i)`：

\[
E_{C0}^{max}=\max_i \|p_A(i)-p_B(i)\|_2
\]

并同时检查样本数、边界 winding、索引映射、切边长度差与 Hausdorff proxy。现行基础合同阈值：`1e-4 m`。共享索引只能证明这一个顶点 identity，不等于完整 boundary contract 已验证。

### 4.2 G1 Tangent / Normal Continuity

对 smooth seam，在对应采样点求两侧单位法线 `nA, nB` 和参数切向/切平面：

\[
\theta_n=\arccos(\operatorname{clamp}(n_A\cdot n_B,-1,1))
\]

并检查两侧切平面角及 seam tangent 方向。在有意硬缝处必须标记 `HardSeam`，不应强迫法线相同。基础阈值暂按 Foundation Contract 的 5°，最终可按视觉与曲率条件细化。

### 4.3 拓扑验证

必须记录：

- vertices / faces / triangles / quads；
- components、boundary edges、non-manifold edges、non-manifold vertices；
- duplicate positions（exact + epsilon）；
- zero/near-zero area faces；
- degenerate quads / inverted faces；
- winding consistency、outward normal probes；
- self-intersection / overlapping shell 检测；
- per-region edge length / aspect ratio / valence / pole count；
- boundary contract 与 UV seam 合规性。

现有 `MeshTopology` 只覆盖其中一部分，应先在研究阶段明确新增验证项，再分批实施；不可把当前 validator 的 PASS 当成以上所有指标通过。

## 5. Rig-aware 设计与 22 Bone ABI

### 5.1 ABI 约束

继续沿用 `BoneId 0..21` 和现有父子链。`LeftShoulder = Chest → LeftShoulderJoint`，`LeftUpperArm = LeftShoulderJoint → LeftElbow`，`LeftLowerArm = LeftElbow → LeftWrist`；右侧同理。未经独立 ADR，不增加/重排 BoneId，不改变 UBO `mat4[22]` 的 1408-byte 合约。

### 5.2 共享来源

- Skeleton、Shape Model landmarks、region controls 必须使用同一 `CharacterDefinition` 和比例缩放。
- 骨骼关节点位置决定骨段；表面 landmark 定义体表形状；region graph 决定局部形变区域；不能把“顶点离某骨最近”作为唯一语义。
- 各 Region 声明 allowed bones / preferred bone / transition neighbors。
- 先由 semantic mask 限制候选集，再在局部区域中使用骨段距离和 smooth falloff 求权重。

### 5.3 初始 Region → Bone 对照

| Region | 主骨 | 过渡骨 |
|---|---|---|
| Thorax / Chest | Chest(3) | Spine(2), Neck(4), Left/RightShoulder(6/10) |
| Abdomen | Spine(2) | Hips(1), Chest(3) |
| PelvicMass | Hips(1) | Spine(2), Left/RightUpperLeg(14/18) |
| Clavicle / ShoulderGirdle L/R | Left/RightShoulder(6/10) | Chest(3), UpperArm(7/11) |
| Deltoid L/R | UpperArm(7/11) 或 Shoulder(6/10) 混合 | Chest(3), LowerArm 仅局部 |
| UpperArm L/R | UpperArm(7/11) | Shoulder(6/10), LowerArm(8/12) |
| ElbowComplex L/R | UpperArm/LowerArm 按局部子区 | Shoulder(6/10), Hand(9/13) 仅必要处 |
| Forearm L/R | LowerArm(8/12) | UpperArm(7/11), Hand(9/13) |
| Wrist L/R | LowerArm/Hand | — |

具体主骨应在动作验证中调整；表格作为初始化映射候选，不代表已批准权重表。

### 5.4 Weight Generation

推荐：

1. 每个顶点/面拥有 semantic RegionId；
2. 通过 Region allowed-bone mask 选取候选骨；
3. 以关节过渡 envelope 限定影响空间；
4. 用 bone segment distance、局部参数坐标和 smoothstep/B-spline falloff 求权重；
5. 取最多 4 骨，非负，和归一化至 1；
6. 对 shoulder/elbow 区域做连续性检查与 heatmap 证据；
7. 对比动作后的体积、轮廓和自交。

禁止仅以全局最近骨段排序来产生人体权重，避免胸肩交界和腋窝区域被错误骨骼支配。

## 6. LBS、DQS 与 Corrective 规划

### 6.1 LBS 边界

现有 CPU/GPU LBS 适合 22 骨矩阵皮肤变形，兼容性好，但矩阵插值可能导致旋转关节收缩、扭转 candy-wrapper 和体积变化。高质量权重只能缓解，不会完全消除这些算法固有缺陷。

### 6.2 DQS

Dual Quaternion Skinning 可缓解典型 LBS 旋转塌陷和 candy-wrapper，并可在 GPU 上执行；但 DQS 有自己的 joint bulging / normal distortion 风险，不能作为自动解决所有 shoulder/elbow 变形的答案。[Kavan et al. 2007](https://dcgi.fel.cvut.cz/publications/2007/kavan-i3d-sdq)；[Kim & Han 2014](https://onlinelibrary.wiley.com/doi/full/10.1002/cav.1604)

### 6.3 Corrective Shapes 与局部体积保持

推荐在 PoC-D 动作对照发现具体失败后，再决定 corrective shapes 或 local volume preservation。不要在没有姿态样本的阶段先实现完整肌肉模拟。

研究显示可以在标准 skinning 后用局部几何体积约束校正软组织，而非立刻引入完整物理模拟。[Rohmer et al. 2009](https://onlinelibrary.wiley.com/doi/10.1111/j.1467-8659.2008.01340.x)

## 7. 法线、Winding 与 LBS

- Bind mesh normals 来源应明确：高密度 field 梯度、模板极限曲面导数或几何面法线。不同来源不能混用而不记录。
- 运行时 LBS 法线目前采用 `mat3(skin)` 线性变换再归一化。若 palette 产生显著非刚性线性变换，需要评估 inverse-transpose 或 DQS normal handling；至少做 CPU/GPU 同姿态一致性比较。
- 每个三角形 winding 必须与邻面保持一致；外法线可用 signed volume、场内外 probe 或 ray test 复核。
- G1 检测不能基于渲染后插值法线“看着顺”；必须从边界两侧 surface derivatives/face normals 实测。

## 8. 动作测试矩阵

| Pose | 变形目标 | 工程检查 |
|---|---|---|
| A-Pose Bind | 初始轮廓 | identity skin palette、顶点不漂移 |
| Arms Raised 45° | 肩部启动 | shoulder region volume、法线 |
| Arms Abducted 90° | 腋窝 | 无洞/自交/折痕、腋窝曲率 |
| Arms Forward 90° | 胸肩联动 | 胸前体积不塌、肩胛/前肩连贯 |
| Elbow Flex 45° | 肘部初始弯曲 | 内侧压缩、外侧拉伸 |
| Elbow Flex 90° | 深屈肘 | 肘窝体积、鹰嘴轮廓、无单环折线 |
| Forearm Pronation/Supination | 前臂扭转 | 截面 roll、肌群不翻转 |
| Torso Flex / Twist | 躯干 | 腰部体积与边环稳定 |

每个动作在 neutral 和 action 状态同时报告：轮廓差、局部体积变化率、顶点速度异常、法线变化、三角翻面/自交。

## 9. 最终拓扑建议

- M12 的 Surface Nets 继续作为表面参考网格，不直接升级为最终 skinned production mesh。
- 优先设计一个可重复实例化的四边形动画 template，包括 torso、shoulder、axilla、upper arm、elbow、forearm 六个相邻区。
- Template 投影到连续目标表面后做局部约束优化：贴合误差、法线误差、edge length、aspect ratio、区域密度、最小体积和关节环线流向。
- 若模板投影无法稳定贴合某参数组合，应先修 Shape Model/模板约束，不允许用重拓扑自动重排掩盖。
- QuadriFlow/field-aligned remeshing 可作为离线对照，不作为动画拓扑的最终自动答案；其目标是生成高质量 quad mesh、控制 singularities，并非识别肩肘 anatomy flow。[QuadriFlow](https://stanford.edu/~jingweih/papers/quadriflow/)

