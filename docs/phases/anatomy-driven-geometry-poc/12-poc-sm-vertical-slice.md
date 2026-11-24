# 12 — PoC-SM 最小垂直切片（Shared Skin Envelope 第一个可测结果）

> **状态：实现完成 + 已测量 / MEASURED**。本文件记录 AUTH-08 授权下的第一个 PoC-SM 增量。
> **这是测量，不是判定。** 没有任何 Gate 被标 PASS（AUTH-08 FB-12）。
> 分支：`poc-sm/shape-model-v2`（基线 `a32251b` + 治理上下文）。

---

## 1. 这轮做了什么

| 授权项 | 交付物 | 位置 |
|---|---|---|
| **AL-2** | `CharacterDefinition` 五层契约（host-only C++） | `native/procedural/shape/v2/CharacterDefinition.{h,cpp}` |
| **AL-3** | Shared Skin Envelope（一份连续外表面） | `native/procedural/shape/v2/SharedSkinEnvelope.{h,cpp}` |
| **AL-5** | OQ-03 固定口径的接缝测量工具 | `native/procedural/shape/v2/SeamAuditV2.{h,cpp}` |
| — | 3 个 host 测试套件 + 测量探针 | `tests/unit/poc_sm_*.cpp` |

**没碰的**：`native/` 下 v2 目录之外的任何源文件、BoneId 0–21、UBO[22]、64B packing、
JNI ABI、RenderThread 生产入口、legacy 三路径（AUTH-08 FB-1..FB-12 全部未触碰）。

---

## 2. 测试结果

```
14/14 suites pass, 0 failures
```

| 套件 | 用例数 | 结果 |
|---|---:|---|
| 既有 11 个套件（core / geometry / fusion / … / poc_route） | — | **全部通过，无回归** |
| `pae_poc_sm_contract_tests`（AL-2） | 9 | 通过 |
| `pae_poc_sm_envelope_tests`（AL-3） | 8 | 通过 |
| `pae_poc_sm_seam_tests`（AL-5） | 9 | 通过 |

值得注意的既有测试（说明契约真的被钉住了）：

- `waist_half_axes_reproduce_the_measured_circumference` —— 由腰围解出的椭圆周长必须**回到实测的 934 mm**（CDC NHANES SR50，男性 20–29 中位）；
- `measured_anthropometric_values_survive_the_translation` —— ANSUR II 的身高 / 肩宽 / 髋宽 / 胸深数值在进入契约后不变；
- `profile_is_continuous_in_height` —— 400 个采样点上最大高度步进 < 2 mm（这条测试抓出了两次真实的实现错误，见 §5）；
- `the_five_degree_contract_needs_at_least_72_segments` —— 把 §4 的口径结论钉成测试，防止被静默遗忘。

---

## 3. 测量结果（32 段 / 26 环）

### 3.1 肩根接缝 —— OQ-03 固定口径

| 项 | 值 |
|---|---|
| Applicability | `APPLICABLE` |
| **max** | **21.76°** —— **超过 ≤5° 合同值** |
| mean | 11.02° |
| p95 | 21.76° |
| 绕序校验 | 通过（0 不一致） |
| 底盖扇形 | 32 三角，max 0.028°（单独标注） |
| 拓扑 | components=1，未登记 boundary=0，sliver=0，正交 quad 比 0.994，平均最小内角 87.8° |

**`G-P1-GEO` = FAIL（就该指标而言）**，与 G2 的结论方向一致（R1/R2 47.66°、R3 16.13° → PoC-SM v2 21.76°）。

### 3.2 分段扫描 —— 这个角度里有多少是网格、多少是形状？

| Segments | max (deg) | faceting 360/S (deg) | **excess (deg)** | within 5° |
|---:|---:|---:|---:|:--:|
| 32 | 21.76 | 11.25 | **10.51** | no |
| 48 | 14.80 | 7.50 | **7.30** | no |
| 64 | 11.18 | 5.63 | **5.55** | no |
| 96 | 7.58 | 3.75 | **3.83** | no |
| 128 | 5.71 | 2.81 | **2.90** | no |

**发现（本切片最重要的输出）**：excess × S 近似常数（336 → 371），
即 **excess ≈ 370/S，max ≈ 730/S**。也就是说：

1. 这个测量值**由离散化主导**，不是由形状缺陷主导；
2. 一个**完美圆柱**在 32 段下也会测出 11.25°（已被测试钉住）；
3. 要让 max ≤ 5°，按外推需要 **S ≈ 146 段**；
4. 因此 "R2 加密不收敛（max 仅 −2%）" 的 G2 结论**在本口径下需要重新解读**：加密是有用的（excess 随 1/S 下降），只是它要对抗的基线（360/S）也在同一量级。

---

## 4. ⚠️ 需要负责人裁定的问题：5° 的口径可能量错了东西

这是本轮**必须上报**的发现。两份文件的语义不一致：

| 来源 | 对 ≤5° 的定义 |
|---|---|
| `docs/foundation/03-geometry-mesh-contract.md`（合同原文） | "缝**两侧**位置误差 ≤1e-4 m，**法线夹角 ≤5°**" —— 指**两个拼接组件之间**跨缝的法线夹角；"Open seam（独立组件允许开放边界）必须显式登记" |
| OQ-03 固定口径（2026-11-24 签署） | **crop 环带上相邻 quad** 的面法线夹角 —— 即**环带内部的多边形离散化** |

合同说的是"A 面与 B 面跨缝夹角"；而 PoC-A 的 torso 本身没有"另一侧"（手臂不存在），
所以 OQ-03 口径实际测的是**环带的多边形离散化** —— 那是一个由段数决定的量（360/S）。

**后果**：按现口径，≤5° 在段数 < 146 时**对任何完美形状都不可能达到**。
这不是"形状做得不够好"，而是**测量对象可能选错了**。

**两个候选出路（需负责人决定，AI 不代选）**：

| 选项 | 内容 | 含义 |
|---|---|---|
| **A** | 维持现口径，把合同阈值改为"excess = max − 360/S ≤ 5°"（把离散化基线扣除） | 口径改动小，阈值语义变成"形状本身的突变"；需重写 OQ-03 决策记录 |
| **B** | 改测**跨缝**夹角：等手臂接入后，测 torso 环与 arm 环在共享边上的法线夹角（合同原文语义） | 才是合同原意；但 torso-only 阶段该值为 `NOT APPLICABLE` |
| **C** | 维持现口径 + 阈值不变，把段数提到 ~150 | 代价是网格量翻 4.7 倍，且"接缝平滑"并没有真正被测到 |

> **在裁定之前，21.76° 这个数不得被用作任何判定依据** —— 正如 OQ-03 决策记录第 5 条
> 对 47.66°/16.13° 所要求的那样：口径未定，数值不作判定。

---

## 5. 实现过程中被抓出并修掉的真实错误

| 错误 | 怎么被抓到的 | 修复 |
|---|---|---|
| 三角形绕序写反（`(a,b,c)` 而非 `(a,c,b)`）→ 所有面法线朝内 | `all_face_normals_point_outward` | 改为模板一致的 `(a,c,b)`/`(a,d,c)` |
| 区域样条只有 3 个控制点（三次 B 样条需 ≥4）→ 整个包络静默退化成 0.1 mm 宽的管子 | `profile_matches_the_measured_anchors` | 每个区域给足 4 个锚点 |
| `evaluateProfile` 与网格构建器各有一份剖面逻辑 → 两者漂移 | `profile_matches_the_measured_anchors` | 抽出唯一的 `buildProfileData()`，两者共用 |
| 钳位均匀 B 样条**只过端点、不过内部锚点** → 实测锚点被近似值替换 | `profile_is_continuous_in_height`（斜率断裂） | 换成**插值型**自然三次样条（C2 且过所有锚点） |
| 三个区域权重用独立的 bandWeight，不互补 → 交接处出现折痕 | `profile_is_continuous_in_height` | 改为**精确归一**的三权重（1−S1、S1(1−S2)、S1·S2），和恒等于 1 |
| 腋窝凹槽用二次衰减 → 渲染图上是锐利三角缺口 | **肉眼（灰模）** | 改为四次衰减（C3），跨度加宽 |
| 底盖扇形角度用合成反向法线 → 恒读 180° | `cap_fan_is_counted_and_reported_separately` | 改用网格自身三角形的真实法线 |
| `auditTopology` 不填 `faces`/`flow`（G2 就是这么用的）→ 报告里出现 0.00/0.000 | 报告审阅 | 探针显式调用 `measureFaceQuality` / `measureEdgeFlow` 并合并 |

另：**G2 模板的底盖法线朝体内**（`center,v1,v0` 顺序）—— 这正是 OQ-16 记录的"骨盆扇盖 144 个法线翻转"的根源。v2 用 `(center,v0,v1)` 修正，未改动 G2 代码。

---

## 6. 这份报告不说什么

- 不标记任何 Gate PASS（AUTH-08 FB-12）；
- 不判定 `G-P1-GEO`：DEC-05 未批准，SPEC §13 其余阈值仍为 `TBD`；
- 不覆盖手臂、前臂、颈部、头部（SPEC 10.1 T1/T2 之外）;
- 不把 21.76° 用作判定 —— 见 §4，口径本身待裁定。

---

## 7. 下一步（按优先级）

| # | 事项 | 谁决定 |
|---|---|---|
| 1 | **裁定 ≤5° 口径**（§4 的 A/B/C 三选一）—— 这决定后续所有接缝工作的目标 | 负责人 |
| 2 | 裁定后重算 G2 三路线的同口径数值（47.66°/16.13° → excess 口径） | AI |
| 3 | 灰模已产出（7 视角）→ 可作为 DEC-05 判定的输入（负责人已选择"等灰模出来再定"） | 负责人 |
| 4 | OQ-16（骨盆法线翻转）在 v2 中已按修正绕序重建，需复测确认 | AI |

---

## 8. 证据清单

| 项 | 位置 |
|---|---|
| 测量报告 | `evidence/poc-sm/poc-sm-shared-envelope-report.md` |
| 灰模 7 视角 | `evidence/poc-sm/shared-envelope-*.png`（front / front-45-left / side-left / side-left-30up / back / back-45-right / front-30up） |
| 相机协议 | 复用 G2 已冻结的 `CameraProtocol::fixedViews()`，与 G2 证据同框可比 |
| 测试 | `build-host` 下 ctest 14/14 通过（修改前基线，日志 `evidence/poc-sm/round-2/baseline-ctest.log`） |

---

## 9. Round 2 增量（本文件不重复完整测量数据）

本轮沿用本报告，**未新建 PoC-SM 总报告**。所有原始日志、矩阵和图片放在
`evidence/poc-sm/round-2/`，本节只给摘要与链接。

| 主题 | 本轮结论（摘要） | 原始证据 |
|---|---|---|
| 全量回归 | 修改前 `14/14`，本轮 **`17/17 suites, 0 failures`**（新增 3 个套件：跨缝 9 例 / AL-6 9 例 / 语义 7 例） | `final-ctest.log`、`baseline-ctest.log`、`full-build.log` |
| 历史环带测量 | 未修改 `measureShoulderRootSeam()`；复测仍为 **21.7622°**（32 段），探针按设计退出码 1 | `baseline-probe-stdout.log`、`poc-sm-shared-envelope-report.md` |
| **B — 跨缝实验** | 16 组矩阵：旧环带统计恒为 360/S（32 段 11.2500° → 128 段 2.8127°），**与已知折角无关**；新跨缝统计稳定读出折角（15° 折角：14.9311° → 14.9958°）。绕序翻转 / 边界错位 / 缺侧均返回 `NOT_APPLICABLE`。**实验，不是验收定义** | `cross-boundary-seam-report.md`、`cross-boundary-seam-matrix.csv` |
| **C — AL-6 差分** | 10 个 ShapeParameter 全部遍历：9 个 `Wired` 均产生非零位移（`shoulder-girdle-height` max 68.20 mm；`axilla-depth` max 27.72 mm / mean 0.19 mm，局部凹面）；`spine-curve` = 0.000 mm。9 个 ProportionProfile 字段中 `shoulderBreadthRatio` / `biacromialRatio` / `abdominalDepthRatio` / `headHeightRatio` = 0.000 mm | `al6-parameter-differential-report.md`、`al6-shape-parameter-delta.csv`、`al6-proportion-delta.csv` |
| **D — 语义落地** | 五层契约齐全；**`AnatomicalLandmarks.placed` 仍为空**（全部 catalogue 未放置）；`JointDeformationRegions` 仍为接口；BodyJoints 仍是独立输入语义来源 | `pae_poc_sm_semantics_tests`（7 例全过） |
| **E — 注释准确性** | 删除 `SharedSkinEnvelope` 中过时的 "Clamped B-spline" 描述，改为实际的**插值型自然三次样条**；明确当前能力边界，不宣称完整 3D 解剖场 | 源码 diff |
| 灰模 | 本轮新生成 9 张：7 个 `fixedViews()` 视角（与上一版同框可比）+ 肩胸 / 腋窝近景；几何未改动，视觉与上一版一致 | `views/*.png`、`views/camera-manifest.csv` |
| 状态 | 无 Gate PASS；OQ-03 定义待裁定；OQ-01 / DEC-05 待负责人视觉审查 | `docs/project-status.md`、`07-open-issues.md`、`06-owner-signoff-checklist.md` |
