# P1-VR-001 — Anatomical Form Recovery Plan

文件：`docs/phases/P1-visual-recovery/P1-VR-001-Anatomical-Form-Recovery-Plan.md`
任务：P1-VR / VR-0
日期：2026-11-25
状态：正式工程设计计划；生产实现尚未开始。AI 不宣布 Gate PASS，不代签授权文件，不自动 commit/push。

## 1. 目标

在保留现有 C++17 / Android NDK / OpenGL ES 3.0 技术路径的前提下，把 PAE 从“全身 SDF + 椭球/胶囊平滑并集”的机器人灰模，重建为自然、可信的半写实成人三维人体。

最终对象不是一份拓扑报告，而是：

- 由 PAE 自主 C++ 规则生成；
- 具有自然成人比例和解剖结构；
- 面部、手掌、手指、脚掌、脚趾具有真实空间结构；
- 躯干、肩带、骨盆、四肢连续过渡；
- 通过现有 22 骨骼 LBS 和 GPU skinning 正确变形；
- 在 Android 真机上以 Clay 和多角度实际渲染验证。

## 2. 当前仓库状态

### 2.1 GitHub 分支

| 分支 | HEAD | 说明 |
|---|---|---|
| `poc-sm/shape-model-v2` | `6e004b2b1e5cf6c6cd60efeb15cc48d16a3a9d2d` | P1 当前实现与证据，参考提交仍是最新 |
| `main` | `c539afbec299ed4a066429ea09f9d3d36e67f74d` | M12-A，未包含 P1 |
| `feature/procedural-character-foundation` | `2ac101fd40e533d2f08776cb6f4e9356cd1b25d1` | 治理/round contract |
| `m11-anatomy-visual-rebuild` | `67ccb3940950623b1a24d6b3e843750c5c4a99b3` | M11 历史 |
| `m12-unified-human-geometry` | `c539afbec299ed4a066429ea09f9d3d36e67f74d` | M12 历史 |

核查方式：`git ls-remote --heads origin` 与 GitHub public branches API；环境无 `gh` CLI。所有分支当前均未标记 protected。

### 2.2 本地工作树

- 路径：`E:/AI-Tools/ProceduralAvatarEngine-poc-sm`。
- 分支：`poc-sm/shape-model-v2`。
- HEAD：`6e004b2b1e5cf6c6cd60efeb15cc48d16a3a9d2d`。
- 已跟踪修改：无。
- 未跟踪：`.workbuddy-ai/`、v6/v7 evidence，以及本轮新增 `docs/research/`、`docs/architecture/parametric-human-model/`、`docs/phases/P1-visual-recovery/`。
- P1-VR 生产代码修改：无。
- VR-1：未开始。
- 本轮 VR-0 性质：纯审计/研究轮，按 ADR-013 为 process note / 治理记录，不是工程里程碑。

### 2.3 P1 已保留成果

- Host CTest 19/19；
- Android `assembleDebug` 成功；
- High detail Canonical 57,652 vertices / 115,300 triangles / 345,900 render vertices；
- GPU/CPU 三组案例通过；
- `2→0→2` quality refresh；
- Surface recreation；
- UV Checker 重建前后 SHA-256 一致；
- 稳定约 45.5–46.9 FPS、p95 23.22–24.32 ms；
- TOTAL PSS 244,785 KB；
- 视觉验收仍 BLOCKED。

## 3. 治理与授权

### 3.1 文件冲突

1. AUTH-08 只授权 `shape/v2` host-only PoC，明确禁止 RenderThread、Android 生产路径和 legacy C++ 修改。
2. P1 v0.2 任务书记录 2026-11-24 负责人另行授权 P1 Android 生产生成链路及白名单。
3. P0 Gate 原状态为 `NOT VERIFIED`，ADR-010 原文要求 P0 Gate 确认后生效。
4. DEC-05/CAS 和默认体型/审美阈值仍有未批准项。
5. 当前用户指令只授权 VR-0 研究/设计，不允许大规模生产重写。

### 3.2 负责人本轮决定

负责人已明确选择：

- 新签 P1-VR 补充授权；
- 确认 P0 Gate 与 ADR-010 生效。

AI 不代签，不修改 AUTH/ADR/DEC/CAS/Gate 文件。正式补充授权签发前，VR-1 生产写入保持 BLOCKED。

### 3.3 补充授权必须明确

补充授权须以精确路径/函数清单 supersede AUTH-08 的 FB-5/FB-6/FB-7/FB-11，至少覆盖：

- 原地扩展 `native/procedural/shape/v2/CharacterDefinition.h/.cpp`；
- 新增 `native/procedural/character/` 下 Anthropometry、AsymmetryParameters、TopologyPolicy、HumanShapeParameters、AnatomicalLandmarks、AnatomicalRegions、TopologyTemplate、SurfacePatch、VariableProfileSweep、LocalShapeBasis、PatchAssembler、TorsoShoulderPatchGenerator、CanonicalBodyGenerator；
- 修改 `BodyPatchGenerator.h/.cpp`、`ContinuousSkinGenerator.cpp`、`CanonicalSkinGenerator.cpp/.h`、`HeadAnatomy.cpp/.h`、`FaceGenerator.cpp/.h`、`native/skeleton/weights/**`；
- 修改 `RenderThread.cpp/.h` 的 mode-2 生成、cache key、per-mode skeleton 恢复和 `debugView=5` weight 分支；
- 修改 `native/CMakeLists.txt`、`tests/CMakeLists.txt`，新增 `tests/unit/vr_*`；
- 写入 `docs/phases/P1-visual-recovery/evidence/**` 和本轮报告；
- 保持 BoneId、UBO、64B packing、JNI、RenderThread 命令/生命周期架构冻结；
- 不允许外部模型/形态数据/训练数据；
- 未另行授权时不 commit/push。

## 4. 当前代码调用链

```text
MainActivity.debug_generator=3 / BodyMode
  → NativeBridge.setPreviewMode
  → jni_bridge.cpp
  → EngineRuntime::setPreviewMode
  → RenderThread::postPreviewMode
  → RenderThread::processCommands
  → ensureCanonicalGeometry(detailLevel)
  → BodyParameters{detailLevel}
  → canonical::generateContinuousSkin
  → global SDF + Surface Nets
  → assignCanonicalWeights
  → buildRenderMesh
  → SkinnedGpuMesh upload / UV VBO
  → GPU parity
  → previewMode=3 draw
```

关键位置：

- `app/src/main/java/com/pae/host/MainActivity.kt:261-266`
- `app/src/main/java/com/pae/host/NativeBridge.kt:28,81-83`
- `app/src/main/cpp/jni_bridge.cpp:145-148`
- `native/runtime/EngineRuntime.cpp:53-55`
- `native/renderer/RenderThread.cpp:606-612,819-831,1190-1212`
- `native/procedural/humanoid/canonical/ContinuousSkinGenerator.cpp:88-198,416-549`
- `native/procedural/humanoid/canonical/CanonicalSkinGenerator.cpp:132-284,355-414`
- `native/renderer/gl/SkinnedGpuMesh.cpp:54-85,147-232`

## 5. 当前实现的结构性问题

### 5.1 形态先验

- 躯干：pelvis/waist/chest 三个 superellipsoid。
- 肩、膝、踝：显式椭球/球。
- 四肢：tapered capsules。
- 手：一个椭球掌 + 五个单 capsule。
- 脚：两个椭球 + capsule + 五个 toe capsule。
- blend：躯干 18 mm、关节 14–18 mm，面部 16–30 mm。

Smooth union 的 `-k*h*(1-h)` 会在重合表面外推约 `k/4`，大 blend 直接产生圆钝 blob。

### 5.2 解剖语义未进入生产

- 五层 `shape::v2::CharacterDefinition` 只编译进 Host PoC。
- `AnatomicalLandmarks::placed` 默认没有空间点。
- RegionId 没有贯通生产 Canonical。
- `PoseFrame` 的 33 个动作 landmark 不是建模解剖 landmark。

### 5.3 稳定拓扑缺失

- Canonical 由 Surface Nets 从全局场提取，顶点/面随形态变化。
- 现成 boundary welding 代码存在但生产未调用。
- mode-2 `BodyPatchGenerator` 有非圆截面和 shoulder bridge，但只服务局部 PoC。
- 面部、眼睑、指节、足弓需要专用拓扑，不能由全局场隐式产生。

### 5.4 测试缺口

现有测试证明 closure、weights、bind identity、跨肢隔离和 GPU parity，但没有证明：

- 胸廓/腰/骨盆解剖截面；
- 眼、鼻、口、耳结构；
- 五个独立手指/脚趾；
- posed joint volume/self-intersection；
- seam 曲率与 UV distortion；
- 固定模式 3 的自动视觉回归。

## 6. 外部成熟模型研究结论

详细报告见 R1/R2。

| 项目 | 本质 | PAE 借鉴 | PAE 不导入 |
|---|---|---|---|
| MakeHuman / MPFB2 | hm08 固定网格 + target delta | 语义 macro、区域/左右控制、piecewise basis | 网格、target、mirror table |
| SMPL-X | 固定拓扑 + shape/pose basis + joint regressor + LBS | shape/pose 分离、joint 随 shape 更新 | 网格、54 joints、blendshapes |
| FLAME | 固定头部拓扑 + face basis + eyeball | 面部分区、眼睑/眼球分离、jaw | 网格、expression、texture |
| MANO | 固定手拓扑 + shape/pose + 局部 corrective | 手掌/digit 模板、局部 geodesic corrective | 网格、15-joint hierarchy |
| GHUM | 非线性 shape/pose + region mask + Top-4 | 区域耦合、locality、shape-dependent joints | 模型、VAE 数据、63 joints |
| MB-Lab | Blender 模板 + JSON morph + 配置分离 | 配置/生成分离、参数分组 | `.blend`、AGPL 数据库 |
| OpenSim | 骨骼/关节/标志点/测量语义 | landmark、joint frame、anthropometry | skeleton/model/data |

许可证结论：SMPL-X full、FLAME academic、MANO、GHUM model/data 均有非商业、分发或条款不明限制；不得作为 Android 资产。MakeHuman/MPFB2 核心资产虽为 CC0，但直接使用仍属于外部资产，不是 PAE 自主程序化生成。MB-Lab GPL/AGPL 与闭源分发存在风险。

## 7. 修订后的 PAE 架构

```text
CharacterDefinition
  ├─ ProportionProfile / Anthropometry
  ├─ AnatomicalLandmarks
  ├─ AnatomicalRegions
  ├─ HumanShapeParameters / AsymmetryParameters
  └─ JointDeformationRegions
          ↓
Procedural Surface Construction
  ├─ Variable-profile Sweep
  ├─ SurfacePatch / TopologyTemplate
  └─ Local Shape Basis
          ↓
Local Geometry Fusion
  └─ bounded local SDF / shared boundary / concavity
          ↓
Canonical Skin Mesh
  ├─ unified topology
  ├─ CPU sidecar region/topology metadata
  └─ Top-4 weights
          ↓
Render Mesh
  ├─ renderToCanonical
  └─ existing 64B base + parallel UV VBO
          ↓
Existing Android Render Pipeline
```

### 7.1 躯干与四肢数学

\[
P(s,\phi)=C(s)+r_1(s,\phi)e_1(s)+r_2(s,\phi)e_2(s)
\]

\[
P'=P+\sum_i a_i\phi_i(s,\phi)n(s,\phi)
\]

- `C(s)`：landmark/曲线驱动的中心线；
- `e1/e2`：稳定局部 frame；
- `r1/r2`：非圆、宽深独立、随位置变化；
- `φ_i`：胸肌、三角肌、臀大肌、股四头肌、腓肠肌等局部影响函数；
- `n`：局部法线。

### 7.2 面部、手部、足部

使用独立结构化 `SurfacePatch` / `TopologyTemplate`：

- Face：cranium、forehead、orbit、zygomatic、nose、vermilion、mandible、ear；
- Hand：palm arch、thenar、hypothenar、MCP row、五个 digit；
- Foot：heel、arch、metatarsal、forefoot、hallux、四个 toe。

不把全身截面公式套到所有区域。

### 7.3 局部 SDF

只允许：

- 腋窝、腹股沟、臀沟、肘窝、腘窝；
- 小范围肩胸/腕/踝/颈根连接；
- 鼻孔、口角、眼睑局部负向修正；
- before/after 研究对照。

禁止再次用全身椭球/胶囊 SDF 作为最终唯一形态来源。

## 8. 代码保留、替换、新增

### 8.1 保留

- JNI、EngineRuntime、RenderThread command queue/lifecycle；
- Skeleton、BoneId、UBO、LBS；
- SkinnedGpuMesh 64B packing 和 UV attribute 5；
- Canonical/Render 映射；
- GPU parity、atomic upload、cache；
- 现有 tests/evidence。

### 8.2 替换

- `ContinuousSkinGenerator` 的全身 SDF 主体；
- 躯干 superellipsoid、关节球、四肢胶囊；
- `HeadAnatomy` 的 primitive-union 最终形态；
- 手指/脚趾单 capsule；
- 仅最近骨段距离的权重。

### 8.3 新增

- 在 `shape::v2::CharacterDefinition` 原地扩展 `Anthropometry`、`AsymmetryParameters`、`TopologyPolicy`、`HumanShapeParameters`，不新增并行 `HumanShapeDefinition`；
- landmark resolver、region graph；
- `TopologyTemplate`、`SurfacePatch`、`VariableProfileSweep`；
- `LocalShapeBasis`、`PatchAssembler`；
- `CanonicalBodyGenerator`；
- `HeadFaceGenerator`、`HandGenerator`、`FootGenerator`；
- Canonical sidecar metadata；
- 对应 Host tests 和 Android capture。

### 8.4 暂停

- 头发、服装、PBR、SSS；
- 表情系统；
- 学习模型/PCA/VAE；
- 外部资产；
- 完整 corrective；
- 指/趾独立骨骼动画。

## 9. 详细阶段计划

### VR-0：架构与授权确认

交付：

- 最新仓库/分支报告；
- 治理冲突与负责人决定；
- 当前代码调用链和差距分析；
- R1–R6 研究报告；
- 本计划；
- 受保护文件清单。

当前状态：研究和文档完成；补充授权待正式签发。

### VR-1A：躯干—肩部最小原型

目标：证明新参数化曲面优于旧 P1-A，而不是先重写全身。

范围：thorax、abdomen、pelvis 上半、shoulder girdle、axilla、左右 upper-arm root；必须真正生成双侧，不能复用当前单臂实现。

接入：Android mode-2，保持局部 crop boundary 显式注册；旧 P1-A 保留为可选 baseline。

交付：

- 新 bilateral generator；
- landmark/region/shape basis；
- Host boundary/weight/parameter tests；
- mode-2 `debugView=5` Weight Heatmap 实现；
- per-mode cache key、质量失效和 skeleton/joints 恢复；mode-2 cache payload 必须包含 matching definition/hash、mesh、Skeleton/BodyJoints，并由 RenderThread 原子激活；
- Android Clay/Wireframe/Normal/Weight 截图；
- `0→1→2→3`、quality refresh、Surface recreation 回归；
- 与旧 P1-A/P1 Canonical 同机位对比。

验收：C0 `<=1e-4 m`；肩根 crop 环按 OQ-03 固定口径独立测量，max `<=5°`，无 crop ring 时 `NOT APPLICABLE`；Top-4 合法；无 T-junction；视觉不再读作躯干+肩球。

### VR-1B：躯干与四肢完整轮廓

范围：pelvis/hip、thigh、knee、calf、ankle、forearm、wrist。

接入：mode-3 完整闭合 Canonical，保持 Render/UV/GPU 路径。

验收：front/back/side/45° 自然成人轮廓；基础 LBS 动作无断裂/塌陷/明显穿插。

### VR-2：头部与面部

范围： cranial vault、face、eyes、nose、mouth、ears、jaw、neck connection。

要求：

- Canonical Skin Shell 保持封闭；
- eyeball/iris 为辅助几何；
- 不增加 eye/eyelid 骨骼；
- 使用现有 Neck/Head 权重和局部 corrective 入口。

验收：面部正面/侧面/眼睛/鼻子/嘴部近景具有空间结构，不再是椭球和附着球。

### VR-3：手部与足部

范围：palm/digits、heel/arch/forefoot/toes。

22 骨骼限制：

- 手指顶点权重归 Hand；
- 脚趾顶点权重归 Foot/Toe；
- 只验证 wrist/hand 和 ankle/foot 整体运动；
- 不宣称独立指/趾关节动画。

验收：手掌正背面、手指侧面、足部侧面/底部有清晰结构，无手套/鸭蹼/模糊趾。

### VR-4：整体融合与最终验证

范围：完整 Canonical/Render/weights、八类动作、性能、内存、生成时间、真机多视角。

验收：

- 技术测试全绿；
- Host/Android mesh hash 一致；
- GPU parity 通过；
- 视觉由负责人独立确认；
- AI 不宣布 Gate PASS。

## 10. 验收标准

### 10.1 技术

- components=1；
- boundary=0（显式 crop 除外）；
- non-manifold=0；
- degenerate=0；
- invalid normals=0；
- bind-pose severe self-intersection=0；
- weights：BoneId 0–21、Top-4、非负、和 `1±1e-3`；
- seam C0 `<=1e-4 m`；肩根 crop 环按 OQ-03 从最终 face/triangle winding 独立测量，先验证 winding，输出 max/mean/p95、纳入 quad 数和角度样本数，硬条件用 max `<=5°`；无 crop ring 时 `NOT APPLICABLE`，不得复用硬编码 0 的 `BoundaryReport`；
- VR-1A mode-2 Weight Heatmap、cache 失效、per-mode skeleton 恢复和模式切换回归通过；
- renderToCanonical 合法；
- UV 无 NaN/越界/零面积；
- GPU/CPU parity 保持现有阈值；
- Android build/run 无 GL/shader error。

### 10.2 视觉

- 成人头身比和肩髋/腰臀关系自然；
- 胸廓、腰、骨盆、臀部有连续体积；
- 肩胸、腋窝、髋/腹股沟无断层；
- 肘、膝、腕、踝无球状堆叠；
- 面部有真实空间深度；
- 手指/脚趾独立可读；
- 不依赖材质、服装、头发、描边或阴影掩饰；
- 至少八视角和局部近景由负责人确认。

### 10.3 性能

当前基线：

- FPS 45.5–46.9；
- p95 23.22–24.32 ms；
- 115,300 triangles；
- TOTAL PSS 244,785 KB；
- Canonical total generation 约 83.8 s。

VR-1 前由负责人批准 stop guardrail。建议临时工程线：同设备关键指标不差于基线 20%；这不是 Gate 阈值。

## 11. 每轮交付要求

每轮必须同时提供：

1. 代码实现 + Git diff/文件清单；
2. 本轮新生成的 Android 真机视觉证据；
3. Host/Android 原始测试日志、失败项和未验证项。

禁止：

- 只交文档；
- 用 Host 测试替代真机；
- 用拓扑通过替代视觉；
- 用旧图充当新证据；
- 删除测试或降低要求；
- 自动 commit/push/merge/rebase；
- AI 宣布 Gate PASS。

## 12. 停止条件

- 补充授权未签发或超白名单；
- 需要修改冻结 ABI/骨骼/UBO/packing；
- 需要替换 RenderThread；
- 新方案无法保持统一 Canonical；
- 新拓扑无法映射现有 22 骨骼；
- seam 无法满足 C0 `<=1e-4 m` 或肩根 OQ-03 max `<=5°`；
- 真机与 Host 明显不一致；
- 性能超过批准 guardrail；
- 局部方案仍无法满足基本解剖结构；
- 需要导入外部模型/数据才能继续。

## 13. 主要风险与缓解

| 风险 | 缓解 |
|---|---|
| 局部 SDF 再次产生 blobs | 严格 AABB/RegionId/k 上限，只做局部修正 |
| Patch 边界 T-junction | 共享 BoundaryEvaluator、统一弧长采样、禁止 T-junction |
| 稳定 ID 丢失 | PAE 自有 templateId/localVertexId，参数只改位置 |
| 面部/手足复杂度过高 | 分阶段，先固定语义模板，再增加细分 |
| 22 骨骼限制 | 明确 digit 整体权重，独立关节运动不在本阶段宣称 |
| 权重跨区 | region mask + joint-aware falloff + 隔离测试 |
| 性能回归 | 分阶段 profile，先优化采样/细分/自交宽相位 |
| 授权冲突 | 补充授权正式签发前不写生产代码 |
| 许可证污染 | 不导入外部资产，R2 矩阵逐项核查 |

## 14. R1–R6 交付索引

- R1：`docs/research/parametric-human-modeling/01-reference-project-analysis.md`
- R2：`docs/research/parametric-human-modeling/02-tech-and-license-matrix.md`
- R3：`docs/architecture/parametric-human-model/01-human-shape-parameter-schema.md`
- R4：`docs/architecture/parametric-human-model/02-procedural-geometry-architecture.md`
- R5：`docs/phases/P1-visual-recovery/00-architecture-revision.md`
- R6：`docs/phases/P1-visual-recovery/01-minimal-torso-shoulder-prototype-plan.md`

## 15. 当前完成状态

- VR-0 仓库/治理/代码/外部研究：完成。
- R1–R6：完成。
- 生产代码：未修改。
- VR-1：未开始。
- 补充授权：负责人已选择新签，但 AI 未代签，正式文件待签发。
- Gate：无 PASS 声明。
