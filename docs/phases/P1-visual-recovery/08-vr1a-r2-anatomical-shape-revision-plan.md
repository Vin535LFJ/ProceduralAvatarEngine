# VR-1A-R2 解剖形态修订实施计划

日期：2026-11-25
基线：`299ae3bf3fcb6d76be563f815aa52380e018d50b`
当前授权：`AUTH-P1-VR-01` 仅覆盖 VR-1A-R1
本计划状态：**`DRAFT — R2 生产代码修改尚未授权`**

## 1. 本轮目标与边界

R2 的唯一首要目标：让 Android mode-2 的 Clay 网格具有可测量的自然成人胸廓、腰腹、肩带、腋窝和上臂根轮廓，而不是继续优化“封闭网格”这一技术结果。

范围：

- Thorax。
- Abdomen。
- Upper Pelvis。
- Shoulder Girdle。
- Axilla。
- Left/Right Upper-arm Root。

不范围：

- Head/Face/Hand/Foot/Full Legs。
- mode-3 full Canonical 重建。
- 新骨骼、JNI、UBO、64B packing。
- 外部模型、morph、blendshape、训练数据。
- VR-1B 生产实现。

## 2. R1 真实代码缺陷分析

### 2.1 固定密度与未消费的请求

- `kTorsoSegments=32`、`kTorsoRings=11`、`kArmSamples=6` 是编译期常量：`native/procedural/character/TorsoShoulderPatchGenerator.cpp:20-22`。
- `GeometryBuildRequest.detailLevel` 只进入 cache key，生成器没有读取 `request.*`：`TorsoShoulderPatchGenerator.cpp:657-660`。
- Host 探针中 detail 0/1/2 都是 524 vertices / 1,028 triangles，最大顶点位移为 0。
- 结果：R1 的 quality refresh 只改变 cache key，不改变拓扑或形态。

### 2.2 三段宽深插值不足以表达躯干

`torsoSection()` 只使用 hip/waist/chest/shoulder 四个宽深值，并在 `t=0.28/0.64` 分三段 smoothstep：`TorsoShoulderPatchGenerator.cpp:39-72`。

实际缺陷：

- `joints` 参数未使用。
- `hipD = hipWidth * 0.30`，骨盆深度不是独立解剖量。
- `shoulderD = chestD * 0.98`，肩带深度没有独立控制。
- `waistPinch` 只压缩腰宽 `(1 - 0.20 * waistPinch)`，不压缩腰深。
- 没有 iliac crest、umbilicus、costal margin、lower/upper rib、sternum、clavicle level 等站点。
- 默认语义腰宽/腰深大于胸宽/胸深，形成桶状轮廓。

### 2.3 径向缩放不是解剖截面

`torsoProfile()` 返回 12 个标量 radial controls：`TorsoShoulderPatchGenerator.cpp:74-88`。`sampleClosedProfile()` 只是椭圆乘标量：`native/procedural/character/VariableProfileSweep.cpp:87-93`。

结果：

- 前胸、侧肋、后背、腹部、腰部、骨盆侧缘共用同一套椭圆径向缩放。
- 不能独立控制 front/back/left/right contour。
- `chestBoost` 是固定 `0.025`，不是 `CharacterDefinition` 输入。
- `clavicleRelief` 只修改 front 侧 radial controls 2/4，并乘 `0.20`，不构造锁骨曲线或肩峰。

### 2.4 clavicle 参数作用极小且 cache 不失效

Host 探针：

- `clavicleRelief: 0.010 → 0.030` 最大顶点位移仅 `0.000669 m`。
- 修改该参数前后 cache key 完全相同。

原因：

- `GeometryBuildRequest.cpp:23-52` 未哈希 `torso.clavicleRelief`。
- 生成器的 clavicle 作用位于 `TorsoShoulderPatchGenerator.cpp:83-86`，不是肩带形态基。

### 2.5 肩宽参数没有移动肩部轮廓

- `shoulders.leftWidthScale/rightWidthScale` 只进入 `buildArm()` 的 `sideScale`：`TorsoShoulderPatchGenerator.cpp:153-158`。
- 它们不改变 acromion、肩带上缘、锁骨或臂根位置。
- 探针中 `leftWidthScale: 1.0 → 1.15` 的 torso 最大位移为 0，仅左上臂移动 `6.889 mm`。
- 真正的肩带宽度来自 `torso.shoulderGirdleMass` 对 `shoulderW` 的全局插值，不是左右独立肩宽。

### 2.6 三角肌是统一半径放大

`buildArm()`：

- 中心路径是 `shoulderCenter → elbow` 的直线：`TorsoShoulderPatchGenerator.cpp:163-170`。
- `baseRadius = max(0.042, chestWidth * 0.155) * sideScale`：`:184`。
- deltoid 只有一个标量：`0.16 * deltoidVolume * compactFalloff(...)`：`:224-227`。
- 截面永远是 `radius × 0.88 radius` 的近似椭圆：`:232`。

结果：

- 没有 deltoid anterior/lateral/posterior 分向体积。
- 没有 biceps/triceps/brachialis/medial/lateral contour。
- 探针中 `deltoidVolume: 1.0 → 1.6` 最大位移只有 `3.097 mm`。

### 2.7 `buildBridge()` 和 `capBoundaryLoops()` 是死代码

- `buildBridge()` 定义于 `TorsoShoulderPatchGenerator.cpp:258-326`，但没有调用者。
- `kBridgeSteps=4` 只在该死函数中使用。
- `capBoundaryLoops()` 定义于 `:514-653`，也没有调用者。
- 当前实际肩部连接是 `buildArm()` 内 root→bridgeRing→firstRing 的两条 quad strip：`:194-220`。

R2 不得继续维护两套“看起来存在但实际不运行”的肩部拓扑。

### 2.8 顶部 cap 是平面 fan

`generate()` 在 torso top ring 与单一中心点之间创建 32 个三角形：`TorsoShoulderPatchGenerator.cpp:773-785`。

- 中心点固定在 `(0, yTop+0.008, 0)`。
- 没有 neck base opening、clavicle curve、trapezius slope 或 acromion peak。
- 这是 R1“平坦肩盖”的直接结构原因。

### 2.9 Landmark 被解析但未控制 mesh

- `ResolvedCharacterState` 保存 `landmarks`：`native/procedural/character/ResolvedCharacterState.cpp:38-39`。
- `TorsoShoulderPatchGenerator.cpp` 没有任何 `state.landmarks` 读取。
- 探针中显式把 `left-acromion` 移到 `(0.30,1.40,0)`，网格最大位移为 0。
- `CostalMargin`、`ChestCenter`、`NeckBase`、`LeftAxilla` 等也没有控制曲面；生成器仍使用 `joints.chest/spine/leftShoulder` 和硬编码偏移。
- `AnatomicalLandmarks.cpp:70-74` 把任意显式 override 直接标为 `Measured`，但没有 provenance 字段；这会把 Assumed 输入伪升级为 Measured。R2 必须增加 confidence/source 并修正测试。

### 2.10 `LocalShapeBasis` 只有一个球形标量

`LocalShapeBasis::evaluate()` 是三维距离的 compact falloff：`native/procedural/character/LocalShapeBasis.cpp:14-19`。

- `region` 和 `normal` 字段没有参与 evaluate。
- 没有 station/angular support、side mask、allowed combinations 或 boundary continuity 元数据。
- chest basis 是固定 `amplitude=0.008` 的球形 relief：`TorsoShoulderPatchGenerator.cpp:696-698`。
- axilla 是单中心球形 concavity，只在 `t>0.70` 作用于躯干环：`:699-727`，没有胸壁/上臂内侧/腋窝后壁四边界。

### 2.11 已有 Patch/Template 模块未接入

- `PatchAssembler::assemblePatches()` 只有定义，没有生产调用者。
- `SurfacePatch` 没有用于当前肩部网格。
- `TopologyTemplate::stableGridId()` 没有调用者；生成器只使用 `makeStableVertexId()`。

R2 应复用并接入这些模块，而不是再创建一套同职责模块。

### 2.12 参数表与专用字段形成双权威

`CharacterDefinition` 同时包含：

- `ShapeParameters parameters`。
- `TorsoShapeParameters torso`。
- `ShoulderShapeParameters shoulders`。
- `UpperArmShapeParameters upperArms`。
- `AsymmetryParameters asymmetry`。

`defaultShapeParameterTable()` 把 `thorax-ribcage-volume`、`waist-pinch`、`axilla-depth` 等标为 `Wired`：`native/procedural/shape/v2/CharacterDefinition.cpp:146-199`，但生成器只读专用字段。

探针中修改 table 参数的最大位移全部为 0。R2 必须明确唯一数值权威，不能继续保留两套可独立编辑的同义参数。

### 2.13 Cache hash 不完整

`hashCharacterDefinition()` 未包含：

- `torso.clavicleRelief`。
- `asymmetry.rightUpperArmRadiusDelta`。
- 显式 landmarks。
- `ShapeParameters.values`。
- R2 将新增的 control stations/shoulder form 字段。
- `asymmetry.leftUpperArmRadiusDelta` 以及多个影响 joints 的比例也没有完整纳入哈希。

`makeGeometryBuildRequest()` 还把 `request.generatorVersion` 直接设为 `definition.schemaVersion`：`GeometryBuildRequest.cpp:68`。schema 升级到 3 后，R1/R2 baseline 不能仅靠 schema 区分；R2 必须解耦 schema version 与 generator version，并让 baseline 显式 pin R1。

探针确认 clavicle、acromion override、right upper-arm delta 的 cache key 均不变。

### 2.14 诊断口径混淆

`tests/unit/vr_torso_shoulder_tests.cpp:376-390` 的 `seamAngles` 遍历所有共享两面边，不是只统计注册肩部 seam。当前 `seam_max/p95` 实际是全局二面角分布，不能解释为肩胸连续性。

`result.report.boundaries` 的 deviation 和 normal deviation 是硬编码 0：`TorsoShoulderPatchGenerator.cpp:869-874`，不是计算值。

### 2.15 关键参数的实际计算公式

| 参数/环节 | R1 实际公式 | 真正影响 |
|---|---|---|
| hip width | `hipW = hipWidth * 0.5 * 0.92 * upperPelvisWidthScale` | 下段宽度 |
| waist width | `waistW = waistWidth * 0.5 * (1 - 0.20 * waistPinch)` | 只压宽 |
| chest width | `chestW = chestWidth * 0.5 * ribcageWidthScale` | 胸段宽度 |
| shoulder width | `shoulderW = shoulderBreadth * 0.5 * shoulderGirdleMass` | 上段全局宽度 |
| center Z | `0.012*smooth01((t-0.35)/0.45) + 0.006*sin(pi*t)` | 固定前后偏移 |
| section point | `(cos(phi)*W*scale, sin(phi)*D*scale)` | 椭圆径向缩放 |
| chestBoost | `0.025*smooth01((t-0.38)/0.35)` | 固定标量，不是输入 |
| clavicle | `clavicleRelief*0.20*smooth01((t-0.78)/0.22)` | 仅 front radial controls 2/4 |
| shoulderCenter | `rootCenter + normalize(elbow-rootCenter)*0.050 + outward*0.030` | 硬编码偏移 |
| arm path | `shoulderCenter + (elbow-shoulderCenter)*t` | 直线 |
| baseRadius | `max(0.042, chestWidth*0.155)*sideScale` | 统一臂根半径 |
| deltoid | `0.16*deltoidVolume*compactFalloff(abs(t-0.10),0.34)` | 统一标量半径 |
| arm radius | `baseRadius*(1+deltoid)*(1-0.25*t)*mix(1,elbowRadiusScale,smooth01(t))` | 统一收缩 |
| arm section | `sampleClosedProfile(profile,u,radius,radius*0.88)` | 固定椭圆比例 |
| chest basis | `0.008*compactFalloff(distance(center),0.16)` | 固定球形 relief |
| axilla basis | `-axillaDepth*compactFalloff(distance(center),0.085)` | 单球形 concavity |
| top cap | center→32 top-ring vertices | 平面 fan |

## 3. Host 探针实测

临时探针位于 `build-host/vr1a_r2_audit_probe.cpp`，不修改生产代码。原始输出：`docs/phases/P1-visual-recovery/evidence/vr1a-r2-code-audit-probe.log`。

| 参数/输入 | 测试变化 | 最大全局位移 | 目标区域位移 | 结论 |
|---|---:|---:|---:|---|
| ribcageWidthScale | 1.00→1.15 | 20.312 mm | torso 20.312 mm | 生效，但改变整段胸廓插值 |
| chestDepthScale | 1.00→1.15 | 20.075 mm | torso 20.075 mm | 生效，但缺少独立前后形态 |
| waistPinch | 0.45→1.00 | 17.332 mm | torso 17.332 mm | 只压宽，不压深 |
| shoulderGirdleMass | 1.00→1.15 | 37.627 mm | 双侧肩带 | 生效，但全局肩宽插值 |
| clavicleRelief | 0.010→0.030 | 0.669 mm | 0.450 mm | 作用过小，且 cache 不失效 |
| axillaDepth | 0.028→0.060 | 18.023 mm | 双侧腋区 | 有凹陷，但不是四边界腋窝 |
| leftWidthScale | 1.00→1.15 | 6.889 mm | 左上臂 | 不移动肩带/acromion |
| deltoidVolume | 1.00→1.60 | 3.097 mm | 双侧上臂 | 统一半径放大，幅度不足 |
| leftRadiusScale | 1.00→1.40 | 18.372 mm | 左上臂 | 生效，但仍是统一截面 |
| rightUpperArmRadiusDelta | 0→0.40 | 18.360 mm | 右上臂 | 生效，但 cache 不失效 |
| left-acromion override | 改到 (0.30,1.40,0) | 0 | 0 | landmark 完全未接入 |
| ShapeParameters table | 多项改为极值 | 0 | 0 | `Wired` 标记不真实 |
| detailLevel | 0/1/2 | 0 | 0 | 网格完全相同 |

R1 默认 mesh 的确定性 FNV-64（positions/joints/weights/indices/stableIds）为 `638a9cf368577940`。R2 必须用该 golden hash 或逐顶点 parity 固定 R1 baseline，防止算法重构悄悄改变对照基线。

可复现探针源码与命令：

- `docs/phases/P1-visual-recovery/evidence/vr1a-r2/audit_probe.cpp`
- `docs/phases/P1-visual-recovery/evidence/vr1a-r2/audit-command.txt`
- `docs/phases/P1-visual-recovery/evidence/vr1a-r2-code-audit-probe.log`

默认关节与 landmark：

- left shoulder joint：`(0.241821, 1.302865, 0)`。
- left acromion：`(0.263821, 1.314865, 0)`。
- left axilla：`(0.267821, 1.247865, 0.004)`。
- chest center Y：`1.350381`，高于 shoulder joint/acromion。
- neck base Y：`1.468507`。
- R1 顶环宽度约 `0.472039 m`，腰环宽度约 `0.283431 m`、深度约 `0.258950 m`。

## 4. 当前参数与 R2 目标差距

| 项目 | R1 当前 | R2 候选目标 | 差距/风险 |
|---|---|---|---|
| 胸廓宽度 | 语义 0.2906 m；上环过宽 | 0.344 m | 需要独立 chest station，而非肩带插值 |
| 腰宽 | 语义 0.3295 m；实际约 0.283 m | 0.264 m | 必须低于胸宽 |
| 腰深 | 语义 0.2636 m；实际约 0.259 m | 0.200 m | 当前甚至深于胸廓 |
| 肩峰间距 | landmark 0.4159 m，未接入 | 0.416 m | 保持尺度，但必须控制 mesh |
| 可见肩盖上缘 | 约 0.472 m | 约 0.432 m 最大 | 去除平板和过宽帽檐 |
| 锁骨 relief | 参数极差仅 0.669 mm | 3–4 mm | 需要曲线型形态基 |
| 三角肌外侧 | 参数极差仅 3.097 mm | 12–16 mm | 需要方向性体积 |
| 腋窝 | 单球形负向凹陷 | 四边界 10–14 mm 凹面 | 需要独立 patch |
| 上臂截面 | 统一椭圆 | 前后/内外独立 | 需要 biceps/triceps 等分向 profile |
| 网格密度 | 固定 524/1,028 | 实验 2,000–4,000 vertices | 必须特征驱动，不能均匀堆面 |
| seam 诊断 | 全局二面角 | 全局 + BoundaryId seam + 曲率 | 必须拆开口径 |
| detail/LOD | 0/1/2 相同 | 真正 LOD0/1/2 + lineage | 需要消费 request 并扩展映射 |

## 5. R2 具体算法

### 5.1 单一语义权威

推荐方案：

- `CharacterDefinition` 继续作为唯一可编辑人体语义输入。
- 新增 typed `TorsoControlStation` 和 `ShoulderFormParameters`，放入现有 `TorsoShapeParameters/ShoulderShapeParameters` 职责范围。
- `ShapeParameters` 不再保存同义数值；只保留 metadata/legacy adapter，且明确 `DeclaredOnly`，或由 typed 字段生成只读视图。
- `schemaVersion` 升级到 3；`generatorVersion` 用于选择 R1/R2 算法，不创建第二个人体定义。`generatorVersion` 必须与 schema version 解耦，R1 baseline 显式 pin `generatorVersion=2`，R2 使用独立版本号。
- 所有新增字段必须进入完整 definition hash。
- 显式 landmark override 必须携带 confidence/provenance；没有测量来源的 override 默认 `Assumed`，不得自动标为 `Measured`。

### 5.2 Landmark 真正进入曲面

`ResolvedCharacterState` 新增/解析：

- IliacCrestLeft/Right。
- Umbilicus。
- LowerRibCageLeft/Right。
- UpperRibCageLeft/Right。
- JugularNotch/SternalAngle。
- ClavicleMedialLeft/Right、ClavicleLateralLeft/Right。
- ScapulaMedial/LateralLeft/Right。
- TrapeziusLeft/Right。
- NeckBaseInterface。

规则：

- 显式 landmark override 优先级最高。
- landmark 位置用于 control station 高度、中心、肩带曲线和臂根路径。
- `CostalMargin` 控制下肋/胸廓下缘，不只是保存一个点。
- `Acromion` 控制肩峰顶点、三角肌顶点和上臂 root tangent。
- `Axilla` 控制腋窝 patch 的顶/后边界和 concavity anchor。
- `NeckBase` 只控制接口环；是否允许生成短 neck/trapezius transition 由授权勾选决定。

### 5.3 多站点躯干截面

纵向使用单调 cubic Hermite 或 monotone cubic interpolation：

```text
W(s) = Hermite(Wi, Wi+1, mi, mi+1, u)
D(s) = Hermite(Di, Di+1, ni, ni+1, u)
C(s) = Hermite(Ci, Ci+1, ...)
```

约束：

- 站点高度单调。
- 斜率受限，避免胸廓/腰部 overshoot。
- 每个站点有独立 width/depth/center/front/back/side controls。
- waist pinch 同时作用于腰宽和腰深，并有独立 station support。

角向不再使用单一椭圆半径，而使用 12 个解剖方向的周期控制：

```text
front-center, anterolateral-left, lateral-left, posterolateral-left,
back-center, posterolateral-right, lateral-right, anterolateral-right
```

使用 periodic cubic B-spline/Catmull-Rom 生成闭合 `Q(s,φ)`。肩带和腋窝可使用 transfinite Coons patch，将四条独立边界连续拼接。

### 5.4 方向性 LocalShapeBasis

扩展现有 `LocalShapeBasis`，不新建同职责模块：

```text
ΔPj = amplitude_j * direction_j
      * compactFalloff((s - sj)/σs)
      * compactFalloff(angleDistance(φ, φj)/σφ)
      * sideMask_j
```

每个 basis 必须声明：

- Region。
- Landmark anchor。
- Station/angular support。
- Direction。
- Amplitude。
- Boundary continuity。
- Allowed combinations。

禁止球形/椭球叠加。Pectoral、ribcage、abdominal、oblique、back、scapular、trapezius、clavicle、deltoid 和 axilla 均使用方向性支撑。

### 5.5 结构化肩带 yoke

复用 `SurfacePatch` 和 `PatchAssembler`：

- Front boundary：clavicle curve。
- Left/Right boundary：acromion→deltoid root。
- Back boundary：trapezius/scapular transition。
- Top boundary：neck-base interface。
- 所有边界使用稳定 BoundaryId 和共享 vertex IDs。

基础方案推荐显式 `neck-base-interface` crop loop，不生成头颈动画。若负责人要求 boundary=0，则需单独授权最小 neck/trapezius transition ring；不得直接扩大 R2 范围。R1 baseline 路径仍保持 boundary=0；R2 若采用 crop loop，只允许一个注册外部接口边界，内部肩胸/腋窝/臂根边界必须为 0，并使用独立断言，不修改或降低 R1 baseline 测试。

### 5.6 腋窝 patch

腋窝由四条边界构成：

1. 胸壁内侧弧。
2. 上臂内侧弧。
3. 腋窝前/顶壁。
4. 腋窝后壁/肩胛过渡。

使用 Coons/structured grid 生成，concavity 作为方向性位移而不是负球。Bind Pose 与肩部动作都要检查凹面是否保留。

### 5.7 上臂中心路径与截面

中心路径使用 cubic Bezier 或曲率自适应曲线：

```text
C(t) = B0(1-t)^3 + 3B1(1-t)^2t + 3B2(1-t)t^2 + B3t^3
B0 = acromion/root center
B1 = deltoid apex
B2 = upper-arm mid contour
B3 = elbow interface
```

- 使用 RMF/parallel transport 保持 frame 稳定。
- 采样密度由曲率、profile 变化和局部形态基支撑决定。
- 截面宽度和深度分别由 `W(t)`、`D(t)` 控制。
- deltoid/biceps/triceps/brachialis 使用方向性 relief。
- elbow transition 独立于 upper-arm root，不把整条手臂当圆管。

### 5.8 特征驱动细分

初始控制网格之外，按以下误差估计局部细分：

```text
e = max(|κ| h^2, normal deviation, profile second derivative, basis curvature)
```

优先细分：

- acromion。
- clavicle。
- deltoid。
- axilla。
- shoulder-chest junction。
- chest front/back contour。

实验范围 2,000–4,000 vertices，但不是验收目标。避免整体均匀增加无用网格。

### 5.9 Stable ID 与 LOD

- 保留现有 Stable ID 体系。
- control lattice ID 与细分级无关。
- 新增 child ID 由 parent vertex/edge + refinement path 派生。
- 扩展 `LodCorrespondence` 支持 parent face/edge/vertex、barycentric 和 refinement path。
- R2 必须真正消费 `GeometryBuildRequest.detailLevel`，LOD0/1/2 不能生成相同网格。

## 6. 诊断口径修正

分别报告：

1. 全局相邻面二面角分布。
2. 注册 BoundaryId 的 seam 两侧对应采样法线差。
3. 局部曲率变化。
4. Sliver triangles。
5. Triangle shape quality：min angle、aspect ratio、condition number。
6. Quad Jacobian 符号和 fold 检测。
7. Self-intersection。
8. 参数局部性：目标区域/非目标区域最大位移。
9. Landmark→mesh 因果测试。
10. 左右对称/不对称。
11. 动作后腋窝、肩胸和上臂根体积。

OQ-03 继续暂缓硬阈值，不自行引入 5° Gate。

## 7. 预计修改文件清单

### 7.1 数据与状态

- `native/procedural/shape/v2/CharacterDefinition.h/.cpp`
- `native/procedural/character/AnatomicalLandmarks.h/.cpp`
- `native/procedural/character/ResolvedCharacterState.h/.cpp`
- `native/procedural/character/GeometryBuildRequest.cpp`

### 7.2 几何与拓扑

- `native/procedural/character/LocalShapeBasis.h/.cpp`
- `native/procedural/character/VariableProfileSweep.h/.cpp`
- `native/procedural/character/TorsoShoulderPatchGenerator.h/.cpp`
- `native/procedural/character/SurfacePatch.h`
- `native/procedural/character/PatchAssembler.h/.cpp`
- `native/procedural/character/TopologyTemplate.h/.cpp`
- `native/procedural/character/LodCorrespondence.h/.cpp`

### 7.3 权重与渲染

- `native/procedural/character/RegionWeightGenerator.*`：仅在新增肩带/腋窝 region 或权重支撑时修改。
- `native/renderer/RenderThread.h/.cpp`：仅允许 generator version 选择、mode-2 cache key/诊断日志；不得改变 camera/lighting/mode-3/JNI。
- `native/CMakeLists.txt`
- `tests/CMakeLists.txt`
- `tests/unit/vr_torso_shoulder_tests.cpp`

### 7.4 文档与证据

- `docs/phases/P1-visual-recovery/07-vr1a-r2-shape-target.md`
- `08-vr1a-r2-anatomical-shape-revision-plan.md`
- R2 授权草稿与白名单草稿。
- `docs/phases/P1-visual-recovery/evidence/vr1a-r2/**`

### 7.5 明确不修改

- `native/procedural/humanoid/**`。
- `native/skeleton/**`。
- `native/renderer/gl/**`。
- `app/src/main/java/**`。
- `app/src/main/cpp/jni_bridge.cpp`。
- `AGENT.md`、ADR、DEC、CAS、Gate。
- `.workbuddy-ai/`、`evidence/character-v6/**`、`evidence/character-v7/**`。

## 8. Host 测试计划

扩展现有 `pae_vr_torso_shoulder_tests`，不删除或降低 R1 测试：

- 每个 control station 的高度、宽度、深度、中心可测。
- 每个 landmark 对 mesh 有非零且局部的影响。
- `Acromion` 改变肩峰和 root path。
- `CostalMargin` 改变胸廓下缘。
- `ClavicleRelief` 产生目标量级且 cache key 改变。
- waist width/depth 独立且均低于 chest。
- pectoral/ribcage/abdominal/oblique/back/scapular/trapezius 分向测试。
- deltoid anterior/lateral/posterior 不同时变化。
- 腋窝四边界和 Bind/Action concavity。
- detail 0/1/2 不同且 LOD lineage 有效。
- BoundaryId seam 对应采样，不复用全局角分布。
- triangle quality、Jacobian、self-intersection、winding、Top-4。
- definition hash 覆盖所有 shape/landmark 字段。
- R1/R2 同定义、同拓扑策略下的确定性。
- R1 baseline 对 `638a9cf368577940` golden hash 或逐顶点 parity。
- 显式 landmark override 无 provenance 时保持 `Assumed`，不伪升级 `Measured`。R1 baseline 的旧契约断言固定在 pin 的 R1 definition/version；R2 新契约使用修正后的 confidence 语义，不降低 R1 baseline parity 要求。
- R2 crop policy 与 R1 boundary=0 baseline 使用各自明确断言。

## 9. mode-2 新旧形态对照方案

### 9.1 固定条件

- 同一设备 `nunai_max`。
- 同一分辨率 1920×1008。
- 同一 mode-2 camera presets。
- 同一灯光和 Clay shader。
- 同一 `CharacterDefinition` 序列化值。
- 同一姿态与 debug view。
- R1/R2 只改变 generator version。

### 9.2 推荐实现

在现有 `GeometryBuildRequest.generatorVersion` 上增加 R1/R2 dispatch，保留 R1 算法作为 baseline 路径。`generatorVersion` 必须与 `schemaVersion` 解耦；R1 baseline 显式 pin `generatorVersion=2`，R2 使用独立版本号。使用 build-time CMake 定义生成两个证据 APK：

- R1 baseline APK：强制 generatorVersion=R1。
- R2 APK：默认 generatorVersion=R2。

不新增 JNI/UI extra，不改变 camera/lighting。若负责人要求同一 APK 运行时切换，则必须额外授权选择入口；本计划默认不假设该授权。

### 9.3 必采视角

Front、Back、Left、Right、45° Left、45° Right、Shoulder/Chest Close-up、Axilla Close-up。

Debug views：Clay、Wireframe、Normal、Weight Heatmap；Material ID 可选。

Poses：Bind、左臂抬起、双臂外展。当前 `SyntheticPoseProvider` 的 action 2 是左臂弯曲，不存在右臂单侧抬起；右臂单侧动作保持 `NOT VERIFIED`，除非负责人额外授权最小 deterministic action 驱动。右侧形变可由双臂外展和左右权重测试覆盖，但不能冒充右臂单侧验证。

### 9.4 对照记录

- R1 APK hash、R2 APK hash。
- 同定义 hash、camera/lighting 代码 diff 为 0。
- R1 mesh golden hash `638a9cf368577940` 或逐顶点 parity。
- 每张截图 hash。
- 目标测量表 R1 vs R2。
- 明确 `IMPROVED / UNCHANGED / REGRESSED / NOT VERIFIED`，由负责人作视觉判断。

## 10. Android 验收与性能计划

### 10.1 技术验收

- components/boundary/non-manifold/winding/normal/self-intersection。
- Top-4、左右隔离、cache/skeleton 一致。
- quality refresh、Surface recreation、mode-3 无回归。
- 显式 neck-base crop boundary 若被采用，必须单独注册，不能把内部肩胸 seam 伪报为 crop。

### 10.2 性能分项

- State/landmark resolution。
- Control station/section evaluation。
- Patch generation。
- Patch assembly。
- Weight generation。
- GPU upload。
- Steady FPS/P50/P95/P99。
- CPU mesh bytes、GPU buffer bytes、TOTAL PSS、peak RSS。

R1 参考：

- 524 vertices / 1,028 triangles。
- Generator total 7.484 ms。
- GPU mesh upload 2.512 ms。
- 约 49.1 FPS，P95 22.02 ms。
- TOTAL PSS 209,167 KB，peak RSS 287,828 KB。

R2 密度增加到 2,000–4,000 vertices 后，不能继续用 R1 的绝对生成时间作同工作量 20% 判定。必须同时记录：

- 绝对时间。
- `ms / 1k vertices`。
- `ms / 1k triangles`。
- 相同 detail 的 R1/R2 workload。
- R2 LOD0/1/2 各自预算。

正式 R2 性能预算需负责人确认；未确认前不宣称 guardrail PASS。

## 11. 主要技术风险与停止条件

- 肩带 yoke/腋窝 Coons patch 自交或折叠。
- 细分后 LOD  cracks 或 Stable ID 失配。
- 法线 seam 改善但曲率仍突变。
- landmark 位置与 skeleton 动作不一致。
- 参数表双权威未消除。
- cache hash 仍遗漏新字段。
- 肩部动作导致腋窝塌陷或权重串扰。
- R2 密度增长导致 FPS/内存超预算。
- 为保持封闭而偷偷加入未授权头颈几何。
- 视觉仍为桶状躯干、平板肩盖或圆管。

出现以上任一情况即停止，不通过降低测试或隐藏问题继续。

## 12. R2 后 VR-1B 完整人体连续形态计划

R2 不实现 VR-1B，但完成后必须提交以下实施计划：

1. **VR-1B-0：全身控制骨架与接口契约**
   - Head/Neck、Torso、Full Arms、Hands、Pelvis、Legs、Feet 的 control stations 和共享 BoundaryId。
2. **VR-1B-1：Pelvis/Hip/Thigh/Knee**
   - 髋臼、腹股沟、骨盆侧缘、大腿前后轮廓。
3. **VR-1B-2：Full Arms/Hands**
   - forearm/wrist/palm/digits，保持 22 骨骼和 Top-4。
4. **VR-1B-3：Head/Neck**
   - 头颈接口、面部空间结构，不新增骨骼。
5. **VR-1B-4：Legs/Feet**
   - calf/ankle/heel/arch/toe，保持现有 BoneId。
6. **VR-1B-5：Canonical/Render/UV/GPU 整合**
   - 完整连续 skin shell、renderToCanonical、UV、GPU parity、动作和性能。

VR-1B 必须重新授权和独立验收，不能把 R1/R2 mode-2 局部 Patch 直接视为完整人体。

## 13. 实施里程碑

1. M0：负责人确认 `07-vr1a-r2-shape-target.md` 与授权草稿。
2. M1：完成参数/landmark/cache 契约修复和 Host 测量测试。
3. M2：实现多站点躯干截面和方向性形态基。
4. M3：实现结构化肩带、腋窝和上臂根 patch。
5. M4：特征驱动细分、LOD lineage 和诊断口径修正。
6. M5：Host 全量回归、Android R1/R2 同条件对照。
7. M6：性能/内存、动作、Surface/mode-3 回归和最终报告。
8. M7：提出 VR-1B 完整实施计划，等待独立授权。

## 14. 当前结论

R1 的技术结果有效，但代码审计表明其形态参数大多是全局插值、固定半径或未接入 landmark。R2 必须把重点从“增加参数”转为“建立可测量的解剖控制站点、方向性表面和真实肩带拓扑”，同时修复 detail、cache hash、参数双权威和诊断口径。

在生产授权签发前，本计划不授权任何代码修改。
