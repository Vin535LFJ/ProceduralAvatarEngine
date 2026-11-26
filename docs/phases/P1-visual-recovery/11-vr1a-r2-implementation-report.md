# VR-1A-R2 实现与真机复核报告

日期：2026-11-26
授权：`AUTH-P1-VR-01` 的 R2 补充决定，`APPROVED FOR P1-VR-1A-R2 ONLY`
设计基线：`66a7e5905dde407b46df1e76074cd8b9ea94ab79`
R1 基线：`299ae3bf3fcb6d76be563f815aa52380e018d50b`
当前状态：**`IMPLEMENTED / TECHNICAL CHECK PARTIAL / VISUAL BLOCKED`**

本报告不宣布视觉 Gate PASS，不授权 VR-1B，也不包含 commit/push。

## 1. 本轮实际实现

### 1.1 语义与状态链

- `CharacterDefinition` 升级为 schema 3，增加 typed torso control stations、8 角向控制、肩带/三角肌参数和 landmark provenance。
- `ShapeParameters` 中同义数值不再作为第二套可编辑权威；旧表保留为 metadata/legacy adapter。
- `ResolvedCharacterState` 输出不可变 `torsoStations`，显式 landmark override 可改变对应 station 的高度和前后位置。
- `GeometryBuildRequest.generatorVersion` 与 schema version 解耦；R1 baseline 固定为 version 2，R2 为 version 3。
- definition hash 覆盖 clavicle、左右臂 radius delta、显式 landmarks、control stations 等 R2 字段。
- 无来源的显式 landmark 默认保持 `Assumed`，不再伪标 `Measured`。

### 1.2 几何与拓扑

- 用 12 个解剖站点和 8 方向闭合截面替代 R1 的三段椭圆插值。
- 增加 pectoral、ribcage、abdominal、oblique、thoracic back、scapular、trapezius、clavicle 等方向性形态基。
- 肩带/上臂通过 `SurfacePatch + PatchAssembler` 组装，共享 BoundaryId/Stable ID/最终 index。
- PatchAssembler 增加 C0、Stable ID、T-junction、edge incidence、winding、重复面、退化面和事务式拒绝组装。
- 上臂改为 Bezier 中心路径、独立宽深 profile、前/后 relief 和多层肩部过渡。
- Acromion、Shoulder Joint、Upper-arm Root、Deltoid Surface 保持四类独立语义。
- detail 0/1/2 生成不同拓扑；增加 parent-face/barycentric LOD lineage。

### 1.3 Android

- mode-2 默认使用 R2。
- 新增 CMake `PAE_VR_R1_BASELINE` build-time 选项，用于生成受控 R1/R2 两个证据 APK。
- 未新增 JNI、UI extra 或运行时切换入口。
- mode-3 Canonical 路径未修改。

## 2. Host 技术结果

完整 CTest：**20/20 通过，0 失败，总耗时 133.19 s**。证据：`evidence/vr1a-r2/implementation/ctest-full-final.log`。

默认 R2 detail 1：

```text
vertices=1795 triangles=3520
components=1 boundary=32 non-manifold=0 degenerate=0 invalid_normals=0
weights=valid frame_diagnostics=valid
self_intersections=0
```

`boundary=32` 全部来自授权方案 A 的显式 `neck-base-interface` 外部 crop loop；肩胸、腋窝和上臂根内部边界为共享 indexed vertices，不登记为 crop。

LOD：

```text
detail 0: 955 vertices / 1,864 triangles
detail 1: 1795 vertices / 3,520 triangles
detail 2: 1859 vertices / 3,648 triangles
```

三个 detail 均通过 topology/weights/frame/self-intersection 检查；detail 2 的额外密度集中在肩部过渡，而不是均匀增加全身网格。

## 3. 修正后的诊断口径

默认 R2：

```text
global dihedral samples=5264 max=179.430° p95=46.749°
registered seam samples=32 max=98.144° p95=97.809°
curvature samples=10560 max=565.087 1/m p95=44.205 1/m
slivers=56
min triangle angle=1.420°
max aspect=36.424
```

解释：

- 全局二面角不再被误标为肩部 seam 指标。
- 注册 seam 仍显示接近直角的局部转折，说明肩胸/肩根过渡还不够光滑。
- 曲率、sliver 和最小角表明局部三角形质量仍差，近景折面明显。
- OQ-03 继续暂缓硬阈值，以上数据只作研究诊断，不新增 5° Gate。

## 4. R1/R2 真机同条件对照

设备：`nunai_max`，Android 8.1，1920×1008。
相机、灯光、姿态、CharacterDefinition 序列化输入和 debug view 保持一致。

APK：

- R1 baseline：`c83ae9c9b8bdd5f53033afbc3320173b3ad692fb4240a76d2605cb6402dbc0a6`
- R2 final：`17280c10843e7bfdb507e8b25ad21688de5753a0e925687a228be6eab577c256`

证据目录：

- `evidence/vr1a-r2/implementation/r1-baseline/`
- `evidence/vr1a-r2/implementation/r2-final/`

覆盖：

- Front、Back、Left、Right、45° Left、45° Right。
- Shoulder/Chest Close-up、Axilla Close-up。
- Clay、Wireframe、Normal、Weight Heatmap、Material ID。
- Bind、左臂抬起、双臂外展。
- 右臂单侧动作按授权保持 `NOT VERIFIED`。

视觉变化：

- R2 的胸廓、收腰和上臂轮廓比 R1 更明确。
- R2 肩胸不再有 R1 的封闭桶状肩盖，landmark 和参数已真实影响网格。
- Bind pose 无自交，culling 开启下无 R1 式肩部黑洞。
- 但肩峰/锁骨/三角肌仍有明显折面，seam 凹槽偏深，腋窝体积不足，双臂外展时肩带仍有不自然牵扯。

因此视觉状态保持 **`BLOCKED`**。

## 5. 性能与内存

R1 baseline 冷启动：

```text
generation total 7.651 ms
state 0.210 / geometry 1.580 / assembly 5.001 / weight 0.809 ms
mesh upload 2.464 ms / wire upload 6.550 ms
peak RSS 286576 KB
```

R2 冷启动：

```text
generation total 49.730 ms
state 0.202 / geometry 10.177 / assembly 36.418 / weight 2.792 ms
mesh upload 8.163 ms / wire upload 23.917 ms
peak RSS 293876 KB
```

稳态 R2：

```text
约 49 FPS
P50 20.14 ms / P95 21.68 ms / P99 22.24 ms
2 draw calls / 3520 triangles
```

归一化：

- R1：约 7.44 ms / 1k triangles。
- R2：约 14.13 ms / 1k triangles。

R2 的绝对生成/上传时间高于 R1，但三角形数量约为 3.43 倍，且包含新的共享边界组装、方向性形态和 LOD 数据，不能直接按同 workload 20% guardrail 宣称 PASS。当前记录为：

- 稳态 FPS/frame time：未发现相对 R1 的明显回归。
- 峰值 RSS：R2 略高，但未形成批准的同 workload 内存预算判定。
- 生成/组装/上传：绝对成本增加，需要后续性能优化和同 workload baseline 复测。

## 6. Cache、Surface 与 mode-3 回归

- mode-2 quality Low/High 实际生成 955/1859 vertices，cache key 和网格均变化。
- mode-2 Surface recreation 后恢复 cache/skeleton/joints/MotionRuntime。
- mode-3 冷启动完成，Canonical 为 46,674 vertices / 93,344 triangles，Render Mesh 280,032 vertices。
- GPU/CPU parity 三个 case 均 `result=ok`。
- mode-3 未因 R2 修改出现构建或渲染失败。

## 7. 未完成与未验证项

1. 腋窝四边界独立 patch 尚未完全实现；当前是共享 root、过渡环和方向性凹面，视觉仍不像自然腋窝。
2. 自适应细分的归一化误差公式已统一，但 epsilon 尚未完成标定，也未替代当前 feature-focused detail policy。
3. seam max/p95、曲率、sliver、最小角仍不理想。
4. 双臂外展时肩带/腋窝仍有牵扯感，动作视觉未通过。
5. 右臂单侧动作 `NOT VERIFIED`。
6. 同 workload 20% guardrail `NOT VERIFIED`。
7. 完整自然成人视觉 `BLOCKED`。
8. 未 commit、未 push；所有修改保留在工作区。

## 8. 下一步

R2 已证明多站点躯干、方向性形态基、共享边界组装和真实 landmark 因果链可行。下一步不应继续无限增加肩部参数，而应：

1. 由负责人独立查看 R1/R2 八视角和近景，确认改善方向。
2. 针对 seam、腋窝和三角肌做一轮小范围几何修正，不扩大身体范围。
3. 标定自适应细分 epsilon，并把高成本 PatchAssembler 进一步性能化。
4. 建立同 workload 性能 baseline 后再判定 guardrail。
5. 制定 VR-1B 完整人体连续形态计划，覆盖 Head/Neck、Torso、Full Arms、Hands、Pelvis、Legs、Feet；VR-1B 必须重新授权。
