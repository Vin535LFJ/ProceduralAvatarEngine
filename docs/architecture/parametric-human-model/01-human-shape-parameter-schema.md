# PAE 自主人体参数体系设计

文件：`docs/architecture/parametric-human-model/01-human-shape-parameter-schema.md`
任务：P1-VR / Deliverable R3
日期：2026-11-25
状态：VR-0 架构设计；不是生产实现授权，不改变任何 Gate 结论。

## 1. 设计目标

建立一套 PAE 自己的、可由 C++17 求值的人体语义参数体系，使人体比例、解剖结构、局部形态、拓扑和变形职责分离。该体系必须满足：

- 不依赖外部人体网格、形态基底或训练数据。
- 不改变 22 骨骼、BoneId 0–21、`UBO[22]`、每顶点最多 4 个权重、64-byte GPU packing 或 JNI ABI。
- 能驱动躯干和四肢的可变截面曲面。
- 能为面部、手部、足部提供独立的结构化拓扑输入。
- 能表达左右差异，而不是强制镜像。
- 能记录每个参数的单位、语义、影响区域、来源置信度和实现状态。
- 能作为 Host 测试、Android 生成、Render Mesh 和后续 corrective 的共同上游。

## 2. 五类技术分层

| 类别 | PAE 数据结构 | 职责 | 不应承担 |
|---|---|---|---|
| A 人体测量参数 | `ProportionProfile`、`Anthropometry` | 身高、头身比、肩髋比、胸腰比、肢段长度、围度等宏观比例 | 不直接描述曲面算法 |
| B 解剖标志点参数 | `AnatomicalLandmarks` | 关节中心、体表标志点、解剖曲线和截面锚点 | 不存储第二套骨骼编号 |
| C 局部形态参数 | `HumanShapeParameters` / `ShapeParameters` | 胸廓、腹部、臀部、三角肌、小腿、面颊、鼻梁、手掌等局部形态 | 不直接改变拓扑连接关系 |
| D 几何与拓扑表达 | `AnatomicalRegions`、`TopologyTemplate`、`SurfacePatch` | 区域邻接、稳定顶点 ID、边界环、patch 参数化、细分/采样策略 | 不改变人体语义参数 |
| E 姿势与形变表达 | `JointDeformationRegions` | 关节影响带、允许骨骼 mask、压缩/拉伸侧、corrective 入口 | 不新增骨骼，不替代 LBS |

`CharacterDefinition` 是上述五类的组合与版本化配置，不是把所有字段塞进一个扁平 `BodyParameters`。

## 3. 推荐数据模型

唯一语义权威是现有 `pae::shape::v2::CharacterDefinition`。本设计不新增第二个同职责的 `HumanShapeDefinition` struct；`HumanShapeParameters`、`Anthropometry`、`AsymmetryParameters` 和 `TopologyPolicy` 是它的组成子契约。`native/procedural/character/CharacterDefinition.h` 与 `BodyParameters` 仅作为 legacy patch/适配输入，不再是人体形态权威。

```cpp
// Extends pae::shape::v2::CharacterDefinition in place.
struct CharacterDefinition {
    uint32_t schemaVersion = 1;
    uint32_t generatorVersion = 1;
    uint64_t seed = 0;

    ProportionProfile proportions;
    Anthropometry anthropometry;          // 由 proportions 派生并可审计
    AnatomicalLandmarks landmarks;        // 由 proportions/joints/表面曲线解析
    AnatomicalRegions regions;
    HumanShapeParameters shape;
    AsymmetryParameters asymmetry;
    JointDeformationRegions jointDeformations;
    TopologyPolicy topology;
};
```

### 3.1 ProportionProfile

只保存稳定、可跨尺度复用的比例和人体测量关系：

- stature；
- headHeight / stature；
- shoulderBreadth / stature；
- biacromialBreadth / stature；
- hipBreadth / stature；
- chestDepth / stature；
- waistCircumference / stature；
- upperArm、forearm、thigh、lowerLeg、hand、foot 长度比例；
- 四肢长宽比和关节宽度比例。

默认体型属于 DEC-05 / Canonical Acceptance Sheet 的待批准内容。代码中的默认值必须带 `ValueConfidence`，不得把假设值标成 measured。

### 3.2 Anthropometry

保存绝对量和派生量，例如：

- shoulderBreadthM；
- chestDepthM；
- waistCircumferenceM；
- hipBreadthM；
- upperArmLengthM；
- forearmLengthM；
- handLengthM；
- footLengthM。

`Anthropometry` 由 `ProportionProfile` 解析产生。测试必须验证：只改变身高时，比例不变；只改变肩髋比时，只有声明区域和相邻边界受影响。

### 3.3 AnatomicalLandmarks

至少包含：

- 骨骼关节中心：hip、knee、ankle、shoulder、elbow、wrist、neck、head；
- 躯干表面点：jugular notch、sternal angle、xiphoid、costal margin、iliac crest、ASIS、umbilicus；
- 肩带点：acromion、clavicle、scapular medial border；
- 头部点：glabella、nasion、nose tip、alar base、columella、mouth corner、upper/lower vermilion border、gonion、chin；
- 手足点：MCP row、thenar、hypothenar、thumb CMC、metatarsal heads、hallux、calcaneus。

每个 landmark 记录 `name`、`kind`、`position`、`side`、`confidence`、`frameId`、`unit` 和可选的 `boneId`/`BodyJoints` 绑定。坐标遵循 ADR-002：右手系、米、+Y 上、+Z 角色正面、+X 角色左侧。resolver 优先级固定为：显式 landmark → 由 `BodyJoints` 派生 → 由 proportion/anthropometry 解析 → Undetermined；冲突时保留显式值并记录 confidence 降级，不得静默覆盖。`PoseFrame::PoseLandmark` 是相机/动作输入，不是这套建模 landmark，两者不得混用。

### 3.4 AnatomicalRegions

区域 ID 应覆盖：

`Thorax`、`Abdomen`、`Pelvis`、`ShoulderGirdle`、`Axilla`、`Neck`、`Head`、`Face`、`UpperArmL/R`、`ForearmL/R`、`HandL/R`、`ThighL/R`、`CalfL/R`、`FootL/R`。

每个区域记录：

- 主区域和相邻区域；
- 共享边界 ID；
- 允许的骨骼 mask；
- 默认局部形态参数集合；
- 是否为可见皮肤区域或辅助几何区域。

区域 ID 通过 sidecar metadata 贯通 Shape → Topology → Skinning，不借用 `Vertex::color.w`，因为该字段已承载 material ID。

### 3.5 HumanShapeParameters

按区域组合，而不是一个全局参数堆：

```cpp
struct HumanShapeParameters {
    TorsoShapeParameters torso;
    LimbShapeParameters limbs;
    HeadShapeParameters head;
    FaceShapeParameters face;
    HandShapeParameters hands;
    FootShapeParameters feet;
};
```

每类参数示例：

- Torso：ribcageVolume、chestDepthRatio、waistPinch、abdominalWall、pelvisVolume、glutealVolume、shoulderGirdleMass、axillaDepth；
- Limb：deltoidVolume、bicepsVolume、tricepsVolume、elbowWidth、forearmFlexorMass、wristTaper、thighVolume、quadricepsVolume、hamstringVolume、kneeWidth、calfVolume、ankleTaper；
- Head：craniumVolume、faceDepth、jawWidth、chinProjection、earProjection；
- Face：browProjection、malarVolume、noseBridge、noseTip、alarWidth、philtrumDepth、vermilionThickness、mouthCorner；
- Hand：palmWidth、palmLength、palmarArch、thenarVolume、hypothenarVolume、digitLengths[5]、digitRadii[5]；
- Foot：heelWidth、archHeight、forefootWidth、halluxSize、toeLengths[5]。

每个参数必须声明：

```cpp
struct ShapeParameterDecl {
    std::string id;
    std::string unit;
    std::string semantic;
    float minValue;
    float maxValue;
    float defaultValue;
    ValueConfidence confidence;
    std::vector<RegionId> affectedRegions;
    ShapeBasisId basis;
    ParameterStatus status;
};
```

### 3.6 AsymmetryParameters

- 左右肩高差；
- 左右手臂长度差；
- 左右腿长差；
- 面部左右局部形态差；
- 左右手掌/足部尺寸差。

默认应为轻微、自然的差异，而不是强制完全镜像。所有差值必须限制在人体测量合理范围内，并由参数范围约束。

### 3.7 JointDeformationRegions

每个关节区域记录：

- `boneId`：仍为 0–21；
- `center`、`axis`；
- `influenceBandM`；
- `allowedBoneMask`；
- `compressSide` / `stretchSide`；
- `volumePreserving` 标志；
- `correctiveBasisId` 入口。

VR-1 只使用该结构约束 LBS 权重过渡，不实现完整 pose corrective。VR-4 才评估是否加入 CPU 端 corrective；任何 GPU 数据变化都必须保持现有 ABI。

## 4. 数学表达

### 4.1 躯干与四肢可变截面

采用负责人给出的构造：

\[
P(s,\phi)=C(s)+r_1(s,\phi)e_1(s)+r_2(s,\phi)e_2(s)
\]

其中：

- \(C(s)\)：由解剖标志点/曲线产生的中心线；
- \(e_1,e_2\)：平行传输或稳定解析截面坐标系；
- \(r_1,r_2\)：非圆、宽深独立、随 \(s\) 和 \(\phi\) 变化的截面半径。

局部解剖形态叠加：

\[
P'=P+\sum_i a_i\phi_i(s,\phi)n(s,\phi)
\]

\(\phi_i\) 是局部影响函数，例如胸肌、三角肌、臀大肌、股四头肌、腓肠肌、颧骨和面颊。影响函数必须满足：

- 有限支撑域；
- 边界处值和一阶导数连续；
- 参数单一语义；
- 不影响未声明区域；
- 在共享边界上使用同一求值器，避免 seam 两侧漂移。

### 4.2 局部形态基函数

PAE 自己实现三类基函数：

1. **Profile basis**：沿中心线或解剖曲线改变宽深、中心偏移和截面指数。
2. **Local relief basis**：高斯/紧支撑 RBF 或样条影响域，在法线方向产生局部隆起或凹陷。
3. **Concavity basis**：腋窝、腹股沟、臀沟、肘窝、腘窝、鼻孔和口角的负向局部修正。

不导入 MakeHuman `.target`、SMPL-X/FLAME/MANO 模型或任何外部顶点 delta。若未来使用自建形态基，必须绑定 PAE 自己的稳定 topology template ID 和 local vertex ID。

### 4.3 面部、手部、足部

这三类不能仅靠截面公式：

- 面部：固定语义控制网格 + 眼眶/眼睑/鼻/唇/下颌局部 patch；
- 手部：掌弓 + thenar/hypothenar + MCP 行 + 五个独立 digit 模板；
- 足部：heel/arch/metatarsal/forefoot + hallux + 四个 toe 模板。

它们使用 `SurfacePatch` 和 `TopologyTemplate`，但仍输出到同一个 Canonical 组装流程。

## 5. 与现有代码的关系

### 5.1 保留

- `native/procedural/shape/v2/CharacterDefinition.h/.cpp` 的五层语义和 `ValueConfidence`；
- `native/procedural/shape/v2/SharedSkinEnvelope.cpp` 的 C2 比例曲线、区域 partition-of-unity 和凹面约束思想；
- `native/procedural/character/BodyPatchGenerator.cpp` 的非圆截面、平行传输 frame、肩胸 bridge 和共享索引经验；
- `CanonicalSkinGenerator` 的边界环、重采样、权重合并、Render Mesh 映射；
- 现有 `Skeleton`、`Skinning`、`SkinnedGpuMesh`、UV VBO 和 RenderThread 数据路径。

### 5.2 重构

- 唯一人体语义权威是 `pae::shape::v2::CharacterDefinition`；在此基础上升级 schema，不新增并行 `HumanShapeDefinition`。
- 生产 Canonical 不再把默认 `BodyParameters` 作为唯一形态输入。
- `BodyParameters` 与 `native/procedural/character/CharacterDefinition.h` 仅作为 legacy patch/适配输入，不再承载第二套人体语义。
- `ContinuousSkinGenerator` 从“全身 SDF 生成器”变为“Canonical 组装编排器”。
- `assignCanonicalWeights` 增加 region metadata 和 joint-aware falloff。
- `CanonicalSkinGenerator` 的现成 weld helper 只接受等密度边界且容差为 `1e-3 m`，不满足新合同；VR-1B 前必须改为共享边界求值、非等密度重采样、切线元数据和 `1e-4 m` C0 检查。

### 5.3 新增

- `Anthropometry`、`AsymmetryParameters`、`TopologyPolicy`；
- 完整的 head/face/hand/foot region 和 local shape 参数；
- `SurfacePatch`、`TopologyTemplate`、`LocalShapeBasis` 的生产抽象；
- Canonical vertex sidecar metadata。

## 6. 参数验证与可追溯性

每个参数组必须测试：

- 参数变化产生可测 mesh delta；
- delta 只出现在声明区域及必要边界过渡带；
- 左右参数可以独立变化；
- 身高缩放不改变比例；
- 所有值有限、在范围内、置信度明确；
- 同一 `CharacterDefinition` 在 Host 与 Android 生成相同 Canonical hash；
- topology template ID 和 local vertex ID 在参数变化时保持稳定。
- 区分持久 `stableId` 与单次构建的 Canonical index；跨 detail level 必须通过 template version + local ID 重映射，禁止把数组下标当作长期 ID。
- landmark 的 frame/unit/side/BoneId 绑定和 resolver 优先级有确定性测试。

## 7. 明确边界

- 本设计不批准默认体型、最终审美阈值或任何 Gate PASS。
- 不导入外部人体模型、形态基底、训练数据或运行时。
- 不新增手指/脚趾骨骼；VR-3 的 digit 形态进入 Hand/Foot/Toe 权重，个别手指关节运动留待后续 corrective/拓扑扩展。
- 不改变 `MeshData::Vertex` 的 64-byte GPU 语义；region/topology metadata 使用 CPU sidecar。
- 不在 VR-1 实现完整 pose corrective 或表情系统。

## 8. 待负责人确认项

1. 默认成人体型和 Canonical Acceptance Sheet 的数值来源。
2. P1-VR 补充授权书的正式签发与白名单。
3. 可接受的左右不对称幅度。
4. VR-1 的临时性能 stop guardrail。
5. 眼部辅助几何是否允许作为非 Skin Shell 组件；默认设计保持 Skin Shell 封闭，眼球/虹膜作为辅助几何。
