# PAE 人体几何生成架构设计

文件：`docs/architecture/parametric-human-model/02-procedural-geometry-architecture.md`
任务：P1-VR / Deliverable R4
日期：2026-11-25
状态：VR-0 架构设计；生产实现须等待负责人正式签发 P1-VR 补充授权。

## 1. 架构总览

```text
CharacterDefinition（唯一可编辑人体语义输入）
  → ResolvedCharacterState（不可变派生 Anthropometry/Landmarks/BodyJoints/Skeleton）
  → GeometryBuildRequest（generator/topology/detail/template/region mask）
  → Procedural Surface Construction
  ├─ Variable-profile Sweep
  ├─ SurfacePatch / TopologyTemplate
  ├─ Local Shape Basis
  └─ Head / Hand / Foot structured generators
          ↓
Local Geometry Fusion
  ├─ bounded local SDF
  ├─ shared boundary evaluator
  └─ local concavity / correction
          ↓
Canonical Skin Mesh
  ├─ unified topology
  ├─ CPU sidecar region/topology metadata
  ├─ Top-4 weights
  └─ topology / normal / self-intersection reports
          ↓
Render Mesh
  ├─ renderToCanonical
  ├─ current per-triangle UV chart
  └─ existing 64B base VBO + parallel UV VBO
          ↓
Existing Android Render Pipeline
  └─ 22 bones / UBO[22] / GLES3 / RenderThread
```

## 2. 当前实现与目标差距

已核实的生产事实：

- `RenderThread::ensureCanonicalGeometry` 只构造默认 `BodyParameters`，覆盖 `detailLevel` 后调用 `canonical::generateContinuousSkin`：`native/renderer/RenderThread.cpp:1190-1212`。
- 五层 `shape::v2::CharacterDefinition` 是真实代码，但只在 Host PoC target 编译：`tests/CMakeLists.txt:95-118`。
- `AnatomicalLandmarks::placed` 默认没有空间点，生产 Canonical 使用 `BodyJoints` 和 head field，并在 `buildHeadField(..., nullptr)` 中不请求 `HeadLandmarks`；权重使用硬编码六区 switch：`native/procedural/shape/v2/CharacterDefinition.cpp:69-89,203-240`、`native/procedural/humanoid/canonical/ContinuousSkinGenerator.cpp:211-273,427-441`。
- Canonical 的躯干、四肢、手指和脚趾全部来自椭球、superellipsoid、胶囊和 smooth union：`ContinuousSkinGenerator.cpp:88-198`。
- 现成 `BodyPatchGenerator` 已有非圆截面、平行传输 frame 和肩胸 bridge，但只服务 mode 2，不是 Canonical：`native/procedural/character/BodyPatchGenerator.cpp:97-357`。
- `CanonicalSkinGenerator` 的边界重采样/焊接代码存在，但生产 Canonical 没有调用：`native/procedural/humanoid/canonical/CanonicalSkinGenerator.cpp:132-284`。

结论：问题不是“SDF 参数还不够多”，而是生产上游没有人体语义、稳定区域拓扑和局部解剖表达。继续增加椭球半径无法解决面部、手足和关节形态。

## 3. 曲面构造

### 3.1 可变截面公式

躯干和四肢采用：

\[
P(s,\phi)=C(s)+r_1(s,\phi)e_1(s)+r_2(s,\phi)e_2(s)
\]

局部形态项：

\[
P'=P+\sum_i a_i\phi_i(s,\phi)n(s,\phi)
\]

实现要求：

- `C(s)` 由 `AnatomicalLandmarks` 和样条曲线产生，不由球心/胶囊端点隐式决定。
- `e1/e2` 使用平行传输或解析局部 frame，禁止固定 cross-product 翻转。
- `r1/r2` 是宽深独立的多控制点截面，不是同一个椭圆半径。
- 正面、背面、左右侧可以有不同的截面控制点。
- 局部形态函数有有限支撑域和 C1 边界，未声明区域位移为零。
- seam 两侧调用同一个 boundary evaluator，不允许分别近似。

Frame 与数值稳定性约束：

- 初始 frame 由解剖标志点和关节局部坐标系确定；
- 使用 Parallel Transport / Rotation-Minimizing Frame；
- 明确 tangent 反向、近零曲率、重复点和急弯的处理；
- 左右侧使用统一 side convention；
- 检查 frame orthonormality、相邻 tangent 点积和 Jacobian 符号；
- frame 翻转或 Jacobian 退化立即停止，不得只靠最终 mesh 自交检查兜底。

### 3.2 中心线与截面锚点

每个区域至少定义：

- 中心线控制点；
- 起始/结束边界环；
- 中间解剖截面高度；
- 每个截面的宽度、深度、中心偏移和局部形态系数。

例如躯干锚点：

`crotch/hip → iliac crest → waist → costal margin → sternal angle → chest → clavicle → neck base`。

复杂区域不能强制使用单值径向椭圆：肩带、腋窝、胸廓与上臂交界使用独立 Surface Patch 和闭合周期样条/二维控制曲线。VR-1A 的 12–16 个语义控制点只是原型起点，必须通过光滑性、细节分辨率和截面自交实验后才能成为正式拓扑分辨率。

### 3.3 局部形态基函数

使用 PAE 自己定义的解析基函数：

- `ProfileBasis`：沿 `s` 改变宽深和中心线；
- `ReliefBasis`：沿法线产生胸肌、三角肌、臀大肌、股四头肌、腓肠肌等局部起伏；
- `ConcavityBasis`：腋窝、腹股沟、臀沟、肘窝、腘窝等凹陷；
- `CouplingBasis`：跨相邻区域的低幅连续修正，必须声明影响区域。

不导入外部 `.target`、blendshape 或顶点 delta。若未来使用数据驱动形态基，必须绑定 PAE 自有 topology template 和 local vertex ID。

每个局部 basis 必须声明 support、amplitude、边界值/一阶导数连续性和允许组合集合。组合测试必须覆盖曲面翻转、局部折叠、过度凸起、曲率突变、自相交和 Jacobian 符号变化。

## 4. 局部 SDF 使用政策

SDF 不再是全身形态来源。只允许以下用途：

1. 局部凹面修正：腋窝、腹股沟、臀沟、肘窝、腘窝。
2. 小范围连接补片：肩胸、腕、踝、颈根等边界过渡。
3. 面部局部负向结构：鼻孔、口角、眼睑与面颊交界。
4. 研究/对照路径：保留现有全身 SDF 作为 before/after，不进入最终唯一生产链。

工程约束：

- 每个 SDF 操作必须有 `RegionId` 和 AABB；
- 默认 blend `k <= 0.006 m` 用于面部/手足，`k <= 0.012 m` 用于较大躯干过渡；
- 超过该范围必须提供视觉理由和 review；
- 禁止再次把几十个椭球/胶囊做全身 smooth union；
- 禁止用 SDF 生成最终面部、手指、脚趾拓扑。

## 5. 模块与文件边界

### 5.1 新增生产模块

建议新增子契约/模块（`CharacterDefinition` 本体仍在 `shape/v2` 原地扩展）：

- `native/procedural/character/Anthropometry.h/.cpp`
- `native/procedural/character/AsymmetryParameters.h/.cpp`
- `native/procedural/character/TopologyPolicy.h/.cpp`
- `native/procedural/character/HumanShapeParameters.h/.cpp`
- `native/procedural/character/ResolvedCharacterState.h/.cpp`
- `native/procedural/character/GeometryBuildRequest.h/.cpp`
- `native/procedural/character/LodCorrespondence.h/.cpp`
- `native/procedural/character/SkinShellContract.h/.cpp`
- `native/procedural/character/AnatomicalLandmarks.h/.cpp`
- `native/procedural/character/AnatomicalRegions.h/.cpp`
- `native/procedural/character/TopologyTemplate.h/.cpp`
- `native/procedural/character/SurfacePatch.h/.cpp`
- `native/procedural/character/VariableProfileSweep.h/.cpp`
- `native/procedural/character/LocalShapeBasis.h/.cpp`
- `native/procedural/character/PatchAssembler.h/.cpp`
- `native/procedural/character/CanonicalBodyGenerator.h/.cpp`

这些模块只依赖 C++17、`MeshData`、`Skeleton` 和现有数学工具，不依赖 GL、Android 或外部资产。

### 5.2 现有模块处理

| 现有文件/类 | 处理 |
|---|---|
| `native/procedural/shape/v2/CharacterDefinition.*` | 唯一人体语义权威；原地升级 schema version，吸收 Anthropometry/Regional Shape/Asymmetry 与 TopologyPolicyRef；实际 TopologyPolicy 属于 GeometryBuildRequest |
| `native/procedural/shape/v2/SharedSkinEnvelope.cpp` | 复用 C2 比例曲线、区域权重和凹面约束思想；不直接作为全身生成器 |
| `native/procedural/character/CharacterDefinition.h` | legacy P1-A patch parameters；明确 legacy-only，后续改名，不形成第二人体语义权威 |
| `native/procedural/character/BodyPatchGenerator.cpp` | VR-1A 改造点；旧实现保留为可选 baseline；新实现必须真正生成左右两侧 |
| `ContinuousSkinGenerator.cpp` | 从 SDF 主体构建改为 Canonical 编排；旧 SDF 路径保留为对照 |
| `CanonicalSkinGenerator.cpp` | 扩展 boundary/weight/render mapping；现成 weld helper 不满足新 C0/G1 合同，必须改造 |
| `HeadAnatomy.cpp/.h` | VR-2 由结构化 `HeadFaceGenerator` 替换生产形态；旧实现保留为对照 |
| `FaceGenerator.cpp/.h` | 复用眼部/眉/鼻/口辅助几何思想，不作为 Canonical 形态来源 |
| `Skinning.cpp/.h` | 保留 LBS、CPU/GPU 一致性和 UBO；只增加 region-aware 权重生成 |
| `SkinnedGpuMesh.cpp/.h` | 保留 64B base + UV attribute 5，不改 packing |
| `RenderThread.cpp/.h` | 保留命令队列、缓存、上传和绘制；增加 shape request/cache key、per-mode skeleton 恢复和 mode-2 weight view |

### 5.3 构建边界

- 新模块加入 `native/CMakeLists.txt` 的 `PAE_CORE_SOURCES`，Host 与 Android 使用同一实现。
- `shape/v2/CharacterDefinition` 从 host-only 合同原地升级为生产唯一语义权威；历史 Host PoC tests 保留，避免第二套 schema。
- 不新增 JNI 方法；内部固定 `CharacterDefinition` 通过现有 `generateContinuousSkin` 边界进入生产。

## 6. 区域生成设计

### 6.1 躯干、骨盆与臀部

- 使用多环非圆截面，前胸、侧肋、背部、腰、骨盆分别控制。
- 胸廓不是 chest superellipsoid；肋骨下缘、胸骨、肩胛区有独立轮廓锚点。
- 腹部使用 linea alba、umbilicus 局部 relief/concavity。
- 骨盆和臀部使用 hip、iliac crest、gluteal fold、crotch 边界，禁止两个臀球与躯干 union。
- 肩带由 clavicle、acromion、axilla 与上臂根环共同构造。

### 6.2 肩、上臂、肘、前臂、腕

- 上臂使用 biceps/triceps/deltoid 局部形态。
- 肘部使用内侧/外侧上髁和 olecranon 轮廓，不用球体。
- 前臂区分屈肌/伸肌侧与腕部 taper。
- 肩胸连接使用共享边界 patch 和局部 concavity，不叠加独立肩球。

### 6.3 髋、大腿、膝、小腿、踝

- 大腿区分股四头肌、内收肌、腘绳肌方向。
- 膝部包含髌骨前凸和腘窝后凹，不用膝球。
- 小腿区分腓肠肌/比目鱼肌与踝部 taper。
- 髋/腹股沟使用连续边界和局部负向修正。

### 6.4 头部与面部

使用固定语义控制网格和局部 patch：

- cranial vault、forehead、temple、occiput；
- brow ridge、orbit、zygomatic、cheek、mandible、chin；
- nose dorsum/tip/ala/columella；
- upper/lower vermilion border、philtrum、mouth corner；
- ear helix/antihelix/concha/tragus。

眼部策略：

- Canonical Skin Shell 保持封闭；
- 眼眶凹陷和上/下眼睑是连续皮肤拓扑；
- eyeball/iris/pupil 作为辅助几何，不计入 Skin Shell 封闭性；
- 不使用两个突出球体代替眼睛；
- 不添加眼球或眼睑骨骼，继续使用 Neck/Head 权重和局部 corrective 入口。

嘴部策略：

- 上/下唇、唇峰、口角、人中由结构化表面控制；
- 口裂使用窄的辅助 inset 或局部负向结构，不使用黑色椭圆贴片。

### 6.5 手部

- palm template：掌弓、thenar、hypothenar、MCP row；
- 四个手指各含 proximal/middle/distal 三段；
- thumb 有独立根部方向和三段结构；
- 指蹼为局部 concavity，指缝有真实空间分隔；
- 指尖和指甲使用局部拓扑/辅助 relief；
- 22 骨骼限制下，所有 digit 顶点权重归 Hand；VR-3 只测试腕/手整体旋转，不宣称独立指骨动画。

### 6.6 足部

- heel、arch、metatarsal、forefoot、hallux + 四个 toe；
- 前足掌宽于后足，足弓有真实纵向/横向曲率；
- 趾间有 web space 和独立 tip；
- 踝部与小腿共享边界；
- 22 骨骼限制下使用 Foot/Toe 权重，不新增 toe bones。

### 6.7 Canonical Skin Shell 边界规则

VR-2/VR-3 前必须固定：

- Skin Shell 包含哪些连续皮肤区域；
- eyeball/iris/cornea、teeth、tongue 等为辅助几何，不计入 Skin Shell 封闭性；
- eyelid 必须与面部皮肤保持真实拓扑连接；
- nostril、oral fissure、ear canal、nasal/oral cavity 明确为封闭凹陷、显式开放边界或独立内部几何；
- 所有开放边界登记 `BoundaryId`、用途、相邻组件和验收规则；
- 禁止局部 SDF 负向操作产生未管理裂口；
- 手指/脚趾是独立分支拓扑，不是在掌面/足面贴圆管；
- 独立 digit 几何不代表独立 digit 骨骼动画。

## 7. Patch 组装与统一 Canonical

### 7.1 Boundary Contract

每个 patch 边界记录：

- `boundaryId`；
- 区域 ID 和相邻 patch；
- 环顶点数/顺序/winding；
- 规范化弧长参数；
- 位置、切线、法线；
- smooth/hard seam tag；
- 共享求值器 ID。

规则：

- 所有共享边界位置误差 `<= 1e-4 m`；
- 不允许 T-junction；
- 不同密度边界通过共享弧长重采样；
- 最终 Skin Shell 除显式注册 crop 外无开放边界；
- 普通 smooth seam 报告法线/切平面连续性，但不能替代肩根 OQ-03 口径；
- **肩根 crop 环**同时记录两类研究诊断：现有 OQ-03 相邻 quad 外法线夹角，以及专家建议的 seam 两侧对应采样点法线夹角；定义域 `[0°,180°]`，先验证 winding，再统计 max/mean/p95、纳入 quad 数和角度样本数；
- OQ-03 口径/阈值存在治理冲突，5° 暂不作为 VR-1A 硬验收；负责人确认前不得标记 PASS；
- 肩根角度必须从最终 mesh 的 face/triangle winding 独立计算，不能复用 `BodyPatchGenerator::BoundaryReport` 中硬编码的 0；
- 无 shoulder-root crop ring 时记 `NOT APPLICABLE`，不得记 PASS；
- 若未来要改为“seam 两侧”或其他 cross-boundary 指标，必须由负责人另行决定，不能由实现侧替换 OQ-03。

现有 `CanonicalSkinGenerator::weldBoundaryLoops` 只接受等密度边界，且 position 容差为 `1e-3 m`，不能直接作为上述合同的实现。VR-1B 前必须扩展为共享 `BoundaryEvaluator`、非等密度弧长重采样、切线/法线元数据和 `1e-4 m` C0 验证，并增加对应测试。

### 7.2 组装算法

1. 各 patch 在局部拓扑模板上生成顶点和面。
2. 对共享边界使用同一 `BoundaryEvaluator` 生成 canonical boundary vertices。
3. 每个共享边界只创建一次 Canonical Vertex ID。
4. patch 内部顶点追加，索引重映射到共享边界。
5. 合并 seam 权重：按 BoneId 累加、Top-4、归一化。
6. 重算法线并检查 winding。
7. 运行 topology、weights、bind-pose self-intersection。
8. 生成 CPU sidecar metadata。
9. 调用现有 `buildRenderMesh` 生成 Render Mesh 和 `renderToCanonical`。

### 7.3 Sidecar Metadata

不改变 `Vertex` 或 64B GPU packing。新增 CPU sidecar：

```cpp
struct CanonicalVertexMetadata {
    uint32_t stableId;
    uint32_t templateVersion;
    RegionId region;
    uint32_t templateId;
    uint32_t localVertexId;
    uint32_t boundaryMask;
    uint8_t side;
};
```

Render Mesh 通过 `renderToCanonical` 继承 metadata。`stableId` 是跨参数/细节等级的持久语义 ID；单次 `MeshData.vertices` 下标只是 build-local Canonical index，两者不得混用。跨 detail level 使用 `templateVersion + templateId + localVertexId` 重映射，并测试 ID 稳定性。

LOD 新顶点必须记录 refinement lineage：

```text
ParentFaceStableId / ParentEdgeStableId / ParentVertexStableId
+ BarycentricCoordinates
+ RefinementPath
+ LODLevel
```

建立 LOD0 ↔ LOD1 ↔ LOD2 correspondence map；不兼容 topology version 必须生成显式迁移映射，禁止假装旧 ID 仍有效。

## 8. 权重与变形

- 使用 `JointDeformationRegions` 限制每个区域允许的骨骼 mask。
- 在区域内按骨段距离和平行传输坐标计算平滑 falloff。
- 肩、肘、髋、膝必须有过渡带，禁止单骨硬切。
- 跨肢体和对侧肢体默认不允许共享影响。
- 保持每顶点最多 4 个骨骼、非负、和 `1 ± 1e-3`。
- 继续使用现有 `SkinPalette = GlobalPose * InverseBind` 和 LBS。
- Shape basis 与 pose corrective 分离；VR-1 不实现完整 corrective。
- VR-4 验证 Bind、arm raise、shoulder abduction、elbow、hip、knee、wrist rotation、ankle rotation。

验收分为三层：①权重合法性；②明确角度/轴/坐标系的基础动作；③局部形变质量（肩部体积、腋窝凹陷、关节塌陷、异常凸包、穿插、左右一致性）。Top-4 合法不得替代动作和局部形变验证。

## 9. Render Mesh、UV 与 Android

- 保留当前每三角形独立 UV chart，先证明新 Canonical 可走既有 GPU 路径。
- 保留 `renderToCanonical`、base 64B VBO、UV attribute 5、独立 UV VAO。
- 保留 transform-feedback GPU/CPU 校验。
- `RenderThread::ensureCanonicalGeometry` 的接入点保持不变，但输入改为内部 `GeometryBuildRequest`。
- cache key 从 detail level 扩展为 `schemaVersion + generatorVersion + definitionHash + detailLevel`。
- mode-2 P1-A cache 同样需要 definition/detail key；质量变化必须失效对应 cache，不能继续上传旧 patch。
- mode-2 per-mode cache 由 RenderThread 持有，payload 必须包含 matching definition/hash、mesh、`Skeleton` 和 `BodyJoints`；cache hit 时原子恢复并重新初始化/校验 `MotionRuntime`，不能只恢复 mesh。
- mode 切换涉及共享 `skeleton_`/`motion_` 时，必须恢复或重新初始化该 mode 的 skeleton/joints；不能把 mode-3 skeleton 留给 mode-2。
- mode-2 当前没有 Weight Heatmap 分支；VR-1A 必须在 `RenderThread::drawFrame` 增加 `debugView=5` 的 `skinnedProgram_`/`uDebugMode=5` 路径，并加入真机 capture 测试。
- 不改变 JNI 签名、previewMode 0–3 或 debug view 0–7。
- 新局部近景可通过现有 free-camera 命令采集，不先扩 UI。

## 10. 方案比较

| 方案 | 自然度 | 解剖控制 | 拓扑稳定 | C++17/Android 成本 | 结论 |
|---|---|---|---|---|---|
| A 参数曲线 + 可变截面 | 中高 | 高（躯干/四肢） | 高 | 中 | 作为主体曲面 |
| B 局部结构化拓扑模板 | 高 | 高 | 高 | 中高 | 面部/手足必需 |
| C 局部形态基函数 | 高 | 高 | 依赖基础拓扑 | 中 | 叠加在 A/B 上 |
| D SDF + 参数曲面混合 | 取决于局部使用 | 中 | 中 | 中 | 只做局部融合 |
| E Surface Patch + 统一 Canonical | 高 | 高 | 高 | 中高 | 最终组装主线 |

选择 E 为统一架构，A/B/C 为区域生成手段，D 为受限局部工具。不是“只选一个算法”，而是明确各层职责。

## 11. 为什么全身 SDF 和固定基础网格都不能直接照搬

### 全身 SDF 的问题

- smooth union 会抹平眼睑、指缝、鼻孔、唇裂等高频解剖结构；
- 原语形状先验过强，胸廓、肩带、手足容易被读成球和管；
- 全局场没有稳定 RegionId 和顶点 ID；
- Surface Nets 拓扑随场变化，难以建立面部/手足专用拓扑；
- 当前 14–18 mm blend 与 facial feature 尺度接近，直接产生 blobs。

### 成熟模型固定基础网格的原因

SMPL-X、FLAME、MANO、GHUM 的 shape/pose basis、joint regressor 和 skin weights 都按固定顶点编号索引。没有固定 correspondence，顶点 delta 无法跨网格迁移。

PAE 的对应方案是：**自己用代码定义固定区域拓扑模板**，参数只改变顶点位置，不随意改主要连接关系；局部形态基绑定 PAE 自己的 `templateId + localVertexId`。这既不导入外部网格，也获得稳定拓扑。

## 12. 性能与风险

当前 P1 基线：约 45.5–46.9 FPS、p95 23.22–24.32 ms、115,300 triangles、TOTAL PSS 244,785 KB、Canonical 总生成约 83.8 s。新方案应优先减少高成本全局 SDF 采样，但结构化细分和自交检查可能增加成本。

负责人已批准同设备基线下降不超过 20% 的临时工程 stop guardrail，并要求分项记录生成、组装、权重、GPU、帧时间和内存；该 guardrail 不是 Gate 阈值，也不替代正式产品预算。超过时先优化采样/细分/自交宽相位，不降低视觉要求。

主要风险：

- 共享边界 T-junction 或法线裂缝；
- 局部 SDF 再次产生 blobs；
- 面/手/足模板复杂度失控；
- 22 骨骼无法表达独立指/趾运动；
- 稳定 ID 跨 detail level 失效；
- Host 与 Android hash 不一致；
- 生成时间或内存显著回归。

## 13. 停止条件

出现以下情况停止当前阶段：

- 需要修改 BoneId、UBO、64B packing、JNI ABI；
- 需要替换 RenderThread 架构；
- 新拓扑无法保持统一 Canonical；
- 边界位置/法线合同无法满足且无法定位根因；
- 局部方案仍读作球/管拼接；
- 真机与 Host 明显不一致；
- 性能超过批准的 stop guardrail；
- 授权文件未正式签发或超出白名单；
- CharacterDefinition 与 BodyJoints 出现双向或多重权威；
- LOD Stable ID 无法建立可靠映射；
- frame 翻转或局部 Jacobian 退化；
- 结构化 Patch 必须依靠大量重叠 SDF 才能融合；
- 基准人体参数未批准，或性能超过已批准的临时 guardrail；
- OQ-03 阈值/口径未确认却被当作硬验收；
- Weight Heatmap 与实际权重不一致；
- cache mesh 与 Skeleton/BodyJoints 不匹配。

## 14. 外部方法依据

本架构借鉴的是数学/参数化思想，不导入模型：

- SMPL-X：shape/pose 分离、joint regressor 思想、LBS、局部 corrective；
- FLAME：固定面部拓扑、face shape space、眼球与眼睑分离、jaw pose；
- MANO：固定手部拓扑、局部 geodesic pose corrective、shape/pose 分离；
- GHUM：区域 mask、非负 Top-4 权重、非线性区域耦合；
- MakeHuman/MPFB2：语义参数 → 局部形态目标的组织方式；
- MB-Lab：参数配置与生成流程分离；
- OpenSim：关节中心、解剖标志点和关节区域语义；
- GNM Head：面部/内部结构分区、identity/expression/pose 分离、UDIM 区域组织。

具体源码、版本和许可证见 R1/R2 报告。
