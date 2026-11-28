# Rig V2 Architecture — PAE-Owned Skeletal Definition

Status: `RIG_V2_ARCHITECTURE = READY_FOR_REVIEW`
Date: 2026-11-28（WP4 轮次创建；MASTER-PLAN-01 复核引用）
Scope: Host 研究 + 未来 Android 动态路径的骨骼契约。本文件不修改任何生产代码。
上位文件：[`MASTER-PLAN`](../../roadmap/PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md)（M1–M8 唯一权威）、[`CharacterInstance 契约`](../human-character-instance/01-character-instance-contract.md)、ADR-014。本文件是 M2 Rig 子系统的权威细化，不另立总路线。

## 1. 六个必须分离的概念

| 概念 | 载体 | 说明 |
|---|---|---|
| Skeleton Topology | `RigDefinition.bones[].parentSemanticId` | PAE 自有层级，与源文件命名无关 |
| Joint Position | `ResolvedJointSet` | 形态应用后从 CC0 几何解析的 XYZ |
| Rest Transform | `G_rest[b] = T(head) · R_rest[b]` | 初始姿态全局矩阵 |
| Bone Orientation | 局部坐标架（Y=head→tail，Z=前向投影，X=Y×Z）+ roll 参考 | 姿态输入的旋转框架 |
| Deformation Role | `Role` + `deformGroup` | 蒙皮分区与权重约束依据 |
| Palette Index | 运行时分配（Android M4） | **不得**作为语义 ID；语义 ID 与数组索引解耦 |

LegacyRigV1 的 22 个 BoneId 是 palette 索引语义，保持冻结；Rig V2 不复用该编号体系。

## 2. 资产来源与合规边界

- **关节位置来源（生产允许）**：CC0 `base.obj`（commit `a8bc2d54`，文件头 CC0 已核实）内的 `joint-*` 辅助组（每组 8 顶点立方体）质心 + PAE authored 锚点规则。目标变形作用于完整 OBJ（含辅助组），因此关节天然随形态移动（WP3 已实测 helper/joint 位移 14.9 mm）。
- **研究参照（禁止分发）**：`default.mhskel`、`default_weights.mhw` 仍 `PENDING_CONFIRMATION`。本轮用其一次性核对 PAE 解析器偏差（Host 研究），运行时链不读取它们。
- PAE 自有内容：语义 ID 表、层级、rest 计算、局部坐标架/roll 规则、变形分组、profile 定义。全部冻结在 `wp4_rig.cpp` 的 authored 表中，附 provenance 注释。

## 3. Profiles（骨骼数量不写死）

### 3.1 Body Core Profile（20 骨，首个身体控制实验配置）
`root, spine.05, spine.03, spine.01, neck, head` + 每侧 `clavicle, upper_arm, forearm, hand, thigh, shin, foot`（7×2）。
用途：建立第一个可运行骨骼控制模型；肩/肘/髋/膝基础动作。
局限（预期，需实测确认）：无手指/脚趾/掌骨，握拳与足部动作不可表达；脊柱仅 3 段。

### 3.2 Full Deform Profile v1（92 骨）
Core 基础上增加：
- `pelvis.L/R`（root→髋关节的桥接骨）
- 脊柱补全 `spine.04, spine.02`（5 段脊柱）
- `shoulder.L/R`（肩胛带）
- `metacarpal.01–04.L/R`（8）
- `finger.01–05.01–03.L/R`（30，finger.01=拇指）
- `toe.01.01–02.L/R` + `toe.02–05.01–03.L/R`（28）

与研究参照 102 骨的差异（**有意简化，记录在案**）：
- neck02/neck03 合并为单 `neck`；upperarm02/lowerarm02、upperleg02/lowerleg02 的扭转细分合并为单 `upper_arm/forearm/thigh/shin`。
- 原因：这些细分骨的中段 landmark 在源文件中引用体表顶点（PENDING 依赖），且本轮姿态套件不需要颈/扭转细分。合并后**全部 92 骨的 head/tail 都可从 CC0 joint 组解析**（root head 除外，见 §4.3）。
- 面部骨（jaw/眼/口轮匝肌等）与 breast 骨不在 deform profile（与既有研究一致）。
- 骨骼数量最终由变形质量与设备性能决定（M2 对照 + M4 设备测试），本表不是最终生产定案。

### 3.3 可选扩展（本轮不实现，仅预留语义）
Hand/Foot 已含于 Full；Facial Extension（下颌/眼）、Twist/Auxiliary（前臂/大腿扭转骨）留待 M6/M4 后按实测需求加入。每顶点 Top-4 influences 与骨骼总数是两个独立概念：92 骨不意味着每顶点 92 影响。

## 4. Joint Resolver 契约

输入：Base Human Geometry + 当前 Shape Target 组合（作用于完整 OBJ）+ 本 RigDefinition。
输出：`ResolvedJointSet`（每关节 XYZ、每骨 head/tail、rest 朝向、骨长、L/R 对称诊断、未解析清单）。

### 4.1 解析顺序（强制）
Shape Target 应用 → Full Geometry → Landmark 解析 → Joint 位置 → Rest Pose → 权重 → 蒙皮 → 姿态验证。**严禁使用基础人体固定关节。**

### 4.2 Landmark 规则
- 常规关节：`joint-<name>` 组顶点质心（变形后坐标，米制）。
- 左右对称诊断：`|joint.L + mirror(joint.R)|`（x 取反）应 ≈ 0；报告每对偏差。
- 检查：无零长骨（|tail−head| > 1e-6 m）、无层级环、语义 ID 唯一、无未解析关节。

**四类 landmark 必须区分（MASTER-PLAN M2-B）：**
- SurfaceLandmark：体表解剖标志（如肩峰 acromion）。
- BoneJointCenter：骨关节旋转中心（如盂肱关节中心）。
- RigJoint：本 RigDefinition 采用的关节（当前 = CC0 joint 辅助组质心）。
- GeometryAnchor：几何缩放/根锚点（§4.3）。

**已知限制：** 当前 WP4 解析器把 joint 辅助组质心直接作 RigJoint，**未显式建模 SurfaceLandmark→BoneJointCenter 的解剖偏移**。尤其**肩峰表面位置 ≠ 肩关节中心**（盂肱关节中心在肩峰内下方）。

**肩偏移研究结论（2026-11-28，见 wp4-host-rig-01/04 号报告）：** 沿肱骨轴近端移 10mm 的偏移已实现为 `buildRig` 可选参数（v2.2），但**固定掩码严格验证表明它是权衡而非明确改善**（中等角度翻面减少，峰值拉伸增大、90° 恶化），先前"减半拉伸"结论系测量掩码假象。**默认不采纳（保持 v2.1 helper 质心）**。肩腋窝变形质量需 M6 几何细化或局部 corrective，单纯移动关节中心已证非解。

### 4.3 Root Transform（显式数学关系）
- 几何缩放锚点（WP3-R1：pivot=(rootX, soleY, rootZ)）**不等于** Root Joint。
- Root Joint（`root` 骨 head）= PAE authored 锚点：静止时距 `pelvis 质心 + (0, −0.015, −0.092) m` 最近的 body 体表顶点（对应研究参照 vertex 4223，偏差实测报告），以稳定顶点 ID 跟踪变形。
- root tail = pelvis 质心。世界根节点 ≠ 骨架局部原点：rest 全局 `G_rest[root]=T(head_root)`，局部 rest `L_rest[b]=G_rest[parent]⁻¹·G_rest[b]`。
- 均匀缩放 k：所有关节坐标按 WP3 锚点公式变换；骨长 ×k；root head 的锚点偏移随 pelvis→spine.04 长度比例缩放（保持解剖比例）。

## 5. Rest / Pose / Skin 数学

- 局部坐标架：`Y = normalize(tail−head)`；`Z = normalize(anterior − (anterior·Y)Y)`，anterior=(0,0,1)；`X = Y×Z`。|anterior×Y| < 0.05 时回退 anterior=(0,1,0) 投影（足部等近水平骨）。Roll 参考即该坐标架；研究参照的 rotation_plane 仅作偏差核对。
- `R_rest[b] = [X Y Z]`（列向量）。`G_rest[b] = T(head_b)·R_rest[b]`。
- 姿态输入 = 每骨局部旋转 `R_local(b, axis∈{X,Y,Z}, θ)`：
  `G_pose[b] = G_pose[parent] · L_rest[b] · R_local[b]`（绕自身 head、在自身坐标架内旋转）。
- Skin 矩阵 `S[b] = G_pose[b] · G_rest[b]⁻¹`。Bind Pose（全 R_local=I）→ S=I **精确**恢复 rest。
- LBS：`v'ᵢ = Σⱼ wᵢⱼ S[j] vᵢ`（K≤4）；法线 `n'ᵢ = normalize(Σⱼ wᵢⱼ R_S[j] nᵢ)`（刚体变换，逆置=旋转）。
- 每次变形从 Rest Mesh 出发，不叠加。

## 6. 权重契约（WP4-B）

- 生成器候选：PAE Distance、PAE Heat、Region-constrained（区域+左右约束的距离法）、Hybrid（仅必要时）。
- 约束：PAE 自生成、当前形态 Rig、稳定顶点 ID、Top-4、归一化、无未绑定、左右不互污、手指/脚趾/躯干分区合理、接缝平滑。
- 验收不得只看“全部有权重”：必须叠加骨骼角色+空间距离有效性（反例：胸腹顶点绑 head）。
- 缓存键：`hash(BaseMesh) + hash(ShapeVector) + rigProfileVersion + weightGenVersion`；形态改变→关节重解析→权重按误差规则决定重算。

## 7. Android 后续（本轮不实施）

独立新运行时路径；palette 容量、顶点属性布局、64B 复用与否、CPU/GPU parity、UBO 限制、Surface 生命周期、缓存——全部在 M4 目标设备实测后决定。不得假定 92/102 骨 palette 全设备可用。

## 8. 本轮验证义务

- PAE 解析器 vs 研究参照（mhskel）关节偏差表（Host 研究，一次性）。
- 9 形态 × 关节跟随；3 形态 × 全姿态；Core/Full 对照。
- 停止条件：见 WP4 计划。