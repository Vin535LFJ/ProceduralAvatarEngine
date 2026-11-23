# Procedural Character Development Roadmap

**中文名称：程序化角色开发指导计划**  
**版本：v1.0**  
**状态：开发基线候选**  
**关联规范：** `01_Procedural_Character_Foundation_Specification_v1.0.md`  
**适用仓库：** `Vin535LFJ/ProceduralAvatarEngine`  

---

## 0. 路线总览

本计划不再以“实现多少个几何算法模块”作为进度指标，而以“是否逐步交付一个可驱动、可换装、可实时跟随的程序化卡通数字人”作为项目指标。

```text
P0 需求/代码审计/技术契约
  |
  v
P1-A 几何主线 PoC 与视觉 Gate
  |
  v
P1-B 完整基础人体生成
  |
  v
P1-C 骨骼、蒙皮、程序化动作
  |
  v
P1-D Android 集成与 P1 验收
  |
  v
P2 Modular Clothing / Appearance
  |
  v
P3 Character Visual Polish
  |
  v
P4 ML Kit / Pose Provider 实时驱动
  |
  v
P5 Video Playback / Device Optimization / MVP
```

### 产品交付顺序

1. 先完成一个完整静态 3D 卡通人物。
2. 让同一个人物能够由统一骨骼驱动。
3. 实现真实几何换装。
4. 提升面部、头发、服装、材质和场景质量。
5. 最后接入真实人体姿态识别与实时跟随。

### 项目原则

- Saltline 的代码生成资产理念是底层生产方法。
- Modular Avatar 是角色组合和换装层，不取代 Procedural Geometry Engine。
- P1 必须从一开始 Rig-aware，但不接入 ML Kit。
- 先验证一个完整闭环，再扩大算法范围。
- Android GLES 3.0 是目标平台，PC 只做辅助测试。
- 用户提供的视觉验收是 Gate；AI 自评不能代替用户通过。

---

## 1. 通用阶段执行闭环

每个阶段都必须执行同一流程：

1. **需求（Requirements）**：本阶段解决什么问题、输入/输出、范围、非目标、验收标准。
2. **设计（Design）**：模块边界、数据结构、算法、线程/资源生命周期、兼容性和 ADR。
3. **任务拆解（Task Breakdown）**：文件级任务、依赖关系、可并行任务、风险。
4. **实施（Implementation）**：增量修改现有项目，禁止无理由推翻。
5. **验证（Verification）**：构建、单元测试、集成测试、Android 真机测试和视觉证据。
6. **阶段验收（Gate Review）**：报告 PASS / PASS WITH FOLLOW-UPS / BLOCKED / NOT VERIFIED。
7. **归档（Closeout）**：更新实施报告、RTM、风险清单、ADR、截图、Commit SHA。

### 状态定义

| 状态 | 定义 |
|---|---|
| PASS | 本阶段所有必需测试与验收证据都已实际完成 |
| PASS WITH FOLLOW-UPS | 核心目标达到，但有不阻塞的已知问题，已记录所有者和后续阶段 |
| BLOCKED | 因缺设备、权限、外部 SDK、硬件或其他实际阻碍无法继续 |
| NOT VERIFIED | 代码可能已实现，但尚无足够证据证明工作正确 |

禁止将“代码已写”“编译通过”“单元测试全绿”单独解释为产品验收通过。

---

## 2. P0 — 需求基线、仓库审计与技术契约

### 目标

在写大量新几何代码前，明确项目当前真实状态、需要保留的基础设施，以及身体几何、骨骼和蒙皮之间的统一契约。

### 需求任务

- 阅读当前技术规范、历史 M10–M12 报告和最新代码。
- 获取最新 main SHA、分支、工作区和未提交改动状态。
- 审计 Android Host、JNI、EGL、GLES Renderer、MeshData、Skeleton、Skinning、Humanoid、SDF、Surface 工具。
- 明确 P1 的风格目标：Level 1 Stylized Cartoon Human，而非医学人体或 Pixar 级成品。
- 明确 P1 不接 ML Kit、不做多套换装、不做高级 PBR。

### 设计任务

- 冻结坐标系、单位、矩阵约定。
- 冻结 BoneId 0–21 ABI。
- 明确 A-Pose Bind Pose 的实际 landmarks。
- 审计并解决当前 Shoulder / UpperArm 骨骼语义重叠。
- 定义 Geometry / Mesh / Material / Skeleton / Skinning 数据契约。
- 定义 Body Mesh 和独立组件的边界。
- 定义未来换装 Seam Contract。
- 定义固定视觉相机和验收 Pose。

### 交付物

- `docs/foundation/00-product-scope.md`
- `docs/foundation/01-current-repository-audit.md`
- `docs/foundation/02-skeleton-contract.md`
- `docs/foundation/03-geometry-mesh-contract.md`
- `docs/foundation/04-skinning-contract.md`
- `docs/foundation/05-visual-acceptance-protocol.md`
- 相关 ADR。
- P0 任务与风险清单。

### 验收 Gate P0

- 当前仓库状态和基线 SHA 清楚。
- 不会覆盖未提交用户工作。
- BoneId、父子关系、Bind Pose、坐标和矩阵顺序有明确规范。
- Shoulder / UpperArm 语义重叠有解决方案及回归测试方案。
- P1 几何主线确定，不同时启动多套通用建模算法。
- P1 的视觉目标和非目标明确。

P0 未通过，不允许开始大规模重写人体生成器。

---

## 3. P1-A — 最小几何主线 PoC 与视觉验收

### 目标

验证一条能够生产动画友好人体曲面的最小技术主线。PoC 不追求完整角色，但必须在项目实际 Android GLES Renderer 中看到真实结果。

### 主路线

- 解剖控制点驱动。
- 参数化 Surface Patch / Profile Loft 作为躯干基础。
- 非圆形、多控制环 Sweep 作为肢体基础。
- Topology Template 定义稳定的边界环和主要连接关系。
- 局部 SDF 可用于比较/辅助，但不作为 P1 最终人体网格唯一来源。

### PoC 范围

只制作：

- 躯干（胸腔、腹部、腰部、上骨盆过渡）。
- 肩胸连接区。
- 一侧上臂、肘部和前臂，随后镜像验证。

### 必须对比的内容

- 基础旧 M11/M12 形态作为对照。
- 新的 Surface Patch + Topology Template 主线。
- 如团队认为必要，可加入局部 SDF 辅助版本，但必须限制为一个具体关节或过渡区域。

不要求全面比较 NURBS、自动 Quad Remesh、Dual Contouring、Marching Cubes 等所有算法。

### 输出证据

- 正面、侧面、45°。
- 肩部特写。
- 线框、法线、网格边界。
- 灰模连续光照截图。
- 顶点/三角形/拓扑/生成时间/CPU 内存数据。
- 参数变化前后对比。

### 验收 Gate P1-A

- 轮廓比 M11/M12 对照更符合预定卡通人体比例。
- 胸腔、腰部和肩胸过渡可控，不是几个椭球的平滑组合。
- 肩部和上臂连接没有明显拼接缝或单独球状接头。
- 线框显示拓扑有明确的面片/边界组织，而不是只看连通组件数。
- 曲面法线连续，灰模光照下不出现不合理的环状阶梯。
- 实际 Android GLES 画面与 PC 预览一致到可接受范围。
- 用户对视觉方向确认后，才进入 P1-B。

如果 Gate P1-A 不通过，最多只允许继续迭代当前 PoC，不得顺手实现面部、头发、服装或 ML Kit。

---

## 4. P1-B — 完整基础人体生成

### 目标

将 P1-A 验收的几何方法扩展为一个完整基础人体。

### 需求任务

必须生成：

- 头部、颈部。
- 躯干、胸腔、腰、骨盆。
- 左右肩、上臂、肘、前臂、手掌。
- 左右大腿、膝、小腿、脚踝、脚掌。
- 基础可识别面部（眼睛/眉毛/鼻/嘴/耳的初级形态）。
- 一个基础卡通默认外观。

### 设计任务

- 所有几何由统一 BodyLandmarks / RigDefinition 驱动。
- 主要表面使用稳定拓扑模板。
- 头部、眼球等可作为独立 Mesh Component，但接缝必须自然。
- 为 P2 预留 Neck、Shoulder、Wrist、Waist、Hip、Ankle 接缝边界。
- 所有关键比例参数必须能真正影响几何。

### 实施任务

- 建立 BodyGenerator、HeadGenerator、Hand/Foot 基础生成器。
- 统一 Mesh Validation。
- 输出完整人体多视角预览。
- 保留 M12 Anatomy Generator 为单独实验路径。

### 验收 Gate P1-B / G1

- 完整人体正面、侧面、背面、45° 轮廓自然。
- 主要部位比例协调，模型不再像圆管、球体和椭球拼接。
- 无明显孔洞、裂缝、自交或法线错误。
- 手掌和脚部具备可识别的卡通形态。
- 至少三个不同参数组合生成不同但结构正确的身体比例。
- Android 真机输出固定视角证据。

未通过 G1，不进入骨骼权重细化和大规模服装开发。

---

## 5. P1-C — 骨骼、蒙皮和程序化动作

### 目标

让 P1-B 人体能够使用标准 Humanoid Rig 稳定驱动。

### 需求与设计

- 使用现有 BoneId 0–21，不随意重排。
- 使用统一 A-Pose Bind Pose。
- 使用 `GlobalPose * InverseBind` 生成 Skin Palette。
- 每顶点最多 4 个影响。
- Semantic Region Mask + Segment Distance 生成权重。
- GLES 3.0 继续使用 UBO Bone Palette 与 GPU LBS。
- CPU Skinning 仅作为测试参考。

### 程序化动作

- Idle。
- 双臂前举。
- 肩外展/内收。
- 肘部 45°/90° 弯曲。
- 躯干前屈/侧弯/扭转。
- 髋关节屈曲。
- 膝关节弯曲。
- 下蹲。
- 单腿抬起。

### 验收 Gate P1-C / G2

- Bind Pose 下不发生网格跳动。
- 权重和为 1，骨骼索引有效。
- 肩、肘、髋、膝无严重塌陷、裂缝和异常拉伸。
- 程序化动作结果可重复。
- 权重热力图和骨架调试模式可用。
- CPU 参考蒙皮与 GPU 结果在允许误差范围内。

未通过 G2，不进入 P2 模块化换装。

---

## 6. P1-D — Android GLES 集成与 P1 最终验收

### 目标

证明完整可驱动人物在目标 Android 设备上可运行。

### 实施任务

- 接入 Avatar Runtime。
- 将生成的 MeshData 上传 GLES Buffer。
- 骨骼矩阵更新和 GPU LBS 在 Render Thread 运行。
- 支持 EGL Surface/Context 重建后资源重新创建。
- 提供固定相机、灰模、线框、法线、骨骼和权重预览模式。
- 保留简单程序化背景和基础卡通材质。

### 验收 Gate P1-D / G3

- APK 构建、安装、启动成功。
- 真机显示的是程序化生成的真实角色，不是预制截图或占位模型。
- 角色可切换至少 5 个程序化动作。
- GLES 3.0 Shader 和 UBO 正常。
- 记录 FPS、Frame time P50/P95/P99、内存、几何生成时间和 GPU Buffer。
- 目标设备达到约定性能目标（基线 30 FPS）；不达标时如实报告瓶颈和优化方案。
- 完整提交 P1 视觉证据和测试证据。
- 用户确认 P1 最终视觉/驱动效果。

P1 完成标志：一个完整、基本精致、可骨骼驱动的程序化卡通角色在 Android 上运行。

---

## 7. P2 — Modular Clothing & Appearance

### 目标

在同一个基础人体和 Skeleton 上，实现真正的程序化模块化换装。

### 功能范围

至少提供：

- 两种上衣（例如 T-shirt、hoodie/jacket）。
- 两种下装（例如 trousers、shorts）。
- 一种鞋子。
- 皮肤颜色参数。
- 头发颜色/发型选择（复杂发型可在 P3 完善）。
- 背景颜色和简单程序化背景切换。

### 技术约束

- Garment 是独立网格生成器，不是把人体换色。
- 衣物顶点使用相同 Skeleton，但生成独立 Skin Weights。
- 使用 P1 预留的 seam contract。
- 换装只替换相关组件，不重建整个 Avatar。
- 不使用外部贴图或模型。

### 验收 Gate P2

- 两种上衣、两种下装和鞋子具有可见的真实几何差异。
- 领口、袖口、裤腰、裤裆和鞋口无明显接缝/穿模。
- 至少复用 P1 关键动作测试，衣服随动作正确变形。
- 组合切换无资源泄漏、无明显加载卡顿。

---

## 8. P3 — Character Visual Polish

### 目标

提升整体角色的美术质量，不改变已稳定的 Rig / Mesh / Avatar 基础契约。

### 范围

- 面部比例、眼眶/眼睑、鼻唇和耳朵细化。
- 多种头发几何和分层发束。
- 服装轮廓、厚度、褶皱和组件细节。
- 皮肤、眼睛、头发、布料、鞋材质。
- 平滑光照、基础 AO/Fresnel/高光等低成本效果。
- 更多身体比例和风格 Preset。

### 验收

- 使用固定相机和固定灯光截图比较。
- 每次美术改动说明解决了什么具体视觉缺陷。
- 不以三角形数量或 Shader 复杂度替代视觉提升。
- 目标设备运行数据不出现不可接受退化。

---

## 9. P4 — 实时 Pose Tracking

### 目标

将真人视频姿态转换为 P1/P2/P3 角色骨骼姿态。

### 任务

- 审计现有 ML Kit Pose Provider。
- 明确 2D landmarks 与任何 3D/world landmarks 的区别。
- 统一 Image/Provider/Avatar 坐标转换。
- 正确处理前后摄像头镜像和旋转，确保只转换一次。
- 使用置信度、缺失关键点保持、时间滤波。
- 使用身体坐标系、关节平面和 IK 约束改善重定向。
- 姿态推理与渲染解耦；渲染按最新姿态插值，不让过期推理队列累积。

### 验收

- 先使用确定的测试视频，再使用真实摄像头。
- 记录 Pose FPS、Inference time、End-to-End latency、丢失关键点比例。
- 明确单目深度的能力边界，不把 Z=0 或推测值描述成真实 3D 捕捉。
- 用固定动作集验证左/右、抬臂、屈肘、躯干转向、下蹲。

---

## 10. P5 — 视频输入、回放与设备优化

### 目标

支持本地视频和低性能 Android 设备的稳定运行。

### 任务

- MediaCodec / 视频时间戳同步。
- PoseFrame 与视频 PTS 对齐。
- 暂停、快进、回放、seek 和倍速时清理/重置滤波状态。
- 输入解码、推理、动作运行时和渲染线程之间使用有界队列。
- 旧帧策略明确（实时模式优先最新帧，文件回放遵循时间戳）。
- 监控 Native Heap、GPU Buffer、温度/降频（设备可测时）。
- 长时间稳定性测试。

### 验收

- 本地视频播放与数字人动作同步。
- Seek/Replay/Speed change 无明显姿态残留。
- 目标设备持续运行达到约定帧率和延迟目标。
- 记录 10–30 分钟稳定性测试结果，不能虚报摄像头/电视盒子未实际验证的结果。

---

## 11. 每阶段必须维护的工程记录

目录建议：

```text
docs/
  foundation/
  roadmap/
  architecture-decisions/
  phases/
    P0-foundation-contracts/
    P1A-geometry-poc/
    P1B-full-character/
    P1C-rig-skinning/
    P1D-android-integration/
    P2-modular-clothing/
    P3-visual-polish/
    P4-pose-tracking/
    P5-video-and-optimization/
  quality/
    visual-regression/
    geometry-reports/
    performance-reports/
```

每个阶段至少提交：

- `00-scope.md`
- `01-requirements.md`
- `02-design.md`
- `03-task-breakdown.md`
- `04-test-and-acceptance.md`
- `05-implementation-report.md`
- `evidence/`

同时更新：

- Requirements Traceability Matrix（RTM）。
- Risk Register。
- ADR Index。
- Project Status。
- Commit/Build/Test Evidence。

---

## 12. 优先级与变更控制

P0/P1 期间禁止临时扩展到：通用建模器、通用 Quad Remesher、DQS、Corrective Shapes、完整 PBR、布料物理、ML Kit 实时跟随、复杂场景系统。

如 AI 发现必须新增技术：

1. 描述当前失败案例。
2. 说明现有最小技术为什么无法解决。
3. 提供至少一个替代方案。
4. 评估代码、性能和维护成本。
5. 提出 ADR。
6. 先做局部 PoC，不直接扩大主线范围。

---

## 13. 项目完成定义

整个 MVP 只有在以下内容都通过后才可声明完成：

- P1：完整可驱动程序化卡通人物。
- P2：真实几何换装。
- P3：外观质量达到约定目标。
- P4：实时人体跟随。
- P5：视频回放、目标设备性能和稳定性验证。

每项均需要真实代码、测试、Android 证据和用户验收。文档完成、模块命名、AI 自评或主机单元测试不能代替最终产品验收。
