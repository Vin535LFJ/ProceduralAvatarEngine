# Procedural Character Foundation Specification

**中文名称：程序化角色基础技术设计规范**  
**版本：v1.0**  
**状态：开发基线候选（供项目负责人确认后执行）**  
**适用项目：** [Vin535LFJ/ProceduralAvatarEngine](https://github.com/Vin535LFJ/ProceduralAvatarEngine)  
**仓库审查基线：** `main` @ `c539afbec299ed4a066429ea09f9d3d36e67f74d`（2026-11-22）  
**核心路线：** Saltline-inspired Procedural Asset Generation + Rig-aware Character Modeling + Modular Avatar Runtime  

---

## 0. 本规范的目的与边界

本规范定义 ProceduralAvatarEngine 下一阶段的**最小、可验证、可逐步扩展的角色生产基础**。它不是通用游戏引擎规范，也不是完整 3D 建模软件规范。

本项目的核心目标是：

> 在不依赖外部 3D 美术模型、贴图和动画文件的前提下，使用 C++ 程序化几何、拓扑、骨骼、蒙皮和 Shader，生成一个比例协调、视觉上可信的风格化 3D 卡通人物；该人物从第一版起就可由标准 Humanoid Skeleton 驱动，并能在 Android OpenGL ES 3.0 上运行。

之后在此基础上逐步增加模块化换装、外观配置、实时姿态跟随和视频回放。

### 0.1 术语边界

| 术语 | 本项目中的含义 |
|---|---|
| Procedural Asset Generation | 通过代码、数学函数、参数和规则生成几何、拓扑、材质与角色部件，而不是导入外部美术资产 |
| Rig-aware Modeling | 在生成几何前就确定骨骼、关节位置、Bind Pose 和变形区域，几何与蒙皮共同设计 |
| Character Mesh | 能够表达角色外观并满足后续骨骼变形要求的网格，不等同于任意连通三角面集合 |
| Topology Template | 由代码定义的稳定顶点/边/面连接规则；参数改变顶点位置，但不随意改变主要拓扑流向 |
| Modular Avatar | 将 Body、Head、Hair、Top、Bottom、Shoes 等角色组件按统一骨骼和接口组合的运行时角色 |
| SDF | 辅助表达复杂形态、局部融合或形态 PoC 的工具；不是 P1 最终动画人体网格的唯一生产方式 |
| P1 | 第一个完整、可程序化生成、可骨骼驱动并能在 Android 上渲染的基础卡通角色 |

### 0.2 本规范不承诺的事项

P1 不以照片级写实、Pixar 级角色质量、完整医学解剖精度或通用自动重拓扑为验收目标。P1 的目标是**基础可用的风格化卡通角色**：人体比例协调、主要曲面自然、没有明显球体/圆柱体拼装感、能完成基础动作且无严重关节塌陷。

更高视觉品质通过 P2/P3 的迭代提升，不得把“用了某种高级算法”当作视觉质量达标的证据。

---

## 1. 已确认的产品与工程约束

### 1.1 产品演进顺序

1. P0：审查现有工程，冻结坐标、骨骼、几何、蒙皮和验收契约。
2. P1：生成第一个完整的可驱动 3D 卡通人，使用程序化动作验证，不接入 ML Kit。
3. P2：生成独立服装几何，实现真正的几何换装。
4. P3：提升角色美术质量，包括面部、发型、服装细节、材质和背景。
5. P4：接入 ML Kit / 可替换 Pose Provider，实现实时人体跟随。
6. P5：本地视频回放、时间戳同步、设备优化和 MVP 收尾。

### 1.2 不可违反的技术约束

- 核心：C++17。
- Android：Kotlin + JNI + Android NDK。
- 渲染：EGL + OpenGL ES 3.0 + GLSL ES 3.00。
- 保留现有自研 Renderer，不引入 Unity、Unreal、Godot、Filament 等完整引擎。
- 不依赖 Blender、Maya、ZBrush 等 DCC 软件导出的运行时模型。
- 不导入 GLB/GLTF、FBX、OBJ 等外部角色模型。
- 不依赖外部人体、服装、头发、场景模型或贴图。
- 允许使用姿态识别 SDK（例如 ML Kit）作为后续输入能力；算法 SDK 不属于外部 3D 美术资产。
- 允许参考公开图形学算法和论文，但算法必须通过项目自己的 C++ 实现与测试落地。
- 允许将程序化生成的网格缓存在自定义格式中；缓存不是外部美术源资产，生成规则和参数仍是权威来源。

### 1.3 目标设备

目标包括 Android 手机、Android TV 和低性能电视盒子。最低设备能力必须由项目负责人提供的目标设备清单确认。当前已知项目历史测试设备包括 Android 8.1、arm64-v8a、GLES 3.0 的电视盒子；该历史设备记录不能自动代表所有目标设备。

P1 以单角色、单相机、简单场景为范围。30 FPS 是运行目标，不是未实测即可宣称的保证。

---

## 2. Saltline 思想在本项目中的正确落点

本项目采用 Saltline 的方法论，而不是照搬其机械角色实现：

- 资产的源头是代码、参数和生成规则。
- 不把外部模型文件作为角色生产依赖。
- 不同结构使用不同的专用生成函数，而不是用一种几何原语解释所有形态。
- 生成结果必须进入真实渲染器，并通过截图和运行数据持续迭代。
- 几何生成、材质生成、场景生成是独立但可组合的子系统。

人体与 Saltline 的机械角色不同：人体需要有机曲面、连续的关节过渡和可变形拓扑。因此，刚性部件绑定或简单 Capsule/Sphere 拼装不能作为正式人体方案。

**本项目的核心原则：** Saltline-inspired code-generated assets + anatomy-aware surface construction + rig-aware topology + modular assembly。

---

## 3. 目标架构

```text
Character Definition / Parameters
                |
                v
Procedural Character Generator
  |             |              |
  v             v              v
Body Surface  Head/Face      Garment (P2)
Generator     Generator      Generator
  |             |              |
  +-------------+--------------+
                |
                v
Topology Template + Mesh Builder
                |
                v
Mesh Validation / UV / Normals / Materials
                |
                v
Rig Definition + Bind Pose + Skin Weights
                |
                v
Avatar Components / Modular Avatar Runtime
                |
                v
Animation Runtime (procedural poses in P1)
                |
                v
OpenGL ES 3.0 Renderer
                |
                v
Android Display

P4/P5 input path:
Camera / Video -> Pose Provider -> Coordinate Adapter
 -> Retargeting / IK / Filtering -> Animation Runtime
```

### 3.1 层级职责

| 层 | 职责 | 禁止事项 |
|---|---|---|
| Character Definition | 描述角色参数、比例、风格和组件选择 | 不直接存储顶点或 GPU 句柄 |
| Procedural Generator | 按参数生成几何与拓扑 | 不依赖 UI 或 EGL 状态 |
| Mesh/Topology Core | 管理顶点、索引、拓扑、法线、UV、子网格 | 不承担角色风格决策 |
| Rig/Skinning | 骨骼层级、Bind Pose、权重和变形 | 不读取相机关键点 |
| Avatar Runtime | 组合组件、换装、外观状态、组件生命周期 | 不生成整套通用游戏逻辑 |
| Animation Runtime | 接收动作目标并更新骨骼姿态 | P1 不接入 ML Kit |
| Renderer | 上传 GPU Buffer、Shader、渲染和相机 | 不负责人体几何算法 |
| Pose Provider | P4/P5 提供统一 PoseFrame | 不直接修改 Mesh 顶点 |

### 3.2 现有工程保留策略

优先审计并复用：

- Android Kotlin Host、JNI、EGL、GLES Renderer。
- `core/math` 数学基础。
- `MeshData`、`Vertex`、`SubMesh` 等数据结构。
- `Skeleton`、`computePose`、`BonePaletteUboData` 和 GPU LBS 基础管线。
- `ParametricSurfaces`、`CatmullRom`、SDF/Surface Nets 作为可复用工具。
- Camera、Video、Pose Provider、Motion Runtime 和统计模块，留待 P4/P5 使用。

M12 的 `HumanAnatomyGenerator` 及 Surface Nets 人体原型保留为研究/对照实现，不作为 P1 正式角色的唯一生产路径。

禁止为启动新路线而重写整个 Android 工程。任何公共接口变化必须有 ADR、迁移说明和回归测试。

---

## 4. 几何生成主线：有限技术集，不建设迷你 Blender

### 4.1 P1 主路线

P1 采用：

**Rig-aware Anatomical Landmarks + Procedural Surface Patches + Topology Templates + Variable-profile Sweeps**。

SDF 作为辅助工具，仅用于：

- 局部形态实验和融合 PoC。
- 复杂轮廓的辅助场表达。
- 离线/初始化阶段的局部修正研究。

P1 不要求开发通用 SDF 雕刻器、通用自动 Quad Remesher 或完整 NURBS 编辑系统。P1 不将 Surface Nets 输出直接视为最终动画人体网格。

### 4.2 几何主干

| 区域 | P1 生成方法 | 形态要求 |
|---|---|---|
| 躯干 | 由解剖控制点驱动的多段 Surface Patch / Profile Loft | 胸腔、腹部、腰部、背部、骨盆轮廓可独立控制；前后不能只是同一个椭圆截面的镜像 |
| 肩胸连接 | 专用 Shoulder/Chest Patch，接入躯干与上臂的拓扑边界 | 不使用一个独立球体遮住连接处；肩峰、三角肌体积、腋下过渡有明确参数 |
| 上臂/前臂 | 多点中心曲线 + 非圆形截面 Sweep | 截面宽度、深度、旋转、曲率和体积沿长度变化；不得只用三点圆管 |
| 肘部 | Joint Topology Patch | 具有环线和足够弯曲分段，避免单球关节 |
| 骨盆/髋部 | 独立参数化曲面 Patch + 双侧髋部连接 | 腰臀过渡和前后厚度可控，髋部拓扑面向腿部变形 |
| 大腿/小腿 | 多截面非圆形 Sweep + 膝部 Patch | 大腿体积、膝部过渡、小腿曲线、踝部收缩可控 |
| 手掌/足部 | 参数化轮廓与局部曲面 | P1 可采用简化卡通手掌/鞋形，但必须可辨识并可随骨骼驱动 |
| 头颈 | 参数化头部轮廓曲面 + 颈部连接 | 头部不能只是放大椭球；P1 基础五官至少具备完整位置关系 |

### 4.3 Surface Patch 与 Topology Template

P1 应优先采用由代码定义的稳定拓扑模板，而非在任意三角网格上尝试自动重拓扑。

每个模板定义：

- 顶点参数域和边界环。
- 环/行/列分段数量。
- 面片连接关系。
- 关节附近边线流向。
- UV 参数域（需要时）。
- 语义区域标签。

参数变化只改变顶点位置和局部曲率，不随意改变骨骼关节区域的主要拓扑结构。

Patch 连接必须采用明确的边界契约：边界顶点数量、顺序、位置和法线约定一致。不能仅通过两个 MeshData 的顶点/索引追加声称表面连续。

对于眼球、牙齿、眉毛、发束等合理独立部件，允许作为独立 Mesh Component；不要求整个角色所有部件形成一个水密网格。

### 4.4 可变截面 Sweep

Sweep 必须支持：

- 至少多个控制环，而非固定三点中心线。
- 非圆形截面轮廓（例如由二维闭合样条定义）。
- 两个主轴独立缩放。
- 截面旋转/roll。
- 稳定的 Parallel Transport Frame。
- 沿曲线的自适应采样。
- 截面插值和端部过渡。
- 正确考虑半径变化的法线计算。

### 4.5 曲面品质与细分

P1 先保证控制曲线/曲面表达质量和拓扑合理性。细分算法只能平滑已经合理的基础曲面，不能用高面数掩盖不正确的形体。

Catmull-Clark、Loop Subdivision 等可先做独立 PoC，只有在基础网格拓扑适配、法线和边界规则明确后才纳入主生产流程。

### 4.6 形态参数必须有实际作用

所有进入 `CharacterDefinition` 的参数必须满足：

1. 经过范围校验。
2. 映射到明确的控制点、曲线或截面。
3. 通过测试证明参数变化会导致预期部位的形体变化。
4. 不得存在只声明、不参与生成的参数。

参数组至少包括：身高、头身比、肩宽、胸宽/厚度、腰宽/厚度、骨盆宽度、上/下臂长度、前臂轮廓、髋宽、大腿/小腿长度、脚长、卡通化比例。

---

## 5. 统一坐标、单位与姿态契约

### 5.1 既有坐标决策

沿用仓库 ADR-002：

- 右手坐标系（Right-handed）。
- +Y 为上。
- +Z 为角色前方。
- +X 为角色左侧（角色自身左右，不等同于屏幕观察者左右）。
- 长度单位为米。
- 角度单位为弧度。
- 矩阵列主序，按现有 `Math.h` 约定使用矩阵乘向量 `M * v`。
- 父子变换按 `ParentGlobal * Local`。
- 时间戳使用纳秒。

如果审查发现现有 `Math.h`、GLSL 或 JNI 数据实际不符合 ADR-002，必须先记录差异并用 ADR 修复，不得静默混用。

### 5.2 Bind Pose

标准绑定姿态采用 **A-Pose**，但必须用明确的关节点坐标定义，而不是只用“A-Pose”文字描述。

- 双臂从躯干向两侧下斜，建议相对水平线约 30–45 度作为初始设计范围；最终由固定角色视觉模板确定。
- 肘部轻微自然弯曲，左右保持对称。
- 手掌方向、拇指方向和前臂扭转必须记录。
- 双腿自然分开，膝盖不锁死，脚掌朝前。
- 头部处于中立位置。

Bind Pose 与日常 Idle Pose 必须分开。Idle Pose 可以更自然地垂臂，但不得更改 Inverse Bind Matrices。

### 5.3 关节点来源

先生成 `RigDefinition/BodyLandmarks`，几何生成器从同一份数据读取关节点，禁止几何和 Skeleton 分别独立估算同一关节位置。

关键点至少包括：Hips、Spine、Chest、Neck、Head、左右 Shoulder、Elbow、Wrist、Hand、Hip、Knee、Ankle、Foot/Toe。

---

## 6. Humanoid Skeleton ABI 契约

### 6.1 固定 BoneId

现有 `BoneId` 0–21 是当前项目的 ABI，P0/P1 不得无理由重排：

| ID | 名称 |
|---:|---|
| 0 | Root |
| 1 | Hips |
| 2 | Spine |
| 3 | Chest |
| 4 | Neck |
| 5 | Head |
| 6 | LeftShoulder |
| 7 | LeftUpperArm |
| 8 | LeftLowerArm |
| 9 | LeftHand |
| 10 | RightShoulder |
| 11 | RightUpperArm |
| 12 | RightLowerArm |
| 13 | RightHand |
| 14 | LeftUpperLeg |
| 15 | LeftLowerLeg |
| 16 | LeftFoot |
| 17 | LeftToe |
| 18 | RightUpperLeg |
| 19 | RightLowerLeg |
| 20 | RightFoot |
| 21 | RightToe |

新增骨骼必须另做 ABI/UBO 容量评估与 ADR，不得在 P1 随意扩展。

### 6.2 父子层级目标

```text
Root
└── Hips
    ├── Spine
    │   └── Chest
    │       ├── Neck
    │       │   └── Head
    │       ├── LeftShoulder
    │       │   └── LeftUpperArm
    │       │       └── LeftLowerArm
    │       │           └── LeftHand
    │       └── RightShoulder
    │           └── RightUpperArm
    │               └── RightLowerArm
    │                   └── RightHand
    ├── LeftUpperLeg
    │   └── LeftLowerLeg
    │       └── LeftFoot
    │           └── LeftToe
    └── RightUpperLeg
        └── RightLowerLeg
            └── RightFoot
                └── RightToe
```

### 6.3 必须审计的当前骨骼语义重叠

当前 `Skeleton.cpp` 中，LeftShoulder 的 head/tail 与 LeftUpperArm 的 head/tail 相同，右侧亦然；这使 Shoulder 与 UpperArm 的语义重叠。

P0 必须检查所有动画、权重、调试和重定向调用者，然后通过 ADR 明确修正策略。建议语义为：Shoulder 是 Chest 到 Shoulder landmark 的短过渡/辅助骨，UpperArm 是 Shoulder 到 Elbow 的主骨。**保持 BoneId 不变，但允许修正 BoneDefinition 的 head/tail 与 parent 语义；修正前必须有回归测试。**

### 6.4 矩阵和蒙皮公式

约定：

- `GlobalPose[i]`：当前姿态下骨骼全局矩阵。
- `InverseBind[i]`：Bind Pose 全局矩阵的逆。
- `SkinPalette[i] = GlobalPose[i] * InverseBind[i]`。
- 顶点位置按 `sum(weight[j] * SkinPalette[bone[j]] * bindPosition)` 计算。

实际实现必须与现有 `computePose`、CPU 参考蒙皮和 GLSL 顺序一致，并用一个已知旋转案例证明矩阵方向正确。

---

## 7. MeshData 与组件契约

### 7.1 当前 MeshData 基础

当前 `Vertex` 包含：Position、Normal、UV、Joints、Weights、Color；`MeshData` 包含 vertices、uint32 indices、subMeshes、bounds。现有 GPU 上传布局为 64-byte stride，当前上传路径并未完整使用 UV 属性。

P0 必须审计该布局；P1 不需要为了“看起来完整”而盲目改动所有顶点格式。需要 UV 或 Tangent 时，必须同时更新 CPU 结构、GPU packing、VAO layout、Shader 和测试。

### 7.2 Mesh Component

角色由多个合理的 Mesh Component 组成：

- BodySurface：身体主表面。
- Head/Face Features：眼球、牙齿等可作为独立组件。
- Hair：后续独立组件。
- Garments：P2 独立服装组件。
- Shoes：P2 独立鞋组件。

不要求所有角色部件合并成一个水密连通网格。验收重点是可见接缝自然、边界位置对齐、组件绑定一致、没有穿模和裂缝。

### 7.3 Mesh Validation

至少检查：

- 索引范围与三角形数量。
- NaN/Infinity。
- 退化三角形。
- 未引用顶点。
- 重复顶点（按容差，而非只用浮点精确相等）。
- 法线长度与方向。
- winding 一致性。
- 边界边、非流形边和连通分量（按 Mesh Component 的预期拓扑判断）。
- AABB。
- UV 合法性（启用时）。
- 骨骼索引与权重合法性。

**不得把所有 Mesh Component 都要求为一个整体单连通水密网格。** 眼球、牙齿、头发和服装等可以合理独立。

---

## 8. Skinning Contract

### 8.1 P1 算法选择

P1 默认采用 Linear Blend Skinning（LBS），最多 4 个骨骼影响，使用现有 GLES 3.0 std140 Bone Palette UBO 路线。DQS、Corrective Shapes、肌肉模拟暂不作为 P1 阻塞项。

### 8.2 权重生成

采用 Semantic Region Mask + Bone Segment Distance 的组合：

1. 先根据顶点语义区域限定允许影响的骨骼集合。
2. 再计算候选骨骼对顶点的距离/影响函数。
3. 进行平滑过渡。
4. 取最多 4 个影响。
5. 归一化权重。
6. 检查骨骼索引与数值合法性。

建议区域与骨骼集合：

| 区域 | 主要影响骨骼 |
|---|---|
| 胸腔 | Spine、Chest |
| 腰/躯干 | Hips、Spine、Chest |
| 肩部 | Chest、Shoulder、UpperArm |
| 上臂 | UpperArm、LowerArm |
| 肘部 | UpperArm、LowerArm |
| 前臂 | LowerArm、Hand |
| 骨盆/髋 | Hips、UpperLeg |
| 大腿 | UpperLeg、LowerLeg |
| 膝部 | UpperLeg、LowerLeg |
| 小腿 | LowerLeg、Foot |
| 脚掌 | Foot、Toe |
| 头部 | Head；颈部边界允许 Neck 混合 |

上述集合是候选限制，不意味着每个区域所有骨骼都必须获得非零权重。

### 8.3 必须提供 Debug View

- Bone Influence Heatmap：显示主要骨骼影响。
- Weight Sum Error：标记权重和不等于 1 的顶点。
- Bone Index Error：标记非法索引。
- Deformation Comparison：Bind Pose 与当前 Pose 对照。

### 8.4 动作变形测试

P1 至少有确定性的程序化姿态：

- Idle/A-Pose。
- 双臂前举。
- 肩外展/内收。
- 肘部弯曲 45°/90°/接近上限。
- 前臂旋转（如果当前骨骼结构支持）。
- 躯干前屈/侧弯/旋转。
- 髋关节屈曲。
- 膝关节弯曲。
- 下蹲。
- 单腿抬起。

逐项记录最大允许角度和预期效果。不能只用一个“整个人动了一下”的截图验收。

---

## 9. Modular Avatar 与换装预留

P1 先完成一个完整基础人物及一个默认外观，不强制在 P1 完成多套换装。P1 必须预留以下组件接缝契约，P2 才实现多套服装：

- Neck seam。
- Shoulder/armhole seam。
- Wrist/cuff seam。
- Waist seam。
- Hip/crotch seam。
- Ankle/shoe seam。

接缝契约应包括：局部坐标、边界环顶点数与顺序、骨骼影响、组件遮挡方向和最小间隙规则。

服装网格有独立顶点和蒙皮权重，但引用同一个 Skeleton。不得假设服装直接复用人体的顶点或权重数组。

换装时按组件替换/隐藏，不重建整个 Avatar；改变肤色只更新材质参数；改变体型才重建受影响几何。

---

## 10. Character Definition 与缓存策略

### 10.1 参数定义

建立版本化的 `CharacterDefinition`（可以先使用 C++ struct，JSON 调试导入/导出后续再做）。最小字段：

- definitionVersion。
- generatorVersion。
- rigVersion。
- body proportions。
- anatomy/style parameters。
- component selections。
- material parameters。
- deterministic seed。
- detail level。

每个参数必须关联具体生成逻辑和验证用例。

### 10.2 `.pmesh` 的时机

`.pmesh` 缓存格式**不属于 P0/P1 的前置任务**。P1 先生成 Runtime MeshData 并上传 GPU。

当 P1 角色网格结构、顶点布局和骨骼协议稳定后，在 P2/P3 再设计缓存格式。届时至少包括 magic、version、coordinate convention、vertex/index counts、vertex streams、submeshes/material IDs、skeleton/rig reference、bounds、checksum 和数据块长度。

生成规则和参数是权威源；缓存可以删除并重新生成。

---

## 11. Renderer 与 Android 约束

- GLES 3.0 是正式目标，不以 PC 高版本 OpenGL 预览替代。
- Shader 使用 GLSL ES 3.00。
- GPU 资源创建/上传必须在具有 current EGL Context 的 Render Thread 执行。
- EGL Surface/Context 重建后能重新上传运行时资源。
- 角色几何在初始化或参数变更时生成，不能每帧重新生成。
- P1 简化灯光和材质，使用平滑灰模光照先验收几何；不要用分段 Toon 阴影掩盖曲面缺陷。
- P1 支持正面、侧面、背面、45°、局部肩/髋/肘/膝视角；相机参数固定并可重复采集。
- Android 真机为最终视觉验收平台。PC 端只可用于辅助数学测试和快速预览。

---

## 12. 视觉目标与验收规则

### 12.1 P1 风格目标

P1 采用**自然比例的风格化卡通人（Stylized Cartoon Human, Level 1）**：

- 明确的人体轮廓和头身比例。
- 胸腔、腰部、骨盆、肩部、上/下肢具有自然过渡。
- 头部和基础五官完整可辨。
- 手掌和脚部可辨识。
- 一个简单、完整、协调的默认外观。
- 无明显椭球/圆柱/球体拼装感。

不复制用户提供的商业角色具体设计、面部身份或模型资产；参考图片只用于描述风格和形体质量，不得作为运行时纹理或几何输入。

### 12.2 固定证据协议

所有比较截图必须固定：分辨率、设备、相机位置、投影/FOV、人物缩放、Pose、光照、材质、背景和渲染质量。

至少输出：

- 正面、背面、左侧、右侧、45° 左、45° 右。
- 肩部、肘部、髋部、膝部特写。
- 线框视图。
- 法线视图。
- 骨骼显示。
- 权重热力图。
- 程序化动作截图序列。

灰模几何验收使用连续 Lambert 光照，不使用 6 级 Toon 作为唯一质量证据。

### 12.3 质量门禁

| Gate | 验收对象 | 不通过时 |
|---|---|---|
| G0 | 坐标/骨骼/网格/权重契约明确，现有代码审计完成 | 不进入 P1 几何生产 |
| G1 | 完整人体静态形态、比例、表面质量、多视角 | 不进入骨骼变形完善 |
| G2 | 骨骼驱动、权重、关键动作变形 | 不进入 P2 换装 |
| G3 | Android GLES 真机运行和性能基线 | 不宣布 P1 完成 |
| G4 | P2 换装接缝与动作跟随 | 不进入大规模服装扩展 |

AI 可以给出自评和缺陷清单，但不能代替用户做视觉 Gate 通过决定。

### 12.4 P1 通过条件

P1 必须同时满足：

1. 完整人体在正面/侧面/背面/45° 观察均具备合理轮廓。
2. 主要身体区域没有明显几何拼装感、裂缝、错误法线或严重穿插。
3. A-Pose/Idle 与关键程序化动作均能稳定渲染。
4. 肩、肘、髋、膝等关键部位无严重塌陷、撕裂或异常拉伸。
5. 所有权重和骨骼索引合法。
6. Android GLES 3.0 真机运行成功。
7. 提交实际构建/测试日志、截图和性能数据。
8. 用户确认 G1/G2/G3 视觉与产品验收。

---

## 13. 性能预算与测量

P1 目标是单角色、简单环境，渲染目标 30 FPS。性能结论必须来自目标设备实测。

记录：

- Geometry Generation Time（冷/热）。
- Vertices / Triangles / Draw Calls。
- CPU mesh memory。
- GPU buffer memory。
- APK 增量。
- Render FPS。
- Frame time P50/P95/P99。
- CPU/GPU load（设备可提供时）。
- Native heap。
- Surface/Context 重建后的资源恢复情况。

暂定工程预算（验证用，不是硬性画质上限）：Medium 角色约 10k–25k triangles；如需超出，必须说明视觉收益、GPU/内存数据和降级策略。不得为了满足数字而降低人体形态质量。

首要优化顺序：避免重复生成、缓存 CPU MeshData、复用 GPU Buffer、降低不必要 Draw Calls、减少材质切换、再考虑 LOD。不得先删掉关键人体形态特征来伪造性能提升。

---

## 14. 明确延期的技术

以下内容不作为 P0/P1 的阻塞任务：

- 通用 Quad Remeshing。
- 全自动人体 Retopology。
- 完整 NURBS 编辑器。
- 通用 SDF Sculpting 工具。
- DQS。
- Corrective Shape 系统。
- 肌肉/皮肤物理模拟。
- 布料物理模拟。
- 多角色动画状态机。
- 高级 PBR/SSS/全局光照。
- `.pmesh` 最终文件格式。
- 实时 ML Kit 跟随。

只有当已验收的 P1 功能出现明确缺陷，且简单方法无法解决时，才提出对应升级，并通过 ADR 说明收益、成本和替代方案。

---

## 15. 设计决策记录（ADR）要求

至少维护以下 ADR：

- 坐标系与单位（沿用 ADR-002，审计验证）。
- Humanoid Skeleton ABI 与 A-Pose 定义。
- 几何主线选择：Topology Template + Surface Patch + Variable-profile Sweep。
- SDF 的辅助定位和边界。
- Mesh Component / seam 规则。
- LBS / UBO 兼容策略。
- 缓存格式推迟决策。

每项 ADR 包含：背景、候选方案、决定、理由、影响、验证证据、回滚方法。

---

## 16. 当前仓库基线事实（用于实施前核对）

以下基于 `main` @ `c539afbec299ed4a066429ea09f9d3d36e67f74d` 的代码审查，AI 必须在执行时重新确认：

- `Skeleton.h` 当前定义 22 根 BoneId（0–21）。
- `Skeleton.cpp` 当前肩部骨与 UpperArm 的 head/tail 语义存在重叠，需要 P0 审计和 ADR 修正。
- `MeshData.h` 当前 Vertex 包含 position、normal、uv、joints、weights、color；MeshData 包含 vertices、uint32 indices、subMeshes、bounds。
- `Skinning.h` 已有 PoseResult、computePose、CPU 参考蒙皮和 BonePalette UBO 数据结构。
- `RenderThread` 已有 GLES Renderer、SkinnedGpuMesh、BonePaletteUbo 和 M12 anatomy preview。
- `SkinnedGpuMesh.cpp` 当前 GPU packing 为 64-byte stride，现有打包路径未上传 UV；如后续材质需要 UV，必须同步修改 GPU layout 与 Shader。
- `Humanoid.h/.cpp` 已有 BodyParameters、BodyJoints、旧人体生成逻辑及部分参数化曲面工具。
- M12 的统一 SDF 上半身是可保留的原型，不代表完整角色生产系统。

仓库基线可能继续变化。AI 必须获取最新 `main` SHA，不能盲目覆盖用户提交。

---

## 17. 规范生效原则

本规范约束 P0/P1 的技术方向，但不要求 AI 在未验证的情况下机械照搬每个算法细节。如果发现现有工程事实与本规范冲突，必须先提交差异、影响和 ADR 建议；未经说明不得悄悄换方案。

所有视觉和性能结论必须由真实生成代码、真实 Android 渲染和真实测试证据支持。
