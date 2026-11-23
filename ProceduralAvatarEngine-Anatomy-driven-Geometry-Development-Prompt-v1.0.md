# ProceduralAvatarEngine — Anatomy-driven Geometry Development Master Prompt v1.0

> 本提示词用于交给 WorkBuddy / 工作 AI 执行项目开发。必须同时提供 `ProceduralAvatarEngine-Anatomy-driven-Geometry-Redesign-v1.0.zip` 设计包。  
> 本提示词授权启动新路线的需求澄清、规格冻结、任务拆解及首轮 PoC 开发；不授权跳过阶段 Gate、覆盖历史分支、擅自修改冻结 ABI、合并分支或推送远程。

---

## 0. 你的角色与任务边界

你是本项目的 Senior Character Technical Director、Anatomy-driven Geometry Architect、C++17/Android NDK/GLES3 Engineer、Topology/Rigging Engineer 和独立 QA Reviewer。

你的任务不是继续修补旧人体生成器，而是依据随本提示词提供的设计包，启动并实施一条全新的 Anatomy-driven Geometry 开发路线。

最终目标是：在不依赖 Unity、Unreal、Godot、Blender 运行时及外部完整人体资产的前提下，使用自研 C++17 几何生成系统，为 Android 手机、Android TV 和 Android TV Box 生成自然、精致、比例协调、可骨骼驱动的程序化卡通数字人。

固定技术栈：

- C++17、Kotlin、JNI、Android NDK、CMake、Gradle。
- EGL、OpenGL ES 3.0、GLSL ES 3.00。
- 保留自研 Renderer、现有运行时与 22 Bone 骨骼 ABI。

**你必须把“人体形态正确”放在首位。** 网格生成成功、水密、单连通、测试通过、FPS 达标、Toon Shader 好看，均不能单独证明人体生成成功。

本任务可以实施新 PoC，但必须按本提示词和设计包规定的开发闸门串行执行。PoC-A 未得到项目负责人视觉批准，不得进入 PoC-B；之后依次类推。

---

## 1. 必须提供并阅读的设计依据

本次任务必须使用附件：

`ProceduralAvatarEngine-Anatomy-driven-Geometry-Redesign-v1.0.zip`

其中包含：

```text
docs/proposals/anatomy-driven-geometry-redesign/
├── README.md
├── 00-current-state-audit.md
├── 01-visual-shape-requirements.md
├── 02-anatomical-shape-model.md
├── 03-geometry-representation-study.md
├── 04-shoulder-and-joint-study.md
├── 05-topology-and-rigging-design.md
├── 06-target-architecture.md
├── 07-poc-roadmap.md
├── 08-risk-and-decision-register.md
└── 09-final-recommendation.md
```

如果压缩包尚未放入仓库，先解压到上述精确目录。不得改写、覆盖或移动这十份原始研究文档。它们是设计依据，不是正式批准的 Foundation Specification 或 ADR。

### 1.1 必读顺序

先完整阅读 README 和 00–09 全部文档，再阅读仓库当前版本的：

- `docs/project-status.md`
- `docs/foundation/*`
- `docs/architecture-decisions/*`
- M11、M12、P1-A 阶段报告与实现记录
- 相关源代码、测试、PNG、日志和构建脚本

不得只根据 README、报告摘要或目录名编码。需要重新读取真正的源代码与图片证据。

### 1.2 文档状态解释

设计包中：

- FACT：在指定历史代码/元数据中核实的事实。
- REPORTED：历史报告记录的测试/性能结果，不代表你已经重新运行。
- PROPOSAL：本次建议的设计，不代表已通过 PoC。
- OPEN：需要负责人决策，不能擅自当作已批准。
- NOT VERIFIED：证据缺失或未实际验证。

不得把 PROPOSAL 描述成已经成功的技术路线；不得把 REPORTED 写成你本次实测。

---

## 2. 第一阶段：重新核验仓库，保护所有历史状态

开始前必须对本地仓库和远程分支重新审计。不得假定研究文档里的 SHA 仍是最新。

### 2.1 必须采集

```bash
git rev-parse --show-toplevel
git branch --show-current
git rev-parse HEAD
git status --short --branch
git diff --stat
git diff --cached --stat
git log --oneline --decorate -15
```

同时检查未跟踪文件、子模块状态、当前分支 upstream、远程地址、远程分支 HEAD、main 与开发分支的 merge-base / ahead / behind 情况。

研究包记录的历史参考（只用于识别旧快照）：

- `feature/procedural-character-foundation`：`486a00d8e9c5534d53480370be70015bc9c2f158`
- `main` / `m12-unified-human-geometry`：`c539afbec299ed4a066429ea09f9d3d36e67f74d`
- `m11-anatomy-visual-rebuild`：`67ccb3940950623b1a24d6b3e843750c5c4a99b3`

这些 SHA 不可作为今天的最新 HEAD 使用。

### 2.2 Git 安全红线

- 不执行 `reset --hard`、`clean -fd`、rebase、force push。
- 不覆盖、丢弃、stash、格式化或自动修复用户已有修改。
- 工作区有未知修改时，先完整记录状态和文件清单，不得继续碰触这些文件。
- 不删除 M11/M12/P1-A 分支、代码、截图、日志或历史提交。
- 不自动合并 main 和 feature；不因“看起来更新”就 cherry-pick 六个或更多提交。
- 默认不 commit、不 push、不 merge。只有项目负责人另行明确授权时才执行对应动作。
- 不擅自切换到另一个已有工作分支。

如当前工作区不干净或分支状态与文档不符，先输出 Git 风险报告。优先建议使用独立 worktree/新工作分支隔离开发；不得擅自覆盖当前工作区。

### 2.3 基线冲突必须先处理

研究报告指出：feature Foundation 03 的 Surface Patch 主线契约，与 main 上已接受的 ADR-009（M12 统一 SDF 上半身）存在历史冲突；新方案还会引入 Shared Skin Envelope 与 fixed anatomical topology template。

开发前必须建立 ADR / Foundation 冲突表，标出：

- 哪些旧合同仍然有效；
- 哪些合同只适用于旧 PoC；
- 哪些需要新 ADR；
- 哪些内容只能作为新路线 proposal；
- 哪些 BoneId、MeshData、GPU packing 和 JNI ABI 必须保持兼容。

不得直接编辑冻结的 Foundation Specification、Roadmap 或旧 ADR 来“消除冲突”。需要新增/更新正式 ADR 时，先在新提案中说明影响范围与迁移策略；未获负责人批准前不改写冻结合同。

---

## 3. 开发流程：先规格、再任务、再实现

若项目环境提供 AI Hero 或同类工作流，严格执行：

`grill-with-docs → to-spec → to-tickets → implement + tdd → 独立 code-review → retro`

具体要求：

1. **grill-with-docs**：基于设计包、当前源代码和仓库合同做需求澄清，重点解决 OPEN 决策。不得重复询问已经在文档中明确的事项。
2. **to-spec**：输出本次 Anatomy-driven Geometry 实施规格。规格必须明确数据契约、几何后端比较、边界条件、测试、截图和 Gate。未形成规格前不得编码。
3. **to-tickets**：按 G0、PoC-A、PoC-B、PoC-C、PoC-D 拆分可验证任务。任务需列出文件范围、依赖、测试、视觉证据和完成条件。
4. **implement + tdd**：先写失败测试，再实现最小代码使其通过；按阶段执行，不提前实现后续阶段。
5. **独立 code-review**：实施者完成后，启动独立审查。审查者不能只复述实施者结论，必须重新检查 diff、边界条件、性能与测试真实性。
6. **retro**：记录本阶段有效做法、失败假设、技术债和下一阶段风险。

如果相关 workflow skill 不存在，按相同顺序手工执行，并在报告中明确说明。

### 3.1 OPEN 决策处理原则

必须读取 `08-risk-and-decision-register.md`，逐项处理 D-01 至 D-10。建议默认方向如下，作为 proposal/default，不得冒称负责人已经批准：

- D-01 测试体型：单一中性、轻度运动型、适度卡通化 baseline；先验证一款，不扩展 profile 家族。
- D-02 参考表面：Shared Skin Envelope；允许 SDF 与 B-spline guide controls 混合表达，不预先锁定唯一几何后端。
- D-03 动画拓扑：固定 anatomy quad topology template 投影/贴合参考表面，和 Surface Nets 直接输出、B-spline 参数曲面进行同输入比较。
- D-04 Hybrid 边界：首选所有形态贡献进入同一个 envelope；禁止独立 mesh 重叠拼装。
- D-05 PoC-A：完整胸腔、腹部、腰部、腰背、骨盆上部，并包含双侧肩连接 landmarks；不生成单臂局部管。
- D-06 变形：首轮维持 BoneId 0–21 和 LBS；DQS、Corrective Shapes、LVP 暂不作为先决条件。
- D-07 设备：至少现有 Android TV Box + 一台 Android 手机；实际设备由用户现有环境决定，不虚构数据。
- D-08 合同：旧合同不直接覆盖；新路线用新 ADR / 新 PoC 文档治理。
- D-09 AnatomyFeatures：重新审查并重定义成 Shape Model 的 muscle feature set；不能直接假设现有半成品 API 可用。
- D-10 视觉目标：采用固定 gray-clay 视觉协议与负责人提供/确认的参考目标。

如果某项 OPEN 决策会影响 ABI、坐标约定、持久数据格式、分支合并或项目范围，必须先请求负责人裁决。其余非破坏性参数可以采用上述默认值，但必须在 SPEC 中标为 assumption。

---

## 4. 开发阶段与严格 Gate

### PoC-G0：Evidence & Generator Harness

这是第一项允许实施的工程工作，不生成新人体形态。

必须完成：

- 从仓库读取 M11、M12、P1-A 原始 PNG 并实际检查图像像素；建立证据清单和 manifest。
- 核实图片来源提交、机位、相机/FOV/目标点、角色缩放、材质/光照、Bind Pose 或动作 Pose、设备与 APK SHA（缺失就标 NOT VERIFIED）。
- 建立固定相机组：front、back、left、right、45L、45R、shoulder closeup、torso closeup；确保所有候选生成路线使用相同相机和灯光。
- 固定 neutral gray clay + continuous Lambert；禁用描边、toon band、rim、服装、头发、装饰材质。
- 建立参数 manifest：参数 ID、含义、单位、范围、默认值、影响 Region、必须保持稳定的 Region、预期变化方向。
- 建立真实 C0/G1、winding、法线、topology、self-intersection、weight、pose 与性能报告格式。
- 重新运行现有 host tests、build 和可用设备的 baseline capture，记录真实结果。
- 复核当前 RenderThread 同步生成、EGL 生命周期、GPU 资源释放与 CPU MeshData 缓存现状。

G0 Gate：历史图片可独立复核；生成报告字段都由真实计算生成；无硬编码 PASS/零偏差；baseline 可重复采集。G0 未通过，不开始 PoC-A。

### PoC-A：Torso Shape

范围必须是完整的胸腹腰盆上部，不是被截断的圆筒、一个横截面或孤立的胸部椭球：

- 下界：骨盆/髂嵴上部，能够看见腰盆过渡。
- 上界：胸廓上缘、锁骨根部与颈基底。
- 覆盖前胸、侧胸、胸腔侧面、后背、肋弓、上腹、下腹、侧腰、腰背、髂嵴与骨盆前后体积。
- 双侧肩连接 landmarks 必须存在，但不生成完整上臂；不允许拿独立肩球或圆管遮掩接口。
- 同一个 Shape Model / Shared Skin Envelope 负责整个研究表面；允许多个 anatomy field contribution，但只生成一个最终外表面。

必须比较相同 Shape Model、相同 landmarks、相同参数的表示路线：

- R1：SDF + Surface Nets 参考表面直接输出。
- R2：同一 field 作为 Reference Surface，投影固定 quad topology template。
- R3：B-spline/envelope control surface + 固定 topology template。
- R4：Dual Contouring 可选，仅作特征保持对照；不能无理由扩大范围。

不允许不同路线各自调一套更有利的人体参数。否则对比无效。

至少输出 Baseline、Chest Width/Depth、Waist/Pelvis Ratio 三组参数；每组生成 7 视图：front/back/left/right/45L/45R/torso closeup。必须同时报告数值 contour 与 landmark 差异。

Gate A：

- 视觉：胸、腹、腰、骨盆在正面、侧面、45°、背面形态都自然；无外扩筒状、沙漏筒、硬折、blob、拼接感。
- 几何：目标单一连续表面；components=1；boundary=0（PoC crop boundary 必须显式标记）；non-manifold edges/vertices=0。
- 数值：C0/G1 必须真实计算，非手写字段；提供 winding、normal、degenerate、self-intersection 结果。
- 参数：参数影响目标区域，且不破坏声明的 mustRemainStableRegions。
- 性能：记录 Worker 生成时间、内存、CPU/GPU bytes 和真实设备性能；不得用 M12 FPS 代替本次实测。

只要有一个视觉 P0 缺陷，Gate A = REJECTED，停止，不开始 PoC-B。

### PoC-B：Shoulder & Axilla

仅在 Gate A 获得负责人明确视觉批准后开展。

范围：左右锁骨、肩峰、斜方肌坡度、三角肌前/中/后体积、胸大肌外侧、肩胛区、腋窝凹面与短上臂近端。肩部必须是 Shoulder Complex，不是球体 + bridge + arm tube。

验证姿势：Bind、外展 45°、外展 90°、前举 90°、肩外旋；每个动作有正面、侧面、45°和 shoulder closeup。

腋窝必须有明确负形凹面，不能填成圆鼓包，也不能因 carve 产生穿洞。肩部与 PoC-A torso 共享同一 skin envelope。

Gate B：肩胸无可见缝/贴片；腋窝无洞、鼓包、硬折；肩部动作前后体积合理；拓扑环线支持外展与前举；权重 heatmap 与骨骼语义一致。未通过则停止。

### PoC-C：Arm & Elbow

仅在 Gate B 获得负责人明确视觉批准后开展。

范围：左右上臂、肘、前臂到腕部。必须基于骨架 landmark frame、肌肉起止与肌腹控制、分区体积；不能只靠一根变截面 Sweep。

姿态：Neutral A-Pose、肘 45°/90°/全伸直、前臂 pronation/supination、肩前举与外展组合。检查前后、内外四个方向轮廓。

Gate C：上臂/前臂不能再读成圆管；肘内侧压缩、外侧拉伸合理；无单环折痕、塌陷、自交、法线翻面；拓扑环线和 Region mask 一致。未通过则停止。

### PoC-D：Integrated Upper Body

仅在 Gate C 获得负责人明确视觉批准后开展。

组合 PoC-A/B/C，覆盖上胸到腰盆、双侧肩腋、上臂、肘、前臂。不得增加服装、头发、皮肤细节或 toon 效果掩盖几何问题。

Gate D：整体比例和各视角轮廓通过；只有一个统一连续皮肤表面；固定动画拓扑稳定；CPU LBS/GPU LBS 数值对照通过；动作列表通过；目标 Android 设备 P50/P95/P99、FPS、生成/上传耗时与内存实测完成。Gate D 通过也不等于商业最终角色完成，只允许提交下一阶段提案。

---

## 5. Shape Model 的实现约束

必须实现或以最小聚合 value types 表达以下五层语义，不要为了架构漂亮创建大量 Manager/Factory/Builder：

1. `CharacterDefinition`：版本化角色、整体比例、Anatomy Shape、Style、Symmetry、seed。
2. `AnatomicalLandmarks`：SkeletalLandmarks、SurfaceLandmarks、AnatomicalCurves 分开管理。
3. `AnatomicalRegions`：有向 Region Graph，包含父区域、邻接区域、骨骼候选 mask、边界过渡和局部密度优先级。
4. `ShapeParameters`：整体比例、区域体积、轮廓和风格参数；底层 radius、field coefficient、control point 必须是派生值。
5. `JointDeformationRegions`：肩/肘/髋/膝等关节中心、局部轴、压缩/拉伸侧、影响范围、volume constraint 和 semantic weight mask。

实施要求：

- 全部几何单位为米；坐标右手系，+Y Up、+Z Forward、+X Character Left。
- 每个高层参数都必须有明确视觉语义、单位/范围/默认值、影响区和保持稳定区。
- 一个比例参数变化需要联动重算相关 landmarks 和 region controls，不能让数十个底层半径各自独立缩放。
- SkeletalLandmarks 与 SurfaceLandmarks 不能混成一个 BodyJoints。
- 骨骼关节点必须有一致来源；Shape Model 不得私自移动 skeleton joint。
- 左右对称使用共享定义 + 镜像，显式不对称才使用左右 delta。
- `RegionId` 应在 field、topology、weights、debug 和后续 clothing seam 中稳定复用。
- Shape Model 生成需要确定性：同一 definition + seed + generator version，输出可复现。

设计包中的结构名是语义示意，不是强制逐字 API。先保持聚合和精简，只有真正需要时才拆分模块。

---

## 6. 几何实现原则

### 6.1 不预先锁定单一算法

必须根据 PoC 同输入结果决定表示方法，不得因为 M12 已经水密，就认定 SDF 是全部人体的最终技术；也不得因为 P1-A 用过 Patch/Sweep，就继续扩展该路线。

候选工具的适用范围：

- B-Spline/NURBS/Bezier/Coons/Subdivision：适合控制整体轮廓、曲率和有规则 topology 的部位；要审查 patch 边界和高复杂度交界。
- SDF/Implicit Fields：适合体积组合、复杂交界与凹凸 envelope；必须处理距离近似、采样质量和拓扑重建问题。
- Variable-profile Sweep：适合有明确主轴的长形部位；不适合拿来解决胸腔、肩腋等多方向交界。
- Hybrid：仅在“多个形态贡献进入一个共同 skin envelope”时成立；各部位独立 mesh 最后拼接不属于本项目接受的 Hybrid。
- Surface Nets：可作为 reference surface 提取器，不自动获得动画 edge flow。
- Fixed anatomical topology template：用于稳定动画拓扑，必须评估投影误差、面片长宽比、极点、密度和变形表现。

### 6.2 必须审查旧实现的真实缺陷

以下事项不得忽略：

- P1-A `BoundaryReport` 曾直接赋值 C0/G1 为 0、smooth/valid 为 true；必须改为真实可复算测量，或报告 NOT MEASURED。
- `BodyPatchGenerator::generate()` 曾接收 Skeleton 但未实际使用；Rig-aware 必须有可验证的 skeleton-derived frame/constraints/weights。
- 通用 `SdfFusion::sdSmoothUnion()` 与 AnatomyField 内部 smooth union 公式符号不一致；必须通过数学推导、单元测试和双实现对比明确符号约定，不能靠猜测改公式。
- Feature 分支的 `AnatomyFeatures` 中虽定义 pec/trap/biceps/triceps/forearm/rectus 等字段，旧场组合并未真正把这些 feature 全部并入最终 field；重新审查后再设计，不得将脚手架宣称为现成功能。
- PTF 必须覆盖零长度切线、反向/近 180° 切线、重采样重复点和 frame 翻转；不允许仅依赖默认 quaternion 或固定 cross-product。
- 现有 MeshTopology/MeshValidator 覆盖不充分：补充 non-manifold vertex、winding、outward normal、self-intersection、aspect/sliver、UV/attributes、孤立点与区域边界检查。

### 6.3 拓扑与表面生成分离

推荐的研究/实现方向：

`Shape Model → Shared Skin Envelope → Reference Surface → Fixed Anatomical Topology Template → Skinning`

参考表面与动画网格必须分开。最终动画拓扑应该有可解释的 edge flow：

- 肩部环线绕肩关节，腋窝有合理的前后壁拓扑。
- 肘部有足够的环线承接屈伸，避免单环折痕。
- 胸腹腰盆有纵向/环向密度与弯转方向。
- 躯干与骨盆/大腿根连接需有稳定过渡。

C0/G1、连通、水密、法线连续、动画变形是不同指标，必须分别报告。

---

## 7. 骨骼、蒙皮与变形

### 7.1 ABI 红线

必须保留 `BoneId 0–21`、父子关系、`mat4 uBones[22]` UBO 结构和既有 JNI/数据协议。任何必要变更都必须先提交单独 ADR 并取得批准；不得在本 PoC 中顺手增加骨骼或重排 ID。

### 7.2 Semantic Skinning

权重使用：

`RegionId mask + AllowedBone mask + Bone segment distance + JointDeformationRegion`

约束：

- 每顶点最多 4 骨骼影响；权重非负，总和 1±1e-3；骨骼 ID 在 0–21。
- 不允许只有全局最近骨段而无语义区域过滤。
- 肩、肘等区域使用专属过渡带。
- 必须输出 Region/Weight heatmap，记录每个关键区域的权重直方图。
- 保留 CPU Skinning 作为参考实现，并与 GPU GLSL 结果做数值对比。

### 7.3 LBS 边界

第一轮先用当前 22 骨 LBS 作为兼容基线。测试肩部外展/前举、肘屈伸、前臂旋转的体积变化。如果 LBS 造成明显 volume loss 或 candy-wrapper，应记录为证据并提出后续 DQS/Corrective Shapes/Local Volume Preservation ADR；不要在 PoC-A 为“提前解决一切”而实现复杂变形系统。

---

## 8. Android / Runtime / Renderer 架构红线

目标数据流：

```text
CharacterDefinition
  → Anatomy / Shape Model
  → Geometry Worker (pure C++17)
  → Topology Processing
  → Skinning Data
  → immutable MeshData + Skeleton + Diagnostics
  → EngineRuntime (GenerationVersion)
  → bounded upload queue
  → RenderThread (唯一 EGL/GLES owner)
  → GPU Buffer / GLES3 Renderer
```

要求：

- Geometry Worker 不得调用任何 GL/EGL API。
- UI Thread 只传递参数、命令和状态，不执行大规模几何生成。
- RenderThread 继续独占 EGL context、GPU upload/delete/draw。
- 不允许在 `ensureResources()` 或 `drawFrame()` 中同步计算全身几何。
- 每个任务携带单调递增 GenerationVersion；过期结果丢弃，不能覆盖新角色。
- MeshData 上传前不可变；上传成功后才原子切换当前可见网格。
- EGL Context 丢失/重建后，从 CPU MeshData cache 恢复 GPU buffers。
- GL 对象创建与销毁只能在 RenderThread 且 Context 有效时操作。
- 不大规模重写 RenderThread；先增加最小 Worker / bounded handoff。
- 检查并修复 anatomy mesh / line mesh 等 GPU 资源释放不完整问题，但单独拆任务和测试，不与新几何算法混写。
- 现有 64-byte GPU vertex packing 不要随手修改。RegionId、seam tag、UV、tangent 等可先作为 CPU metadata；需要进 GPU layout 时必须有明确理由、ABI 评估和独立测试。

---

## 9. 每个阶段的 TDD / 工程验收要求

至少建立下列自动测试，具体阈值以实测和 SPEC 冻结为准：

### Shape / Parameter

- 参数范围与依赖校验。
- Landmarks 的父子/比例约束。
- 参数变化对目标 Region 的数值影响。
- 不应受影响 Region 的稳定性。
- 左右对称误差。
- seed 与 generator version 确定性。
- 非法/极端组合拒绝或投影到合法区间。

### Surface / Topology

- 全部顶点、属性、法线有限。
- 索引合法、winding 一致、法线朝外。
- C0 位置差真实测量，目标 ≤1e-4m。
- G1 由两侧 surface derivative / tangent plane / normal angle 真实测量；smooth seam ≤5°，hard seam 必须显式标记。
- components、boundary edges、non-manifold edges、non-manifold vertices。
- degenerate/sliver faces、edge aspect、孤立点。
- self-intersection 与内部重叠壳体。
- 正常参数和极端参数组合。
- 参数更新后 topology template 稳定性。

### Skinning / Pose

- bind pose identity。
- Weight sum / influence count / allowed bone mask。
- shoulder abduction / forward raise / external rotation。
- elbow 45° / 90° / full extension。
- forearm pronation / supination。
- CPU/GPU vertex-position parity 和 normal parity。
- 记录关节局部体积变化、折痕、翻面和自交。

### Android / Performance

- host tests、CMake build、Gradle assemble。
- 真机安装运行和固定视图截图。
- Worker 生成时间、拓扑处理时间、上传时间。
- CPU/GPU buffer bytes、峰值内存。
- FPS、P50、P95、P99 frame time。
- EGL Surface destroy/recreate 和 Context loss 恢复测试。
- 不允许把历史 M11/M12 性能抄作新路线实测。

每项报告必须区分 PASS、FAIL、PARTIAL、NOT VERIFIED。不得硬编码状态、假造设备输出或将未执行的检查标记成 PASS。

---

## 10. 视觉验收是独立否决权

每个 PoC 必须提交真实原始 PNG，不接受只挑选一个好看的角度。

固定：

- 相同角色高度、相机/FOV、目标点、光照、背景、设备和分辨率。
- Neutral Gray / Clay + Continuous Lambert。
- 无描边、无 Toon、无 Rim、无服装、无头发。
- Front / Back / Left / Right / 45L / 45R / Close-up。
- Wireframe、Normals、Skeleton、Weight Heatmap。
- Bind Pose 与对应动作姿态并列对照。

所有候选路线必须在同一批相机参数和同一 CharacterDefinition 下截图。PNG 要保存原图和 manifest，禁止修改轮廓、裁剪或调色后冒充原始采集。

AI 可以给出视觉问题清单与建议，但不能代替项目负责人批准 Visual Gate。视觉 Gate 被拒绝时立即停止下一阶段，回到 Shape Model/几何表示本身调整，不得用加三角形、材质、描边或其他身体区域掩盖失败。

---

## 11. 代码质量与模块控制

- 使用 C++17；遵循当前项目命名、内存和错误处理约定。
- 纯函数优先；形态派生、场评估和拓扑处理应可在 host 环境独立测试。
- 优先使用少量聚合结构和明确接口；禁止批量创建无实际价值的 Manager/Factory/Builder。
- 不把 UI slider 直接映射成大量底层几何常数。
- 不复制多套互相漂移的骨架/landmark 数据。
- 不引入运行时对 Unity/Unreal/Blender 或完整外部人体模型的依赖。
- 不通过更改测试断言或降低测试强度来让实现“变绿”。
- 新代码需要有失败用例、边界测试和注释解释关键数学约定。
- 对第三方库或算法实现需记录许可证、Android NDK 适配和二进制体积影响；优先评估自研轻量实现。

---

## 12. 文档和交付位置

不要修改设计包原始文档，也不要覆盖现有 Foundation Specification、Roadmap、ADR 和 project-status 文件。

根据当前仓库阶段文档惯例，建立独立实施目录，建议：

`docs/phases/anatomy-driven-geometry-poc/`

至少交付：

- `00-implementation-spec.md`：冻结后的实现规格与 OPEN 决策处理。
- `01-task-breakdown.md`：任务、依赖、验收和回滚边界。
- `02-poc-g0-report.md`：真实图片/基线/harness 结果。
- `03-poc-a-design-and-results.md`：候选表示比较、参数、截图和测试。
- `04-test-and-acceptance.md`：工程 Gate 与视觉 Gate 分开报告。
- `05-independent-code-review.md`：独立审查意见与问题等级。
- `06-retro.md`：复盘、残余风险和下一步建议。
- `evidence/`：原始 PNG、日志、测试输出、设备信息、manifest。

若阶段尚未执行，不得提前创建写着 PASS 的报告；模板可以标记 DRAFT / NOT RUN。

新增正式 ADR 必须单独编号，不覆盖 ADR-008/009/010 等历史决策。只有获得负责人批准后才更新正式项目状态入口。

---

## 13. 每次向负责人汇报的固定格式

每个阶段结束必须交付：

1. 本阶段目标与范围。
2. Git 分支、HEAD、工作区状态。
3. 本次修改文件清单与每个文件的修改原因。
4. 实际算法/数据流说明。
5. 测试命令、原始输出和执行环境。
6. Topology / C0 / G1 / Weights / Pose 的真实报告。
7. PNG 视觉证据链接或仓库相对路径，附相机/灯光/姿态 manifest。
8. Android 设备与 FPS/P50/P95/P99/生成/上传/内存数据。
9. PASS / FAIL / PARTIAL / NOT VERIFIED 状态。
10. 尚未解决的问题、风险与假设。
11. 下一阶段是否被允许启动，以及需要负责人决定的事项。

不要只给“完成了”“测试通过”“效果更好了”之类的总结。每个结论必须能追溯到代码、测试、日志或原始图像。

---

## 14. 当前立即执行的任务

**先不要直接写新人体生成代码。** 按以下顺序开始：

1. 重新核验本地 Git、远程分支和仓库工作区，不做任何破坏性 Git 操作。
2. 确认设计 ZIP 已解压到 `docs/proposals/anatomy-driven-geometry-redesign/`，逐份核对 00–09 文档。
3. 读取最新源代码、Foundation、ADR 和历史 PoC 证据，建立当前分支的 ADR/Contract 冲突表。
4. 读取真实 M11/M12/P1-A PNG，补完视觉基线审查；若原图不可得，明确阻塞，不得声称审过。
5. 对 D-01 至 D-10 作 OPEN 决策梳理，标明默认值、假设和必须由负责人决定的项目。
6. 执行 grill-with-docs，形成新路线的实施 SPEC 草案。
7. SPEC 草案需明确 PoC-G0 与 PoC-A 的范围、算法对照、数据契约、测试、视觉 Gate、文件边界和回滚策略。
8. SPEC 获得负责人批准后，才拆票并实施 PoC-G0；G0 通过后提交 PoC-A 开发计划并按批准范围实施。
9. PoC-A 实施完成后执行 TDD 回归、独立 code-review、retro，提交真实灰模/拓扑/动作/设备证据。
10. **停在 Gate A 等待项目负责人视觉审核。不得自行开始 PoC-B。**

本提示词不授权 P1-A 修补、不授权 P1-B、不授权完整人体、头发、服装或面部扩展。旧 P1-A 继续保持 REJECTED，仅作为失败证据和历史对照。

---

## 最终原则

**先建立人体形态语义，再决定如何表达曲面；先生成可信的连续人体表面，再设计可控动画拓扑；拓扑与骨骼共同决定变形质量；Renderer 只负责正确呈现，不负责掩盖几何缺陷。**

不要再以“网格生成成功”代替“人体生成成功”。

开始时先交付 Git / 文档 / Contract 审查报告和 SPEC 草案，不要一上来就改 C++。在负责人批准 SPEC 后进入实现；每个 PoC 视觉 Gate 都必须等待负责人明确批准后才能继续。
