# Anatomy-driven Geometry — Implementation SPEC（草案 / DRAFT）

> 状态：**DRAFT — 未经项目负责人批准，不授权编码**。本文件是第一阶段的审查与规格交付物。
> 阶段范围：**仅 PoC-G0 与 PoC-A**。PoC-B/C/D、P1-B、肩部/手臂/头发/服装/面部均不在范围内。
> 依据：`ProceduralAvatarEngine-Anatomy-driven-Geometry-Development-Prompt-v1.0.md`（开发总提示词）与
> `docs/proposals/anatomy-driven-geometry-redesign/` 下 README + 00–09 共 11 份设计文档（全部逐份读完，未改写）。
> 文档日期：2026-11-23。所有状态按 FACT / REPORTED / PROPOSAL / OPEN / NOT VERIFIED 标注。

---

## 0. 本阶段做了什么 / 没做什么

| 项目 | 状态 |
|---|---|
| 本地 Git / 远程分支 / HEAD 重新核验 | DONE（见 §1） |
| ZIP 解压到 `docs/proposals/anatomy-driven-geometry-redesign/` | DONE（11 个文件，zip -t 无错误；原根目录下用户自带解压目录未动） |
| 设计包 README + 00–09 全读 | DONE |
| 仓库 Foundation / ADR / project-status / P0 / M11 / M12 / P1-A 报告 | DONE（见 §3） |
| 实际 C++ / JNI / Kotlin / tests 源码审查 | DONE（见 §4） |
| M11 / M12 / P1-A 真实 PNG 像素复核 | PARTIAL（逐张看代表图，见 §5；全部图逐张像素复核留 G0 完成） |
| D-01…D-10 OPEN 决策梳理 | DONE（见 §6） |
| 新 ADR / 正式状态文件 / 冻结合同修改 | 未做，等待负责人批准 |
| 任何 C++ / JNI / Kotlin 源码修改 | **未做** |
| Git commit / push / merge / rebase / reset / clean | **未做** |
| 新增文件 | 仅本目录下 SPEC 草案与 `docs/proposals/` 解压产物（均为 untracked） |

---

## 1. Git 核验报告（FACT）

采集时间：2026-11-23 14:33–14:41 GMT+8。

| 项 | 实测值 |
|---|---|
| `git rev-parse --show-toplevel` | `C:/Users/Giant/Desktop/GITCODE/Procedural Avatar Engine` |
| `git branch --show-current` | `feature/procedural-character-foundation` |
| `git rev-parse HEAD` | `486a00d8e9c5534d53480370be70015bc9c2f158` |
| HEAD 标题 | `docs: record P1-A visual rejection and defect archive` |
| upstream | `origin/feature/procedural-character-foundation` |
| 与 upstream ahead/behind | `0 0`（完全同步） |
| `git status --short --branch` | 无 tracked 修改、无 staged 修改；初始 3 项 untracked（提示词 md、ZIP、根目录解压目录），本次新增 `docs/proposals/` |
| `git diff --stat` / `git diff --cached --stat` | 均为空 |
| remote | `origin https://github.com/Vin535LFJ/ProceduralAvatarEngine.git`（fetch/push） |
| submodules | 无 |
| `git ls-remote --heads origin` | feature=`486a00d…`、m11=`67ccb394…`、m12=`c539afbe…`、main=`c539afbe…`，与本地 tracking ref 一致 |
| merge-base(main, feature) | `c539afbec299ed4a066429ea09f9d3d36e67f74d` |
| `git rev-list --left-right --count main...feature` | `0 6`（main 无独有提交，feature 领先 6 个提交，与 00-audit 的 REPORTED 一致） |
| tags | 无 |

**结论**：设计包记录的三个 SHA 与今天实测一致，但仍只作为历史快照，不作为开发基线授权。
工作区存在用户提供的 untracked 附件与设计包解压目录；未触碰、未 stash、未覆盖。未发现未知 tracked 修改。

**Git 风险与隔离建议（需负责人决定）**：按提示词 §2.2，建议后续编码使用独立 worktree 或新工作分支，
不覆盖当前工作区，不自动合并 main / feature，不 cherry-pick 历史提交。未经明确授权不 commit / push / merge。

---

## 2. 设计包状态判定

- 设计包总体状态：**PROPOSAL**，不是已批准的 Foundation Specification 或 ADR，更不是已验证技术路线。
- 历史实现定性（依 00-current-state-audit + 本次源码复核，均为 FACT）：
  - **M11**：完整角色参数化 Loft/Sweep + 独立 Part 的 buffer 合并（merge ≠ fusion）；可复用 spline/PTF/WeightGen，
    但不是统一连续人体表面路线。历史 14k tris / 50–53 FPS 属 REPORTED。
  - **M12**：统一 SDF + Surface Nets 能给出 components=1 / boundary=0 的水密表面，但场由椭球/胶囊/圆台组成，
    不代表自然解剖形态；Surface Nets 拓扑不是动画拓扑；24,360 tris / 51.7 FPS 属 REPORTED，不能外推。
  - **P1-A**：**REJECTED（项目方 Gate）**，仅作失败证据与对照。5 站位 Loft + bridge + Sweep，不继续修补，
    不进入 P1-B。其 BoundaryReport 的 C0/G1=0、smooth/valid=true 是硬编码（本次源码逐行确认）。
- 本 SPEC 不把上述任一条历史路线认定为新路线的最终技术选择；M12 的提取器与 P1-A 的失败证据只作为候选对照。

---

## 3. 仓库文档审查与冲突表（FACT）

### 3.1 已读文档
- `docs/project-status.md`：P1-A REJECTED，停留在 P1-A；P0 提交 `835dc71`，基线 `399967f`。
- `docs/foundation/02-skeleton-contract.md`：22 Bone ID 0–21 冻结；右手系、+Y Up、+Z Forward、+X Character Left、米制；
  Shoulder 语义 P0 修正为 Chest→Shoulder 短骨；bindLocal 仅平移。
- `docs/foundation/03-geometry-mesh-contract.md`（feature 冻结主线）：Rig-aware Landmarks → Topology Templates →
  Surface Patches → Variable-profile Sweeps → Patch Assembler；SDF 仅研究/局部辅助（ADR-010）；
  C0 ≤1e-4 m、G1 ≤5°；64B GPU packing，UV 未上传。
- `docs/foundation/04-skinning-contract.md`：≤4 骨、非负、和=1±1e-3、ID∈[0,21]；语义 Region Mask + Bone Segment Distance；
  LBS `Σw*(Palette*position)`；UBO `mat4 uBones[22]` 1408 字节 binding 0；CPU↔GPU 对照当前 NOT VERIFIED。
- `docs/foundation/05-visual-acceptance-protocol.md`：固定设备/相机/光照/灰模；AI 不得自标 VISUALLY ACCEPTED；
  阶段结论仅 PASS / PASS WITH FOLLOW-UPS / BLOCKED / NOT VERIFIED。
- `docs/foundation/06-thread-and-resource-lifecycle.md`：Worker 纯 C++，RenderThread 唯一 EGL owner，
  GenerationVersion 单调、过期丢弃、CPU MeshData 可恢复；P1-A 未重构 RenderThread。
- ADR-008（参数化主体+局部 SDF 补丁）、ADR-009（M12 统一 SDF 上半身 Accepted）、
  ADR-010（P1 Surface Patch 主线 Accepted，需用户 P0 Gate）、`docs/ADR_INDEX.md`。
- P1-A `04-test-and-acceptance.md`、`06-visual-defect-report.md`：工程 9/9 PASS、真机 870 verts/1664 tris、gen 2.419ms，
  但视觉 Gate REJECTED；7 项缺陷含 P0 级筒状躯干、肩胸暗线、圆管手臂、折痕。

### 3.2 ADR / Contract 冲突表

| ID | 旧合同 / 决策 | 适用性判定 | 新路线处理 |
|---|---|---|---|
| C-01 | Foundation 03 主线：Surface Patch / Sweep / Patch Assembler | 仅适用于 P1-A 旧 PoC；P1-A 已 REJECTED | 不直接沿用；新路线 Proposal 表述，需新 ADR 裁决 |
| C-02 | ADR-009 统一 SDF 上半身（main Accepted） | 在 feature 分支被 ADR-010 取代；M12 历史证据保留 | SDF/Surface Nets 作为 R1 对照候选，不是默认终局 |
| C-03 | ADR-010 P1 Surface Patch 主线（feature Accepted，待用户 Gate） | 其 PoC 实现（P1-A）视觉失败，但 ADR 文本仍存在 | 不改写 ADR-010；新 ADR 单独编号，说明取代/兼容范围 |
| C-04 | Foundation 02：BoneId 0–21、父子链、坐标/米制 | 仍有效，ABI 冻结 | **必须保持**，任何变更先单独 ADR |
| C-05 | Foundation 04 / ADR-005：UBO `mat4 uBones[22]` 1408B、binding 0、64B vertex packing | 仍有效 | **必须保持**；RegionId/UV/seam 先作 CPU metadata |
| C-06 | Foundation 05：视觉协议、AI 不得代批 Gate | 仍有效 | 严格执行；Gate A 由负责人审核 |
| C-07 | Foundation 06：Worker/RenderThread 分离、GenerationVersion、CPU cache | 合同有效但**实现未满足**（见 §4.5） | G0 复核记录；本轮不重写 RenderThread |
| C-08 | JNI/Kotlin 对外接口 | 仍有效 | 保持现有函数签名；新增仅内部预览通道 |

**结论**：存在 C-01/C-02/C-03 的真实历史冲突。按提示词 §2.3，建立本表即可；不直接编辑冻结文档。
需负责人批准新 ADR（建议 `ADR-011 Anatomy-driven Geometry route`，独立编号）后才更新正式状态入口。

---

## 4. 实际源码审查（FACT，逐行核对）

### 4.1 已确认的历史缺陷（与设计包审计一致）
- `BodyPatchGenerator.cpp:331-350`：4 条 BoundaryReport 直接写 `maxPositionDeviation=0`、`maxNormalDeviationDeg=0`、
  `smoothSeam=true`、`valid=true`，无对应采样点计算 → 历史 C0/G1 证据为 **NOT MEASURED**，不可采信。
- `BodyPatchGenerator.cpp:97-101` 签名含 `const Skeleton& skeleton`，函数体未读取 → 不是 Rig-aware。
- `SdfFusion.cpp:53` `sdSmoothUnion` 为 `+ k*h*(1-h)`；`AnatomyField.cpp:109` 为 `- k*h*(1-h)` →
  两套实现符号相反，必须先做数学推导 + 双实现单测统一约定，不能猜改符号。
- `AnatomyField.cpp:386-455`：`makeUpperBodyField` 只用肋/腹/盆/背 carve/肩/臂/颈/头；
  `buildPrimitives` 里计算的 `trapA/pecA/bicepsA/tricepsA/brachialis/rectus` 等未并入最终场 →
  `AnatomyFeatures.cpp` 定义的 feature set 是脚手架，不是已实现功能。
- PTF 边界条件：`ParametricSurfaces.cpp:26-56` 的 `computeParallelTransportFrames()` 无重复采样点清理，
  自身无显式反平行分支（反平行保护只在 `Math.h:86-100` 的 `Quat::fromToRotation` 内部，条件 `d < -0.999999f`），
  无正交性/手性/roll delta 验证；`BodyPatchGenerator.cpp:33-57` 的 `computeFrames()` 同样依赖该 quat 保护，
  但无重复点清理与连续性验证 → 测试 `parallel_transport_frames_avoid_flipping` 只断言单位长度/正交/连续 n·n>0.5，
  未覆盖近 180° 反平行与重复点。

### 4.2 拓扑/校验器能力边界（FACT）
- `MeshTopology.cpp`：仅精确 float 位置重复、有限/单位法线、边引用计数、边界边、non-manifold edge、
  零面积三角、连通分量；**无** non-manifold vertex、winding、outward normal、self-intersection、
  aspect/sliver、UV/attribute、孤立点、区域边界检查。
- `MeshValidator.cpp`：非空、索引范围、有限值、单位法线、权重和/负权/ID<22、退化比例、AABB、SubMesh range；
  不证明水密/法线朝外/自交/动画质量。

### 4.3 Shape/权重/蒙皮（FACT）
- `CharacterDefinition.h` 是 P1-A 扁平参数（pelvis/waist/chest/arm 宽深 + 段数），不是分层 Shape Model。
- `HumanAnatomyGenerator.cpp:69-118`：field 最小者为主区域，次骨取最近骨段，最多 2 权重；
  非语义 mask 的完整实现。
- `Skinning.cpp:7-19`：LBS，`SkinPalette = GlobalPose * inverseBind`；法线用 mat3 线性变换。无 DQS/corrective。
- `Skeleton.h`：BoneId 0–21 冻结，`BodyJoints` 位置为 P0 基线。

### 4.4 Renderer / 线程 / JNI（FACT）
- `RenderThread.cpp:568,592,613`：`ensureResources()` 内同步生成 M11 humanoid、M12 anatomy、P1-A patch →
  违反 Foundation 06「不得在 ensureResources/drawFrame 同步生成全身几何」。
- `RenderThread.cpp:636-643`：`releaseGpuResources()` 释放 avatar/shadow/p1a/boneUBO，
  **未释放 `anatomyMesh_` / `anatomyLines_`** → EGL Context 恢复路径潜在残留。
- `RenderThread.h` 有 `avatarRevision_/sceneRevision_`，但无 GenerationVersion / 过期任务丢弃 / 不可变 MeshData handoff。
- `RenderThread.cpp:20-26` GLSL：`layout(std140) uniform BoneBlock { mat4 uBones[22]; }`，attributes pos/normal/joints/weights/color，无 UV。
- GPU packing：`SkinnedGpuMesh.cpp:17-45` 交错 64 字节 = pos(3f)@0 + normal(3f)@12 + joints(4×u16)@24 +
  weights(4f)@32 + color(4f)@48；`BonePaletteUbo.cpp:9-13/30-35` UBO `mat4[22]` = 1408 字节、binding 0、
  `glBufferSubData` 上传；index 为 `GL_UNSIGNED_INT`。GLES3 + `GL_TRIANGLES`，无 compute shader。
- EGL：`EglContextManager.cpp:23-67/69-89/91-118` 支持 initialize / resize / destroy 并显式 `eglMakeCurrent(noContext)`，
  无 `EGL_CONTEXT_LOST` 专门分支（NOT VERIFIED）。
- `previewMode_ 0/1/2`、`cameraPreset_ 0..7`、`debugView_ 0..3`（0 shading/1 wire/2 normals/3 gray clay）已存在。
- `EngineRuntime.cpp` 为薄 facade；`jni_bridge.cpp` 导出 `nativeCreate/nativeSurfaceCreated/nativeSurfaceChanged/
  nativeSurfaceDestroyed/nativeSetAppearance/nativeSetBackground/nativeSetPoseMode/nativePushPose/nativeResetMotion/
  nativeSetQuality/nativeSetAnatomyPreview/nativeSetPreviewMode/nativeSetCameraPreset/nativeSetDebugView/
  nativeGetStats/nativeShutdown`；`PoseFrameCodec.kt` 编码 118 floats = 13 关节 + root + 时间戳。
  新增能力只能在内部预览通道扩展，保持现有签名。

### 4.5 测试现状（FACT）
- `tests/CMakeLists.txt` 有 9 个测试套件：`pae_core/geometry/fusion/face_hair/anatomy/humanoid/skinning/motion/
  character_patch_tests`，另有 `pae_proportion_probe` 可执行。
- `character_patch_tests.cpp:51-54`：`for boundary: EXPECT_TRUE(valid); EXPECT_NEAR(maxPositionDeviation,0,1e-6);
  EXPECT_NEAR(maxNormalDeviationDeg,0,1e-6)` —— 测试直接断言硬编码的 0，**使 P1-A BoundaryReport 假指标变绿**，
  不是真实 C0/G1 测量；新路线必须替换为真实计算的 seam 测量测试。
- `anatomy_tests.cpp:93-95` 真实断言 components=1 / boundaryEdges=0 / nonManifoldEdges=0；
  `:169` 断言胸宽变化 >3cm；`anatomy_mesh_supports_skinning` 检查 ID<22、权重和 1、≤2 影响（比 Foundation 的 ≤4 更窄）。
- `geometry_tests.cpp` 覆盖 Catmull-Rom / 非圆截面 / tangent / normal；`fusion_tests.cpp` 覆盖 SDF 原语 + smooth union +
  `poc_shoulder_fusion_comparison` + resolution 效应；`humanoid_tests.cpp` 覆盖骨架 BoneId/parent/head-tail、
  22 骨 palette、bind identity、生成合法性。
- `WeightGenerator.cpp:31-42` 主骨 = 最近骨段；非 Path 邻骨距离 >0.17m 排除、总贡献封顶 30%，
  取前 4 权重；`kinematicChainFromJoints` 左右链 0–21 完整。
- `MotionRuntime.cpp:42-54` 以骨架驱动的 LBS+骨骼旋转（每骨 `T(head)*R*T(-head)`），非 mesh 顶点变形。

### 4.6 设备可达性（FACT）
- `adb devices -l`：`MAX0019111000104 product:nunai_max device:nunai_max`，仅一台 Android TV Box 在线；
  未发现 Android 手机。D-07 要求的「TV Box + 一台手机」当前**只满足一半**，手机需负责人提供或明确豁免。
- P1-A 历史真机日志 `evidence/logcat-p1a.txt`：`PAE_STATS frames=120 fps=51.1 frame_ms=19.57 draw_calls=2
  triangles=1664 buffer_bytes=688960 p50_ms=20.71 p95_ms=24.05 p99_ms=31.30`；采集分辨率 1920×1080。
  这是 **REPORTED** 历史数据，不是本次实测。

---

## 5. 图片证据复核（PARTIAL，本次真实看图）

| 图 | 实际观察 |
|---|---|
| `P1A/clay_preset0.png` | 灰模躯干呈外扩筒状，上下端截断；单侧上臂为独立圆管，肩部接口有明显暗线；无自然胸腰盆节奏。与 DEF-002/004 一致。 |
| `P1A/clay_preset6.png` | 肩/背视角顶部开口边缘尖锐，肩胸过渡可见硬线与局部折叠；与 DEF-003/006 一致。 |
| `M12/dv0_preset0.png`（gray clay） | 单一连续表面，头/颈/肩/臂连成一体，但肩峰与胸廓像光滑 blob，肋廓与腹盆缺乏体块区分，臂部串珠状体积；佐证「水密 ≠ 自然人体」。 |
| `M11/frame-m11c-front-v2.png` | 完整卡通角色带服装/头发/描边，非灰模；多个 Part 的拼装感可见；仅作历史对照，不作几何 Gate 证据。 |

以上为本次真实像素观察，不是 REPORTED 转述。**未逐张看完 56 张图**；G0 将补齐全量图 + manifest
（branch/commit/modelVersion/preset/debugMode/pose/device/APK SHA），以当前仓库路径为唯一来源。
另发现 `P1A/04-test-and-acceptance.md` 引用 M12 `dv3_presetN.png` 作同机位对照，但 M12 evidence 实际只有
`dv0/dv1/dv2`（无 dv3）→ 该对照引用需在 G0 标为 **NOT VERIFIED** 并重新采集，不得沿用。

---

## 6. D-01…D-10 OPEN 决策梳理

原则：影响 ABI / 坐标约定 / 持久数据格式 / 分支合并 / 项目范围者必须负责人裁决；
非破坏性参数可用提示词 §3.1 的默认方向，但在本 SPEC 中标为 **assumption**，不冒充已批准。

| ID | 决策 | 默认方向（PROPOSAL / assumption） | 是否需负责人裁决 | 本 SPEC 处理 |
|---|---|---|---|---|
| D-01 | 测试体型 | 单一中性、轻度运动型卡通 baseline（H≈1.70m，依 01-visual 表） | **是**（影响范围/视觉目标） | 暂用默认值做 PoC-A；Gate A 前需确认 archetype |
| D-02 | 参考表面 | Shared Skin Envelope，SDF 与 B-spline guide 混合，不预锁后端 | 否（技术路线，可比较） | PoC-A 同输入比较 R1/R2/R3 |
| D-03 | 动画拓扑 | 固定 anatomy quad template 投影 | 否（PoC-A 只比参考表面，拓扑在 B/C 才深化） | 记为后续方向，不提前实现 |
| D-04 | Hybrid 边界 | 所有贡献进同一 envelope，禁止独立 mesh 叠加 | 否（强约束，写入契约） | 作为 PoC-A 硬约束 |
| D-05 | PoC-A 范围 | 完整胸腹腰盆上部 + 双侧肩连接 landmarks，不含完整上臂 | **是**（范围边界，与用户「不开发肩部/手臂」需对齐） | 按提示词与用户指令执行；肩仅保留连接锚点/封口 |
| D-06 | 变形 | 首轮 LBS + BoneId 0–21；DQS/corrective 非前提 | 否（ABI 不变） | 保持 LBS，不实现 DQS |
| D-07 | 设备 | 至少 TV Box + 一台 Android 手机 | **是**（手机缺失需提供/豁免） | 仅 TV Box 可用；手机项标 NOT VERIFIED |
| D-08 | 冻结合同/ADR | 旧合同不覆盖，新增 ADR 治理 | **是**（分支/合同治理） | 建冲突表；新 ADR 待批准后才写 |
| D-09 | AnatomyFeatures | 重定义为 Shape Model 的 muscle feature set，不复用半成品 API | 否（实现细节） | PoC-A 不依赖旧 AnatomyFeatures |
| D-10 | 视觉目标 | 固定 gray-clay 协议 + 负责人提供/确认的参考目标 | **是**（视觉 Gate + 参考物） | 需要负责人给参考图；AI 不代批 |

**必须由负责人裁决的阻塞项**：D-01、D-05（范围确认）、D-07、D-08、D-10；另加 Git 隔离方式（worktree/新分支）。
其余为 assumption，负责人可否决。

---

## 7. PoC-G0 规格（待批准）

### 7.1 目标与范围
只建可复核 harness，**不生成新人体形态**。

### 7.2 必做项
1. 建立证据清单与 manifest：读取 M11/M12/P1-A 全部原始 PNG，核实来源 commit、机位、相机/FOV/目标点、
   角色缩放、材质/光照、Bind/Pose、设备与 APK SHA；缺失标 NOT VERIFIED；修正 dv3 引用问题。
2. 固定 8 相机组：front/back/left/right/45L/45R/shoulder closeup/torso closeup；统一角色高度、FOV、目标点、光照。
3. 固定 neutral gray clay + continuous Lambert；禁用描边/toon band/rim/服装/头发/装饰材质。
4. 参数 manifest：parameter ID、语义、单位、范围、默认值、affectedRegions、mustRemainStableRegions、预期变化方向。
5. 报告格式（真实计算，禁止硬编码 0/PASS）：
   - C0：对应 seam 采样点位置差，目标 ≤1e-4 m；记录样本数、winding、索引映射、Hausdorff proxy。
   - G1：两侧 surface derivative / tangent plane / normal angle，smooth ≤5°，hard seam 显式标记。
   - topology：components、boundary（PoC crop 显式登记）、non-manifold edge/vertex、winding、outward normal probe、
     degenerate/sliver、aspect、self-intersection、孤立点、UV/attribute、region boundary。
   - weights：≤4、非负、和 1±1e-3、ID∈[0,21]、region allowed-bone mask 命中率。
   - pose：bind identity、动作体积/折痕/翻面/自交。
   - performance：Worker 生成、拓扑、上传耗时；CPU/GPU bytes；FPS/P50/P95/P99、设备/APK SHA。
6. 重新运行现有 host tests（当前基线 REPORTED 9/9）、CMake build、Gradle assemble；记录真实输出与环境。
7. 复核 RenderThread 同步生成、EGL 生命周期、GPU 释放遗漏、CPU MeshData cache 现状（§4.4 已列 FACT）。

### 7.3 G0 Gate（全部为可复核事实才通过）
- 历史图片可独立复核，manifest 字段齐全；每个报告字段来自真实计算，无硬编码 PASS/零偏差。
- 固定相机/灯光协议被代码和测试引用，且所有候选路线共用。
- baseline 可重复采集（同 preset 两次截图差分在记录阈值内）。
- G0 未过 → 不开始 PoC-A。

---

## 8. PoC-A 规格（待批准，G0 通过后执行）

### 8.1 范围（硬边界）
- 下界：骨盆/髂嵴上部，可见腰盆过渡；上界：胸廓上缘、锁骨根部、颈基底。
- 覆盖前胸、侧胸、胸腔侧面、后背、肋弓、上腹、下腹、侧腰、腰背、髂嵴、骨盆前后体积。
- 双侧肩连接 landmarks 必须存在；**不生成完整上臂**，不生成肩部 PoC、头发、服装、面部细节。
- 一个 Shape Model / Shared Skin Envelope 负责整个表面；多 field 贡献只允许一个最终外表面。
- 肩根以可解释的解剖边界/显式登记的 crop boundary 封口，不用独立肩球或圆管遮掩。

### 8.2 数据契约（最小聚合 value types，不建 Manager/Factory 堆叠）
- `CharacterDefinition`（schemaVersion、比例、anatomyShape、style、symmetry、seed）— 新结构，独立于 P1-A 扁平参数。
- `AnatomicalLandmarks`：`SkeletalLandmarks` / `SurfaceLandmarks` / `AnatomicalCurves` 三类分开，不从 BodyJoints 混成。
- `AnatomicalRegions`：有向 Region Graph（Thorax / AbdominalWall / PelvicMass / ShoulderRoot），
  含 parent、neighbors、allowedBones、边界过渡、局部密度优先级、稳定 `RegionId`。
- `ShapeParameters`：比例/体积/轮廓/风格四组高层参数；radius/field coefficient/control point 为派生值。
- `JointDeformationRegions`：PoC-A 只定义肩根接口语义，不做完整肩/肘变形求解。
- 单位米；右手系 +Y Up / +Z Forward / +X Character Left；同一 definition+seed+generatorVersion 确定性复现；
  高层参数变化联动重算 landmarks/controls；左右对称由共享定义+镜像。
- `RegionId` 跨 field/topology/weights/debug/clothing seam 稳定；骨骼仍引用 0–21，不移动 skeleton joint。

### 8.3 表示路线对照（同 Shape Model、同 landmarks、同参数）
- R1：SDF + Surface Nets 参考表面直接输出（可复用 M12 提取器作对照，不改其 ABI）。
- R2：同一 field 作为 Reference Surface，投影固定 quad topology template（PoC-A 只验证可行性/误差，不深化动画拓扑）。
- R3：B-spline/envelope control surface + 固定 topology template。
- R4 Dual Contouring：可选，仅作特征保持对照；无理由不扩大范围。
- 禁止各路线各自调参数；差异必须可归因于表示方式。

### 8.4 参数组与截图
- 至少 3 组：Baseline / Chest Width-Depth / Waist-Pelvis Ratio。
- 每组 7 视图：front / back / left / right / 45L / 45R / torso closeup；所有路线同 preset 同相机。
- 同时报告数值 contour 与 landmark 差异（非仅看图）。

### 8.5 Gate A（工程 + 视觉分离）
工程：
- 单一连续目标表面；components=1，boundary=0（crop 边显式登记），non-manifold edge/vertex=0。
- C0/G1 真实计算、可复算；winding/normal/degenerate/self-intersection 报告齐全。
- 参数只影响目标区域，不破坏 mustRemainStableRegions；对称误差复核。
- Worker 生成时间、内存、CPU/GPU bytes、真实设备性能；不得抄 M12 数字。

视觉（负责人独立否决权）：
- 胸/腹/腰/骨盆在 front/side/45/back 均自然；无外扩筒、沙漏筒、硬折、blob、拼接感。
- 任一 P0 视觉缺陷 → Gate A = REJECTED，停止，不进入 PoC-B；AI 不代批。

---

## 9. 测试与开发流程

- 流程：`grill-with-docs → to-spec → to-tickets → implement + tdd → 独立 code-review → retro`。
  `grilling` skill 在本环境可用并已用于本次 OPEN 决策梳理；to-spec/to-tickets/implement/code-review/retro
  未安装为同名 workflow skill，按提示词 §3 手工同序执行，并在报告中说明。
- 本阶段只完成 grill + spec draft；拆票、编码、TDD 均在 SPEC 获批且 Git 隔离方式确定后进行。
- TDD 顺序：先写失败测试（参数校验、landmark 约束、C0/G1、topology、权重、确定性、极端参数），
  再写最小实现；不通过改弱断言让测试变绿。
- 报告状态只用 PASS / FAIL / PARTIAL / NOT VERIFIED；未执行项写 NOT RUN，不预写 PASS。

---

## 10. 文件边界与回滚

- 本阶段新增：`docs/phases/anatomy-driven-geometry-poc/00-implementation-spec.md`（本文件）与 `evidence/`（空，待采集），
  以及 `docs/proposals/` 解压产物（用户 ZIP 原样）。
- 获批实施阶段拟新增（**尚未创建**）：`native/procedural/anatomyshape/`（Shape Model / envelope / 参考表面对照 / 拓扑投影）、
  对应 host 测试文件、G0/A 报告与 manifest。具体文件名在 to-tickets 阶段冻结。
- 不修改：`native/procedural/character/*`（P1-A 保留）、`native/procedural/humanoid/*`（M11/M12 保留）、
  `native/renderer/RenderThread.*` 与 JNI/Kotlin 对外签名、任何冻结合同/ADR/project-status。
- 回滚：所有新增代码通过独立 previewMode 通道挂接；失败时 `previewMode_` 回 0/1/2，历史路径不受影响。
- 无 commit / push / merge；负责人可随时要求丢弃本阶段新增文档。

---

## 11. 待负责人批准 / 裁决清单（阻塞后续阶段）

1. **批准本 SPEC 草案**或给出修改意见（未批准前不拆票、不编码）。
2. **Git 隔离方式**：独立 worktree / 新工作分支 / 其他（当前工作区有用户 untracked 附件）。
3. **D-08**：是否批准新建 `ADR-011` 并在何时更新正式状态入口；不改写 ADR-008/009/010。
4. **D-01 / D-10**：确认测试体型 archetype 与 gray-clay 视觉参考目标（负责人提供或指定参考图）；视觉 Gate 只能由负责人签字。
5. **D-05**：确认 PoC-A 边界 =「完整胸腹腰盆 + 双侧肩连接锚点，不含肩部/手臂生成」与用户「不提前开发肩部/手臂」一致。
6. **D-07**：提供一台 Android 手机用于真机验证，或明确书面豁免（则手机相关项标 NOT VERIFIED）。
7. 确认是否授权在 G0 阶段重新运行 host tests / CMake / Gradle 并采集真机 baseline（只读构建与安装，不改源码）。

---

## 12. 状态声明

- 本 SPEC 为 **DRAFT / NOT RUN**，不含任何 PASS 声明。
- PoC-G0、PoC-A 的代码、测试、截图、拓扑/参数/性能报告：**未执行 / NOT VERIFIED**。
- 视觉 Gate A：**未提交、未审核、未批准**。
- 下一步：等待负责人对上述清单裁决；批准后才进入 to-tickets 与 PoC-G0 实施。
