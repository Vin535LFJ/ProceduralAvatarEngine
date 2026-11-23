# G1 路线可行性报告（AUTH-07 / R2、R3 最小实验）

> 日期时间：2026-11-23 19:00–19:50 GMT+8
> Worktree：`E:/AI-Tools/ProceduralAvatarEngine-PoC`（detached HEAD `99941a9`）
> 基线 Git SHA：`99941a94ec58fb37ab0f1a36b90590d3fdce5198`
> 阶段：**G1 Route Feasibility**
> 结论：**R2 = FEASIBLE（推荐进入 G2）；R3 = FEASIBLE（技术可行，不推荐进入 G2）；R1 = FEASIBLE 但不满足固定拓扑前提，仅作参考路线**
> Host Tests：**10/10 PASS，0 FAIL**（9 项既有 + 1 项新增 G1 套件，共 9 个用例）
> **本轮发现并修复 DEF-02（公平比较协议缺陷）——修复前后 E1 结论从"R2 略优"变为"R2 决定性领先"**

---

## 0. 一句话结论

固定拓扑模板路线**技术可行**：R2 在参数变化下顶点数恒定（834 → 834）、拓扑干净（components=1 / boundary=0 / non-manifold=0 / degenerate=0）、20 个参数中 16 个可测得几何增量。**在修正了公平比较协议缺陷（DEF-02）之后**，R2 的 E1 误差为 **0.0116 m（均值）/ 0.0387 m（最大）**，R3 为 **0.0353 m / 0.1332 m**——R2 领先 **3.0×（均值）/ 3.4×（最大）**，且 R2 的误差随模板分辨率提升而收敛、R3 的基本不收敛（见 §6.2）。**建议只带 R2 进入 G2。**

---

## 1. 判定口径

| 判定 | 条件 |
|---|---|
| `FEASIBLE` | 路线能生成几何；拓扑可解析；参数变化能以**可测方式**改变几何；失败时有可复现证据 |
| `KILLED` | 路线在上述任一条上失败，且失败证据可复现 |

本轮三条路线**均未触发 KILLED**。但"未 KILLED"不等于"满足 G2 准入"——G2 准入还要求：

1. 固定拓扑（顶点数不随形状变化）；
2. 参数可测（SPEC 参数证据等级 ≥ L4：可测 Mesh Delta）；
3. `DeclaredOnly` 参数可证明零影响；
4. **公平比较协议成立**（同一 Shape Model、同一 landmark 骨架、同一参考面、同一采样、同一坐标系、同一相机协议）。

第 4 条正是本轮 DEF-02 的落点。

---

## 2. 环境与复现命令

| 项 | 值 |
|---|---|
| 编译器 | `g++ (GCC) 11.5.0`（`C:/cygwin64/bin/c++.exe`） |
| CMake / CTest | `3.20.0-rc1`，Generator `Unix Makefiles` |
| 构建目录 | `E:/AI-Tools/ProceduralAvatarEngine-PoC/build-host` |
| 解剖分辨率 | 44 × 66 × 26（与 G0 Host/TV Box 基线一致） |
| 模板分辨率 | 粗 10×16（单测）、生产 26×32（PoC-A 规格） |
| 参考骨架 | `deriveJoints(BodyParameters{})`，scale 0.9605（见 §4.1） |

```bash
cmake --build "E:/AI-Tools/ProceduralAvatarEngine-PoC/build-host" \
      --target pae_poc_route_tests poc_route_probe -- -j4
ctest --test-dir "E:/AI-Tools/ProceduralAvatarEngine-PoC/build-host" --output-on-failure
"E:/AI-Tools/ProceduralAvatarEngine-PoC/build-host/pae_tests/poc_route_probe.exe" \
      "E:/AI-Tools/ProceduralAvatarEngine-PoC/evidence/g1/meshes"
```

### 2.1 边界遵守声明

| 边界 | 状态 | 证据 |
|---|---|---|
| 原始工作区只读 | ✅ | 全部写入位于 `E:/AI-Tools/ProceduralAvatarEngine-PoC` |
| 不新增 Git commit / 不 push | ✅ | `git status` 仅显示 `M` / `??`，无新 commit；远端未触碰 |
| 不改 ABI、不替换生产入口 | ✅ | `native/CMakeLists.txt` 使用**显式源文件列表、无 glob**；`add_subdirectory(tests)` 只在 `if(PAE_HOST)` 分支内，Android 分支根本看不到 `tests/` 与 `native/procedural/shape/` |
| **生产 APK 字节级未变** | ✅ | DEF-02 修复后再次增量重建，SHA256 仍为 `088476fcd1fd5b84edfe7109116f310a4753a8a627a837fe39691320f055a9c4`，与 DEF-01 修复版**完全一致** |
| 不改设备数据 / 系统配置 | ✅ | 本阶段全部在 Host 侧执行，无设备写入 |
| 不扩范围 | ✅ | 仅 T1（躯干主干）最小实验，T2 见 §7 |

G1 增量 Android 重建：`BUILD SUCCESSFUL in 8s`，`buildCMakeDebug[arm64-v8a]` 实际执行并通过，日志 `evidence/android-build/gradle-assembleDebug-g1.log`。

---

## 3. DEF-02：公平比较协议缺陷（本轮最重要的发现）

### 3.1 缺陷描述

初版实现中，三条路线**没有站在同一副骨架上**：

- **R1**（参考路线）调用 `generateAnatomyUpperBody(params, BodyParameters{}, resolution)`，该函数内部用 `deriveJoints(bodyParams, scale)` **自行推导** landmark 骨架；
- **R2 / R3** 则由调用方传入硬编码的默认 `BodyJoints{}`。

两者实测差异：

| 关节 | `deriveJoints(BodyParameters{})`（R1 实际使用） | 硬编码 `BodyJoints{}`（R2/R3 原先使用） | 偏差 |
|---|---|---|---|
| hips.y | 0.864 | 0.900 | 3.6 cm |
| spine.y | 1.039 | 1.100 | 6.1 cm |
| chest.y | **1.254** | **1.350** | **9.6 cm** |
| leftShoulder.x | 0.202 | 0.200 | 0.2 cm |
| leftElbow.x | **0.252** | **0.400** | **14.8 cm** |
| leftWrist.x | **0.280** | **0.500** | **22.0 cm** |

后果：R2/R3 是在"骨架 A"上生成的，参考面却来自"骨架 B"。E1 测到的绝大部分不是模板保真度，而是**两副骨架之间的手臂位置差**。

### 3.2 缺陷的量化证据

E1 最差点定位（修复前）：

```
R2 worst: err=0.1501 @ (x=+0.280, y=1.061, z=+0.008)   ← 恰好是骨架 A 的腕部 x=0.280
R3 worst: err=0.1436 @ (x=+0.279, y=1.100, z=+0.008)
```

参考网格在 `|x| ≈ 0.28` 处有顶点（骨架 A 的手臂几乎垂直下垂，腕部 x=0.280），而 R2/R3 的 SDF（骨架 B）手臂在 `x ≈ 0.356`。直接场采样确认：

```
y=1.06  SDF 沿 +X 的过零点：[exit 0.128]  [enter 0.356]
        → 躯干在 0.128 处结束，0.356 之后是手臂（独立 lobe）
```

### 3.3 第二个叠加缺陷：参考区混入了手臂

参考区原用 `|x| ≤ leftShoulder.x + 0.10` 的盒式过滤，把**手臂 lobe 的采样点也纳入了参考面**。而 PoC-A 的模板只覆盖躯干，两者不可比。

### 3.4 修复

1. **统一骨架**：三条路线的入口函数**不再接受 joints 参数**，全部内部调用 `referenceJoints()`。这样"用两副骨架比较"在 API 层面**不可能发生**。
2. **参考区改为躯干 lobe**：新增 `torsoRadiusAt()`——从身体轴向外行进，取**第一个**等值面过零点。因为手臂是从轴向外更远处的**独立 lobe**，取第一个过零点天然排除手臂。
3. **回归防护**：新增用例 `routes_share_one_skeleton`，断言 `r1.joints == r2.joints == r3.joints == referenceJoints()`（1e-6），并断言参考骨架 **不等于** 硬编码 `BodyJoints{}`（后者一旦被误用立即报错）。

### 3.5 修复前后对比

| | 参考样本 | R2 E1 mean | R2 E1 max | R3 E1 mean | R3 E1 max | R2/R3 均值比 |
|---|---|---|---|---|---|---|
| 修复前（骨架不一致 + 含手臂） | 3172 | 0.0433 | 0.1501 | 0.0533 | 0.1436 | 1.23× |
| **修复后（骨架一致 + 仅躯干）** | **2066** | **0.0116** | **0.0387** | **0.0353** | **0.1332** | **3.04×** |
| 变化 | −35% | **−73%** | **−74%** | −34% | −7% | — |

> **结论的方向性因此改变**：修复前 R2 只比 R3 好 23%（在 4 cm 量级的噪声上做 20% 的比较不可靠）；修复后 R2 领先 3.0×，且 R3 的最大误差几乎不受参考区修正影响（−7%），说明 R3 的误差是**模型固有**的（见 §6.2）。

---

## 4. 统一 Shape Model 参数契约（Task #19）

新增 `native/procedural/shape/ShapeModel.{h,cpp}`：23 个参数的单一读写入口，`readParameter` / `writeParameter` 按名字与区间校验，带 `found` 标志。

| 状态 | 数量 | 参数 |
|---|---|---|
| `Wired` | 20 | chestWidth, chestDepth, waistWidth, waistDepth, pelvisWidth, shoulderWidth, ribCageVolume, abdominalVolume, backDepth, waistCurvature, clavicleWidth, deltoidVolume, shoulderSlope, axillaDepth, upperArmVolume, elbowWidth, forearmLength, forearmVolume, wristWidth, smoothness |
| `DeclaredOnly` | 3 | torsoHeight, upperArmLength, stylization |
| `NotImplemented` | 0 | — |

`DeclaredOnly` 的判定依据是**代码事实**：`torsoHeight` / `upperArmLength` / `stylization` 在 `AnatomyField.h` 中被声明，但从未被 `buildPrimitives` / `makeUpperBodyField` 读取。

### 4.1 DeclaredOnly 零影响（可证明）

```
[declared-only] torsoHeight        R2 delta=0 R3 delta=0
[declared-only] upperArmLength     R2 delta=0 R3 delta=0
[declared-only] stylization        R2 delta=0 R3 delta=0
```

三条路线上均为**精确 0**（断言阈值 1e-9），符合 DEC-06「DeclaredOnly 参数必须可证明产生零 mesh delta」。

### 4.2 Wired 参数可测性

| 路线 | 有效（delta > 1e-5） | 被遮蔽（MASKED） |
|---|---|---|
| R2 | **16** / 20 | 4：`shoulderWidth`(0.000000)、`forearmLength`(0.000000)、`forearmVolume`(0.000002)、`wristWidth`(0.000000) |
| R3 | **7** / 20 | 13 |

**R3 的有效参数集是 R2 的真子集**：R3 有效的 7 个参数（chestWidth, chestDepth, waistWidth, waistDepth, pelvisWidth, ribCageVolume, waistCurvature）在 R2 上同样有效；R2 另外多覆盖 **9** 个；反向为 **0**。

原因明确：R3 的包络函数 `envelopeAt()` 只消费 4 个关键高度（hips / spine / chest / shoulder）的半宽半深，因此 `backDepth`、`deltoidVolume`、`shoulderSlope`、`axillaDepth`、`upperArmVolume`、`elbowWidth`、`smoothness`、`abdominalVolume`、`clavicleWidth` 根本不进入 R3 的几何。

> **诚实标注**：按 SPEC 参数证据等级，4 个 MASKED 参数（`shoulderWidth`、`forearmLength`、`forearmVolume`、`wristWidth`）当前**最多只能到 L3（几何入口）**，在 R2 上不可测、在 R3 上同样不可测。其中 `shoulderWidth` 属 T2 肩根区域，遮蔽原因与 §7 的 T2 未实现直接相关。

---

## 5. 路线度量

目标参考面：解剖 SDF 等值面（同一副 `referenceJoints()` 骨架），限制在 PoC-A **躯干 lobe**（第一过零点定义，排除手臂），共 **2066** 个采样点，三条路线共用。

### 5.1 生产模板分辨率 26 × 32

| Route | Status | Verts | Tris | Quads | Components | Boundary | NonManifold | Degenerate | MeanEdge(m) | E1 mean(m) | E1 max(m) |
|---|---|---|---|---|---|---|---|---|---|---|---|
| R1 SDF + Surface Nets | FEASIBLE | 5108 | 10212 | 5106 | 1 | 0 | 0 | 0 | 0.017 | 0.0000 | 0.0000 |
| R2 quad template projected on SDF | FEASIBLE | 834 | 1664 | 800 | 1 | 0 | 0 | 0 | 0.032 | **0.0116** | **0.0387** |
| R3 quad template on B-spline control lattice | FEASIBLE | 834 | 1664 | 800 | 1 | 0 | 0 | 0 | 0.027 | 0.0353 | 0.1332 |

生成耗时（Host, x86_64）：R1 191 ms / R2 51 ms / R3 < 1 ms。

> R1 的 E1 = 0.0000 是**定义性结果**而非精度优势：R1 与目标参考面是同一曲面（参考面取自 R1 的等值面），逐点最近距离恒为 0。R1 不参与 E1 比较，E1 只用于 R2 / R3 之间的横向比较。

### 5.2 粗分辨率 10 × 16 对照

| Route | E1 mean (粗) | E1 mean (生产) | 收敛 | E1 max (粗) | E1 max (生产) | 收敛 |
|---|---|---|---|---|---|---|
| R2 | 0.0252 | **0.0116** | **−54%** | 0.0710 | **0.0387** | **−45%** |
| R3 | 0.0429 | 0.0353 | −18% | 0.1357 | 0.1332 | **−2%** |

**这是区分「离散误差」与「模型误差」的关键证据**：R2 的误差随模板分辨率**显著收敛**（真离散误差）；R3 的最大误差几乎**不收敛**（−2%），说明它是形状模型本身的表达力不足，无法靠加密模板解决。

---

## 6. 核心发现

### 6.1 R1 不保持固定拓扑

形状变化（chestWidth 1.00→1.15、pelvisWidth 1.00→1.12）下：

| Route | Verts before | Verts after | Vertex set fixed | Index-wise delta well defined |
|---|---|---|---|---|
| R1 SDF + Surface Nets | 5108 | **5342** | **NO** | **NO** |
| R2 quad template projected on SDF | 834 | 834 | yes | yes |
| R3 quad template on B-spline control lattice | 834 | 834 | yes | yes |

R1（Surface Nets）每次都从等值面重新推导顶点集合，形状一变顶点集合就变，因此**逐索引顶点对应关系不成立**（`maxVertexDelta` 返回 −1 哨兵，表示"不可测"而非"距离为负"）。这正是 R2 / R3 存在的理由，也是三条路线在 G2 必须分开评估的根因。

在 20 个 `Wired` 参数的逐项扫描中，R1 有 **16 个参数**的顶点集合发生变化（见 `evidence/g1/g1-route-tests.log`）。

### 6.2 E1 误差的空间分布：R2 是离散误差，R3 是模型误差

生产模板下的 E1 按高度分布（`evidence/g1/meshes/route-metrics.md`）：

**R2**（误差均匀散布，最大 0.0387）：

| 高度带 | 样本 | E1 mean | E1 max |
|---|---|---|---|
| y=[0.85, 0.95) | 440 | 0.0093 | 0.0210 |
| y=[0.95, 1.05) | 350 | 0.0082 | 0.0170 |
| y=[1.05, 1.15) | 552 | 0.0124 | 0.0365 |
| y=[1.15, 1.25) | 584 | 0.0143 | 0.0387 |
| y=[1.25, 1.35) | 92 | 0.0143 | 0.0359 |

最差点落在 `(|x|≈0.164, y≈1.20)` 与 `(|x|≈0.231, y≈1.25)`——腋窝与锁骨凹陷等**真实解剖细节**，32 段环无法贴合，属于可预期的离散误差。

**R3**（误差集中在上胸外侧，最大 0.1332）：

最差点 `err=0.1332 @ (x=+0.279, y=1.080, z=+0.008)`。参考面在该高度到达 `x=0.279`，而 R3 的模板环在该高度只到 `|x| = 0.1442`——**上胸外侧被 B-spline 包络模型截掉约 13.5 cm**。这不是离散误差，是包络模型（4 个关键高度 + 椭圆截面）**表达力不足**，与 §5.2 的"不随分辨率收敛"互相印证。

### 6.3 拓扑

R2 / R3 均 components=1 / boundary=0 / non-manifold=0 / degenerate=0 / invalidNormals=0。

> 修复过程记录：最初的 54 条 boundary edge 源于 seam 顶点重复（两个共线但不同的顶点）。修复方式是改为**周期性环（不重复 seam 顶点）**——`t.vertexCount`、两处 grid 分配、两处索引、两处 `j <= segments` 循环全部改为 seam-free 口径。

---

## 7. 范围边界：T2 尚未实现（重要）

本轮最小实验只覆盖 **T1 躯干主干**：

- 模板竖向范围 `y ∈ [0.844, 1.254]`（参考骨架 hips−0.02 → chest+0.00）；
- 顶部以**平盖**封闭；
- **不存在肩根开口/连接拓扑**，即 SPEC PoC-A 的 **T2（Shoulder-root Boundary/Connection Topology）未实现**。

T2 缺失在度量上的具体体现：R2 的最大误差正落在腋窝/肩根区域（`|x|≈0.164–0.231, y≈1.20–1.25`）。**补齐 T2 同时是 SPEC 范围要求，也是进一步压低 E1 的关键。**

---

## 8. G1 判定与 G2 准入

| 路线 | G1 判定 | 是否满足 G2 准入 | 理由 |
|---|---|---|---|
| **R1** | FEASIBLE（几何） | **不满足** | 顶点集合随形状变化（5108→5342），不满足固定拓扑前提；仅作参考路线与 E1 基线 |
| **R2** | **FEASIBLE** | **满足，建议进入 G2** | 固定拓扑（834 恒定）、拓扑干净（1/0/0/0）、参数可测（16/20 有效 + 3 个 DeclaredOnly 精确零 + 4 个可证遮蔽）、E1 最低（0.0116 / 0.0387）、误差随分辨率收敛、生成 51 ms |
| **R3** | FEASIBLE（几何） | 满足技术准入，但**不推荐** | 固定拓扑与拓扑质量同 R2；但参数覆盖率是 R2 真子集（7 vs 16）、E1 差 3.0×（0.0353 vs 0.0116）、最大误差不随分辨率收敛（上胸包络截断 13.5 cm） |

**G1 门禁规则「至少两个 FEASIBLE 路线」已满足**：R2 与 R3 均为 FEASIBLE。**建议只带 R2 进入 G2**，R3 作为备选保留、不参与 G2 公平比较主表（避免 E1/E2/E3 三层测量在两条实质同源的模板路线间重复消耗）。

### 8.1 G2 前必须补齐的缺口

1. **T2 肩根边界拓扑**——当前模板为封闭管道，肩根无开口/连接结构。这是 PoC-A 范围内的**未完成项**，且是 R2 残余误差的主要集中区。必须在 G2 公平比较前补齐。
2. **4 个 MASKED 参数**——`shoulderWidth`、`forearmLength`、`forearmVolume`、`wristWidth` 需在 T2 区域上复测；若仍遮蔽，必须按 DEC-06 降级为 `DeclaredOnly` 并同步文档。
3. **E2 / E3 测量层尚未实现**——当前仅有 E1（目标参考面 → 路线曲面）。G2 需要三层齐备。
4. **R1 的 `nonmanifold=2` / `degen=4`** 曾在个别参数扫描值中出现（`smoothness`、`axillaDepth` 变体），需在 G2 中作为 R1 自身的质量事实记录，不作为 R2/R3 的对比优势。
5. **参考面离散偏差**——参考面取自 44×66×26 的粗网格等值面，而 R2 投影到**连续** SDF，两者存在约 1 个网格单元（~16 mm）的系统性偏差。G2 应评估是否改用更细的参考分辨率，或以连续场采样定义参考面。

---

## 9. 交付物清单

| 交付物 | 路径 | 状态 |
|---|---|---|
| 参数契约实现 | `native/procedural/shape/ShapeModel.{h,cpp}` | 新增 |
| 固定拓扑模板实现 | `native/procedural/shape/QuadTemplate.{h,cpp}` | 新增（含 `torsoRadiusAt()`） |
| 路线实验框架 | `native/procedural/shape/PocRoutes.{h,cpp}` | 新增（含 `referenceJoints()`） |
| G1 单元测试（9 例） | `tests/unit/poc_routes_tests.cpp` | 新增 |
| G1 诊断探针 | `tests/unit/poc_route_probe.cpp` | 新增 |
| Host 目标接线 | `tests/CMakeLists.txt`（仅 host PoC 目标） | 修改 |
| 路线度量表 | `evidence/g1/meshes/route-metrics.md` | 生成 |
| R1 参考网格 OBJ | `evidence/g1/meshes/r1-sdf-surface-nets.obj` | 生成（5108 v / 10212 f） |
| R2 模板网格 OBJ | `evidence/g1/meshes/r2-quad-template-projected.obj` | 生成（834 v / 1664 f） |
| R3 模板网格 OBJ | `evidence/g1/meshes/r3-quad-template-bspline.obj` | 生成（834 v / 1664 f） |
| 粗分辨率对照 OBJ | `evidence/g1/meshes/*-coarse.obj` | 生成（各 162 v / 320 f） |
| 单元测试日志 | `evidence/g1/g1-route-tests.log` | 生成 |
| 全量 CTest 日志 | `evidence/g1/ctest-all.log` | 生成（10/10 PASS） |
| 探针日志 | `evidence/g1/probe.log` | 生成（DEF-02 修复后） |
| **修复前探针日志（DEF-02 原始状态）** | `evidence/g1/probe-pre-def02-fix.log` | 保留为对照（3172 样本 / 860 顶点 / 54 boundary / E1 0.0433） |
| G1 后 Android 增量重建日志 | `evidence/android-build/gradle-assembleDebug-g1.log` | 生成（APK 字节级未变） |

---

## 10. 单元测试用例清单（9 例，全部 PASS）

| # | 用例 | 断言要点 |
|---|---|---|
| 1 | `shape_parameter_contract_is_complete` | 23 参数 / 20 Wired / 3 DeclaredOnly；名称、单位、区域、区间、默认值合法性；3 个 DeclaredOnly 逐一点名核验 |
| 2 | `shape_parameter_write_validates_range_and_name` | 未知参数名拒绝；越界值拒绝；合法写入可读回 |
| 3 | `route_r2_r3_topology_is_parameter_independent` | R2/R3 顶点数、quad 数、三角形数在形状变化下恒定；且形状确实改变 |
| 4 | **`routes_share_one_skeleton`** | **三路线骨架 == `referenceJoints()`；且 != 硬编码 `BodyJoints{}`（DEF-02 回归防护）** |
| 5 | `route_r1_topology_is_not_fixed` | R1 顶点数 5108 → 5342 变化，逐索引 delta 不可测；R2/R3 恒定且可测 |
| 6 | `route_topology_is_clean` | components=1、boundary=0、non-manifold=0、degenerate=0、invalidNormals=0、quad 数符合模板规格 |
| 7 | `declared_only_parameters_have_zero_effect` | 3 个 DeclaredOnly 参数在 R2/R3 上 delta 精确为 0（阈值 1e-9） |
| 8 | `wired_parameters_effect_is_measured` | 20 个 Wired 参数逐项扫描并打印 delta；有效数 ≥ 10；MASKED 项显式标注 |
| 9 | `e1_is_measured_per_route` | 三路线共用同一目标参考面；E1 均有限且 > 0 |

---

## 11. 复现性说明

- Worktree 为 detached HEAD，基线 `99941a9`，**无新增 commit、无 push**；所有改动以工作区未提交状态存在，符合总指令「不得创建 Git commit / 不得 push」。
- 原始工作区 `C:/Users/Giant/Desktop/GITCODE/Procedural Avatar Engine` 全程只读，未被触碰。
- 本阶段无设备交互，因此**不涉及** G0 阶段发现的 `E:\AndroidSdk` 之外的新环境副作用。

---

## 12. G2 追加勘误（T2 实现后的口径变化）

> 本节由 G2 追加。上文 §3 / §6 / §7 / §8 中的模板顶点数 **834**、三角形数
> **1664**、以及 `boundary=0` 是 **T2 实现之前** 的 G1 测量值，已被 G2 的
> T2 肩根 crop 取代。G1 的可行性结论（R1/R2/R3 均 FEASIBLE、DEF-02 修正后的
> E1 方向）不变，但拓扑计数必须以本节为准。

| 指标 | G1 报告原值（T2 前） | G2 实测（T2 后） | 原因 |
|---|---|---|---|
| R2/R3 顶点数 | 834 | **833** | 顶环不再封口，去掉顶部中心点 |
| R2/R3 三角形数 | 1664 | **1632** | 去掉 32 个顶盖三角形 |
| R2/R3 quad 数 | 800 | 800 | 四边网格未变 |
| R2/R3 boundary | 0 | **32（已登记 crop）** | 顶环成为显式肩根 crop 边界 |
| 未登记 boundary / non-manifold / degenerate | 0 | 0 | 无裂缝、无孤岛 |

T2 的 crop 是**已登记**边界（一个闭合环、32 条边、位于胸廓标志点高度），
不是缺陷：G2 的 `TopologyAudit` 能区分已登记 crop 与未登记裂缝，生产
`TopologyReport` 只能数 boundary edge，无法做此区分。

- 断言 `route_topology_is_clean` 已改为：未登记 boundary=0、
  已登记 boundary == 32、crop 与边界环 1:1 匹配、crop 闭合、`audit.clean`。
- 新增断言 `t2_shoulder_root_boundary_is_explicit`：唯一 crop 标签
  `shoulder-root`，顶环闭合，顶点数/边数均为 32，连通分量 = 1。
- G1 报告 §7 中「不存在肩根开口/连接拓扑，T2 未实现」在 G2 已**关闭**；
  T3 肩部形变环与 T4/T5 仍按 SPEC §10.1 延后。

完整重新测量见 `evidence/g2/fair-comparison.md` 与
`evidence/reports/g2-fair-comparison-report.md`。
