# 00 — Current State Audit

**审计日期：2026-11-23 · 代码审查范围：远程 GitHub · 不含本机工作区**

## 1. 审计范围与可信度约定

本报告基于以下远程 Git 对象和实际文件内容：

| 分支 | HEAD | 本次用途 |
|---|---|---|
| `feature/procedural-character-foundation` | `486a00d8e9c5534d53480370be70015bc9c2f158` | 本次主审查基线，仍是该分支远程 HEAD |
| `main` | `c539afbec299ed4a066429ea09f9d3d36e67f74d` | M12 最新可见参考 |
| `m11-anatomy-visual-rebuild` | `67ccb3940950623b1a24d6b3e843750c5c4a99b3` | M11 历史参考 |
| `m12-unified-human-geometry` | `c539afbec299ed4a066429ea09f9d3d36e67f74d` | M12 历史参考，与 main 同 HEAD |

GitHub compare 结果：以 feature 为 base、main 为 head 时 `ahead_by=0 / behind_by=6`，表示 feature 有 6 个相对 main 的独有提交；不能把 main 与 feature 当成同一个快照。指定 SHA 正是 feature HEAD，无基线漂移。

**本机 Git 工作区状态：NOT VERIFIED。** 远程 GitHub API 无法检查开发机的 `git status`、暂存区、未跟踪文件、子模块状态或本地未推送提交。本次未得到本地仓库目录访问权，不会声称工作区干净。

标记规则：FACT=代码/元数据可直接确认；REPORTED=项目归档报告记录但本次未重新跑；HYPOTHESIS=原因解释待 PoC 验证；NOT VERIFIED=本次缺少可直接复核证据。

## 2. Git 与阶段状态

- Feature HEAD 的提交标题是 `docs: record P1-A visual rejection and defect archive`。变更是记录 P1-A Gate REJECTED、缺陷归档及状态文件；没有把 P1-A 改为通过。
- M12 `main` 头部提交 `c539af...` 是 Gate A 后续形态修正：将手臂外展角由 24°/10°改为 10°/6°，增加三角肌底部半径，并重采 24 张图片。提交日志报告 ctest 8/8、左右像素对称差小于 0.03%。这是 main/M12 的证据，不能算作 feature 的 P1-A 结果。
- `main/docs/project-status.md` 内容仍显示 M11 收尾阶段，明显早于其 M12 HEAD；项目状态文档对 main 当前头部并不同步。正式状态判断应以具体阶段报告、ADR 和提交内容交叉核对，不单独依赖该文件。
- Feature 中 `docs/project-status.md` 与 `docs/phases/P1A-geometry-poc/04/05/06` 明确记载 P1-A **REJECTED**，禁止进入 P1-B。

## 3. M11：历史参数化完整角色路线

### 3.1 实际实现（FACT）

主要代码：`native/procedural/humanoid/Humanoid.cpp`、`native/procedural/surfaces/ParametricSurfaces.cpp`、`native/procedural/geometry/CatmullRom.h`、`native/skeleton/weights/WeightGenerator.cpp`。

- 躯干通过 `computeTorsoSections()` 生成 7 个主要 LoftSection，胸宽/胸深/腰收/髋外扩/骨盆倾斜由参数调节，再以 Catmull-Rom 样条放掠生成表面。
- 颈部、双臂、双腿分别使用 spline sweep；手部是椭球和拇指部件，脚是圆角盒等独立网格。
- `computeArmRings()` 使用肩部、肱二头肌附近、肘、前臂腹部、腕部等 ring 参数，允许椭圆截面、半径变化及 PTF。
- 头、五官、头发和服装也由各自子生成器输出独立 Part。
- `generateHumanoid()` 最后将各 Part 的顶点数组追加到 `combined.vertices`，将索引偏移后追加到 `combined.indices`。它是**缓冲区合并**，不是几何焊接、隐式融合或共享边界拓扑。
- `WeightGenerator::assignWeights()` 以 Path 骨骼及其父子相邻骨为候选，计算顶点到骨段的距离并选前四权重；邻接骨影响距离超过 0.17m 时排除，非 Path 邻骨的总贡献被限制为 30%。

### 3.2 已解决与未解决

已解决/可复用：角色完整度高；参数化 Loft/Sweep 可预测；样条工具、PTF、Face/Hair/Clothing Generator、22 骨骨架和权重计算是可复用的工程积累。

未解决：躯干仍主要由横截面控制；肩胸和肢体由各自表面重叠遮挡；多 Part 缓冲合并不代表连通；拓扑不是跨部位统一的动画 Edge Flow；Path 距离权重缺少解剖语义约束。

### 3.3 历史性能/视觉报告（REPORTED）

M11 阶段报告记录：Medium 约 14,065 triangles、50.6–53.2 FPS、P50 约 18.7–18.9ms、P95 约 21.5–21.9ms、生成约 25–40ms。M11 的视觉矩阵曾将轮廓、躯干、肩部、肢体、变形等打到 4/5，但肩部评分引用了 M10 截图；M12 后续审计也明确指出 M11 的完整多视角证据不充分。因此历史视觉评分不可直接作为本次最终人体质量基准。

## 4. M12：统一 SDF 上半身路线

### 4.1 实际实现（FACT）

主要代码：`native/procedural/humanoid/anatomy/AnatomyField.cpp`、`HumanAnatomyGenerator.cpp`、`native/procedural/fusion/SdfFusion.cpp`。

- 上半身场以肋廓、腹部、骨盆体积场为主体，另有后背 carve、锁骨胶囊、三角肌体积、上臂 round-cone、肘部球形项、前臂 tapered capsule/spindle、颈部和头部占位。
- 双侧解剖项通过 `mirrorX()` 镜像求值，建立对称场。
- Smooth Union / Smooth Subtraction 将多个区域场组合为同一个标量场，再由 Surface Nets 提取三角网格。
- Feature 分支的 Surface Nets 使用各向异性 X/Y/Z 采样；每个跨越等值面的 cell 生成一个 dual vertex，位置是 12 条 cell 边上的零交点平均值；围绕跨越边发射 quad，再三角化。法线由场函数有限差分梯度得到。
- `HumanAnatomyGenerator` 以 joints 和参数建立场，提取网格后，通过 Region Field 的最小值判主区域，再以骨段最近距离挑选次级骨，计算最多两个影响骨骼的权重。
- M12 的 `AnatomyField` 在私有本地函数中使用 `... - k*h*(1-h)` 形式的多项式平滑并集。

### 4.2 一个必须记录的代码不一致

`native/procedural/fusion/SdfFusion.cpp` 的通用 `sdSmoothUnion()` 返回值是 `... + k*h*(1-h)`；而 `AnatomyField.cpp` 私有实现是 `... - k*h*(1-h)`。标准多项式 smooth-min 的常用形式使用减项；两份实现符号不一致，存在对未来调用方造成结果膨胀/偏差的风险。

这不等于证明 M12 当前形状提取结果错误：当前人体场在 `AnatomyField.cpp` 内部组合，并非必然调用通用函数。必须增加两实现对照/符号约定测试并统一，不可用历史通过记录替代此审计。

### 4.3 AnatomyFeatures 的集成状态

Feature 审查快照里新增 `AnatomyFeatures.h/.cpp`、`sdMuscleFan()` 及 pec/trap/biceps/triceps/forearm flexor-extensor/rectus 等锚点结构。这证明项目已开始尝试把“体积块”升级为“肌肉体积”。但在所审查的 `makeUpperBodyField()` 中，最终组合仍只使用肋廓、腹部、骨盆、back carve、三角肌/锁骨、上臂/肘/前臂、颈和头占位；`m.pecA`、`m.trapA`、`m.bicepsA` 等特征字段没有被并入最终场，`AnatomyFeatureSet` 也未成为 `HumanAnatomyParameters` 的成员。

因此这套 AnatomyFeatures 在 feature SHA 上属于**未集成的代码脚手架/候选模型**，不能宣称已经生成了相应肌肉体积。main/M12 HEAD 中甚至没有 `AnatomyFeatures.h` 文件。

### 4.4 M12 实验报告（REPORTED，不是本次重跑）

`docs/phases/M12-unified-human-geometry/06-test-and-acceptance.md` 记录：host 8/8 suites、47 cases；测试分辨率 48×72×32 时 9,920 triangles、components=1、boundary=0、nonmanifold=0、degenerate=0；参数射线测试胸廓半宽约 0.155m→0.320m；确定性提取一致；真机 Android 8.1 / GLES 3.0 约 24,360 triangles、51.7 FPS、P50 19.3ms、P95 19.9ms、P99 20.1ms，24 张截图为 3 debug view × 8 机位。

这些结果只证明对应历史 M12 build/设备上的几何提取、拓扑与性能表现；不证明肌肉形态自然，也不证明 Surface Nets 输出具有动画拓扑。水密、单连通和像素对称不等于商业人体外观验收。

## 5. P1-A：Surface Patch / Sweep PoC

### 5.1 实际实现（FACT）

`native/procedural/character/BodyPatchGenerator.cpp` 的躯干由 5 个 Y 高度站位组成，宽/深/指数由 5 个数组控制，截面使用 Superellipse；顶部局部增加 shoulder bump。当前代码构造了左侧上臂路径与 sweep 环，肩部由 torso arc 到 arm-root arc 的 bridge strips 连接。局部肌肉感由宽深沿程变化和少数 Gaussian bell 函数组合。

这是一种更精细的 Loft + Sweep + Bridge，但不是统一的胸腔-肩带-腋窝三维解剖场。单个 ring/strip 的位置或索引连续不自动形成符合人体体积的肩部轮廓。

### 5.2 BoundaryReport 数据真实性审计（FACT）

代码直接把 `maxPositionDeviation` 与 `maxNormalDeviationDeg` 赋为 0，把 `smoothSeam` 与 `valid` 赋为 true；随后复制到其他 seam。没有遍历匹配边界对进行实际位置差、法线角或切平面角计算。

- C0：共享索引能保证同一个顶点只有一份位置数据，因此该处位置相同是结构事实；报告中的 `0.0f` 不是测量结果，也没有验证 seam 对应顺序/winding。
- G1：没有计算边界两侧曲面导数或切平面差，不能报告 0°。共享顶点 normal 由全网格面法线累加再归一化，不能作为 G1 连续性的独立证明。
- `smoothSeam=true` / `valid=true` 是手工赋值，不是 validator 输出。

历史验收文档声称 Boundary Contract PASS 的部分指标不可采信；应标为 **NOT MEASURED**。

### 5.3 Skeleton 使用审计（FACT）

函数签名接收 `const Skeleton& skeleton`，但函数体没有读取 `skeleton`。生成路径由 `BodyJoints` 构造，顶点权重也由硬编码骨骼 ID 和局部插值计算。因此实现是 Landmark-driven，不是 Rig-aware 的几何求解。

### 5.4 P1-A 报告数据（REPORTED）

P1-A 文档记录 host 9/9 suites、真机 870 vertices / 1,664 triangles / components=1 / boundary=80 / nonmanifold=0 / degenerate=0 / bad normals=0、生成约 2.419ms，约 46–55 FPS。它是一个局部 PoC，开放边界属于局部范围；但项目方最终视觉 Gate 为 REJECTED。工程合法性通过不能抵销视觉失败。

缺陷档案列出：躯干外扩筒状、胸腹腰盆体积不足、肩胸暗线和硬边、肩部贴片感、上/前臂圆管感、肘折痕、线框/法线不均匀。此清单引用自项目方报告；本次没有读取实际 PNG 像素，故不冒充独立视觉复核。

## 6. Mesh / Topology / Skinning 审计

### 6.1 MeshTopology 的覆盖范围（FACT）

`MeshTopology.cpp` 检查精确浮点位置重复、单位/有限法线、triangle edge 引用计数、边界边、边被超过两面共享的 non-manifold edge、零面积三角形和连通分量。重复顶点只按精确 float 相等，不包含 epsilon weld 诊断。

当前实现没有检查：non-manifold vertex/bow-tie、triangle winding 一致性/外法线方向、自交面、相交重叠壳体、UV/颜色有效性、关节语义权重、法线接缝，或者四边形 Edge Flow 质量。components 分析也不计入没有任何共享边的孤立顶点。

### 6.2 MeshValidator 的覆盖范围（FACT）

`MeshValidator.cpp` 检查顶点/索引非空、索引范围、位置/法线有限、法线单位长度、权重和、负权重、非零权重骨骼 ID<22、degenerate triangle 比例、AABB 和 SubMesh range。它不证明水密、manifold、法线朝外、自交安全或动画变形质量。

### 6.3 Skeleton / Skinning（FACT）

22 骨 ABI 在 `Skeleton.h` 冻结为 ID 0–21；肩骨语义是 Chest→Shoulder joint，上臂骨是 Shoulder→Elbow，下臂骨是 Elbow→Wrist。坐标约定是右手系、+Y Up、+Z Forward、+X Character Left、米制。

`bindLocal` 当前仅使用平移，bindGlobal 按 parent 链相乘，inverseBind 从 bindGlobal 求逆。`Skinning.cpp` 是 Linear Blend Skinning（LBS）：顶点位置对 bone palette 变换加权，法线以加权 `mat3(palette)` 变换后归一化。没有 DQS 或 corrective deformation。

当前 LBS 的肩部体积变化、肘部内侧压缩、前臂扭转 candy-wrapper、外侧拉伸不足等属于已知能力边界；增加权重平滑不能代替形态 corrective。

## 7. Renderer / Runtime / 线程资源审计

- `RenderThread::ensureResources()` 在 EGL/GLES 上下文已建立后，同步调用 `generateHumanoid()`、`generateAnatomyUpperBody()` 和 `BodyPatchGenerator::generate()`；几何生成因此在 RenderThread 上执行，不符合 Foundation 06 中 Geometry Worker 与 RenderThread 职责分离的目标契约。
- `SkinnedGpuMesh::upload()` 创建 VAO/VBO/IBO；GL 资源操作应只在 RenderThread 且有效 Context 下执行，这个所有权方向是正确的。
- `RenderThread::releaseGpuResources()` 当前释放 avatar/shadow/P1-A buffers 和 bone UBO，但未调用 `anatomyMesh_.release()` / `anatomyLines_.release()`。在 SurfaceDestroyed 或 Context 恢复路径上可能遗留旧 GL name 状态；需要独立修复与 EGL Context loss 测试。
- `EglContextManager` 能初始化/销毁 EGL display/context/surface，并提供 recreate 路径；CPU `MeshData` 持久缓存和全 GPU 资源恢复路径仍需要明确契约。
- 当前没有 `GenerationVersion`、过期生成取消、不可变 CPU MeshData handoff 的正式路径。RenderThread 阻塞式生成也会把初始化耗时与 Context 生命周期耦合。
- 当前 GPU Skinned Vertex packing 为 64 bytes，`SkinnedGpuMesh` 上传 pos/normal/joints/weights/color；UV 没有进入当前 skinned GPU layout。若新拓扑阶段需要 UV/tangent 或额外 region/seam 数据，应先定义 CPU-only metadata 与正式 GPU attributes 的边界，不宜顺手扩大现有 ABI。

## 8. 失败根因分层

| 路线 | 表层表现 | 结构性根因 | 本次判断 |
|---|---|---|---|
| M11 | 完整角色有拼装/部件分离感 | 各自生成的独立 Part 最后只拼接 buffer；截面曲面缺少胸肩/腋下的共同形态解算 | 不适合作为躯干/肩部最终连续表面的直接路线；Sweep/Loft/Face/Hair/Clothing 子系统可复用 |
| M12 | 网格完整水密但形态不足 | 单一标量场解决连通性，场的低维原语组合仍不能自动表达胸大肌、肩胛、肋弓、腹壁和肌群空间层次 | 场表示有效但当前 shape model 不足；不可将 Surface Nets 等同于解剖模型 |
| P1-A | seam/拓扑局部指标好，视觉不合格 | 5 站位 Loft + 肩部 bridge + 管状 Sweep 把局部参数化做得更复杂，却没有建立共享的形态控制和体积语义；报告数据又把 seam 指标直接赋值 | 应存档，不在其上继续加补丁 |

**总根因：过去路线从几何算法出发，再用有限参数拟合人体；下一路线必须先建立一个共享、分层、具备解剖语义的人体 Shape Model，让所有几何表示服从同一套形态约束。**

## 9. PNG / 视觉证据审计边界

仓库树包含以下证据路径（路径存在于远程 tree，实际二进制像素未读取）：

- M11：`docs/phases/M11-anatomy-visual-rebuild/evidence/frame-m11c-*`、`frame-m11d-*`、`frame-m11e-*`、`frame-m11f-*` 等。
- M12：`docs/phases/M12-unified-human-geometry/evidence/dv{0,1,2}_preset{0..7}.png`，共 24 个命名视图。
- P1-A：`docs/phases/P1A-geometry-poc/evidence/clay_preset0..7.png`、`wire_front.png`、`wire_shoulder.png`、`normals_front.png`、`normals_shoulder.png`、`action_elbow_*.png` 以及 `iter2/iter3` 图。

本次 GitHub connector 可以读取源代码和 Markdown，但无法读取仓库中的二进制 PNG；直接打开 GitHub/raw 图片也未得到可读图像。因此：

- 文件清单与报告路径：**已核对**。
- 报告中记录的相机、材质、动作、设备、性能：**已阅读，但属于 REPORTED**。
- 对正面/侧面/45°/wireframe/normals PNG 的实际像素/曲率/轮廓独立判断：**NOT VERIFIED**。
- P1-A 失败判断：来自项目负责人已归档的 REJECTED Gate 与缺陷报告，不伪称本次独立看图。

下一轮审查必须提供 PNG 原文件或本地路径，再以相同机位逐张查看；文件名或报告摘要不能替代视觉检查。

## 10. 审计结论

1. M11 证明程序化完整角色生成链可落地，但不是统一连续人体表面方案。
2. M12 证明统一隐式场 + Surface Nets 可生成单一水密表面，但没有证明它具备自然解剖形态或适合动画的拓扑。
3. P1-A 的 C0/G1 BoundaryReport 不可信，Skeleton 参数未实际参与生成，不能继续沿用其 PASS 结论。
4. 22 Bone ABI、C++17、CPU MeshData、GLES3 渲染器和固定视觉预览体系是应保留的基础。
5. P1-A 已 REJECTED，停止于此。任何新路线应从独立设计与新 PoC 开始，不能自动启动 P1-B。
