# VR-1B-WP3-HOST-R1 — 人体参数化实测结果复核与修正报告

> 日期：2026-11-27
> 授权：VR-1B-WP3-HOST-R1（负责人会话批准；Host-only 审查修正，不增加新参数、不接入 Android）。
> 审查基线：`poc-sm/shape-model-v2` @ `56c5f9f8a9fb3d5017ab3845eeee05ef1a79c23c`（WP3-HOST-01 报告与证据）。
> 相关提交：WP3 Host 工具代码 `d7e83ee1ddca079cb1a190c4a25819bdefd038ca`；WP3 报告与证据 `56c5f9f8a9fb3d5017ab3845eeee05ef1a79c23c`。
> 本轮修改：仅 `tools/human-template-poc/host/` 4 个源文件（+492/−30 行）；**未 commit/push（授权要求），等负责人审查**。
> 证据：`evidence/vr1b-wp3-host-01/r1/`（18 份数据/报告 + 153 张图 + 4 份日志）；原始 WP3-HOST-01 记录在上级目录原样保留。

## 0. 状态

```text
WP3_HOST_R1 = PASS
```

四个审查问题全部查明根因、修复（或澄清）并从代码复测；关键矛盾全部解释；全量回归通过；Android 生产路径零改动。详见 §12。

## 1. 当前代码基线和提交

- 审查起点 HEAD `56c5f9f`，与远端一致（0/0）。工作区已跟踪文件干净，无其他代理近期写入。
- 本轮改动文件（git diff --stat）：
  - `wp3_experiment_main.cpp` +340（锚点修复、锚点验证、翻面调查、截面验证、联动矩阵新语义、截面可视化）
  - `wp3_measure.cpp` +62（regionLinkage 重写、validateTorsoSection、SectionLoop.pts）
  - `wp3_measure.h` +45（RegionLinkage 新字段、SectionValidation、SectionLoop.pts）
  - `wp3_tests.cpp` +75（6 项联动语义测试 + 截面验证测试）
- 固定资产未改：base mesh SHA `8e761e66…f6fb4c`；分发模板 `.paht` v3 SHA `d3bcbbe…a5e417b`（与 WP1/WP2 已验证资产逐字一致）。

## 2. 本轮问题清单

| # | 级别 | 问题 | 结论 |
|---|---|---|---|
| 1 | P0 | 身高变换缩放中心写错（`D3{sole,sole,sole}`） | **属实，已修复** |
| 2 | P0 | shorter-pae 质量 CSV “2” 与报告 “0” 矛盾 | **澄清：CSV 列误读；flipped_faces 实为 0，"2" 是 degenerate 列；退化根因查明** |
| 3 | P1 | 联动矩阵“平均位移”是 affected-only，非 whole-region | **属实，已拆分语义** |
| 4 | P1 | 截面测量缺少真实人体区域验证 | **已补解剖验证 + 可视化** |
| 5 | 治理 | 18 报告 §0 过期 Git 状态 | **已修订** |
| 6 | 治理 | 14/14 不是完整工程回归 | **已跑全量回归** |

## 3. 每个问题的根因

- **问题 1**：`applyVariant()` 把足底 Y 坐标 `sole`（≈−0.8168 m）同时当作 X/Y/Z 缩放中心。均匀缩放绕任意中心虽不改变比例，但中心偏离角色轴线会引入水平平移 `sole·(1−k)`。1.55 m 时 k=0.9304，水平漂移 ≈−0.0568 m（5.68 cm）。
- **问题 2**：`variants-quality.csv` 列序为 `…,nonmanifold,degenerate,dup_vertices,…,candidate_pairs,flipped_faces,…`。shorter-pae 行的 `2` 落在 **degenerate**（第 6 数据列），**flipped_faces（第 11 数据列）实为 0**。审查把 degenerate 误读为 flipped。退化的真因：`pae::analyzeTopology`（`native/procedural/MeshTopology.cpp:45`，只读复用）以**绝对阈值** `float cross lengthSq < 1e-14f`（面积 < ~5e-8 m²）判退化；均匀缩小到 1.55 m 使面积按 k²=0.8656 收缩，2 个最小三角形跌破阈值（叠加 float 舍入）。这是检查器的尺度相关特性，**非几何缺陷、非翻面**。
- **问题 3**：`regionLinkage()` 的 `mean` 只对 target 实际移动（非零 delta）的顶点求平均，即 affected-only mean。当某区域仅 10% 顶点被移动时，该值不代表整个区域的平均变化。
- **问题 4**：`measureAll()` 用 `largestArea()` 选躯干环，比整截面求和合理，但缺少“选中环确属躯干”的空间验证与环闭合统计。

## 4. 修改前后代码差异（关键 hunk）

**P0-1 锚点**（`wp3_experiment_main.cpp`）：
```cpp
// 前（WP3-HOST-01）：
else p = D3{sole, sole, sole} + (p - D3{sole, sole, sole}) * k;
// 后（R1）：
const ScaleAnchor a = scaleAnchor(c);   // pivot=(rootX, soleY, rootZ)
else p = a.pivot + (p - a.pivot) * k;
```
`scaleAnchor()`：rootX = body bbox x 中心（=0，与矢状对称面重合，vertex mean x=6.6e-16）；soleY = min body y；rootZ = rig Root 骨 head z（权威角色根节点）。

**P1-1 联动**（`wp3_measure.cpp`）：新增 `total[]`（区域全部顶点普查）、`meanAll = sum/total`、`coverage = affected/total`、`p95`（affected 位移排序）、helper 独立普查；保留 `meanAffected`（旧 `mean` 别名）。

**P1-2 截面**（`wp3_measure.cpp/.h`）：`SectionLoop` 增 `pts` 折线；新增 `validateTorsoSection()`（largestArea 候选 + 质心 |x−centerX|≤0.10 m 空间确认 + openChains==0）。

## 5. 身高锚点修正（`r1/data/anchor-study.md/.csv`）

pivot=(0.000000, −0.816760, −0.077920) m，mirror-symmetric=YES。9 项检查 × 4 身高（1.55/1.6659/1.7549/1.85）**全 PASS**：

| 检查 | 结果 |
|---|---|
| 足底锚点不变 | soleErr = 0 |
| 角色根节点不变 | rootZErr = 0 |
| X/Z 中心线无漂移 | meanX drift ~1e-16 m |
| 身高精确达成 | heightErr ≤ 4.4e-16 m |
| 肢段比例恒定 | upperArm/stature 0.13188、thigh/stature 0.24984 |
| 左右关节对称 | L/R symErr = 0 |
| helper/joint 同步 | helperErr = 0 |
| 重复生成一致 | repeat diff = 0 |
| 四身高复测 | 全 PASS |

旧 bug 水平漂移量化：1.55 m 时 −0.0568 m（x 与 z）。**缩放变体的 45 张图（3 变体×5 视角×3 模式）已重生成消除漂移**；非缩放变体几何不变。

## 6. shorter-pae 翻面调查（`r1/data/flip-investigation.md`）

- 从代码重跑：`compareSurfaces` **flipped_faces = 0**，**minNormalDot = 1.000000**（均匀缩放 k>0 保持每个面朝向；Newell 法线方向尺度不变）。volumeRatio 0.805483，edgeRatioP99 1.0748。
- CSV 三处统一：baseline / taller-pae / shorter-pae 的 flipped_faces **均为 0**；18 报告（0）与 height-study（uniform 1.55 = 0）与 CSV 一致，**无翻面矛盾**。
- “2” 的真身 = **degenerateTriangles**，定位到 2 个三角形：

| quad face | tri | vertex ids | base 面积 (m²) | 1.55 m 面积 (m²) | 比 | float crossLenSq | 法线点积 |
|---:|---:|---|---:|---:|---:|---:|---:|
| 320 | 0 | 428,500,501 | 5.501e-08 | 4.763e-08 | 0.8657 | 9.073e-15 | 1.000000 |
| 6947 | 1 | 7188,7250,7249 | 5.501e-08 | 4.763e-08 | 0.8657 | 9.073e-15 | 1.000000 |

- 根因：绝对阈值 1e-14f + 缩小 k²=0.8656 + float 舍入。面积比 0.8657 ≈ k²，证实是纯尺度效应。
- 该检查器在 `native/`（白名单外），**只读复用未修改**；建议 WP3-Production 改用尺度相对阈值或在原生尺度评估，使退化普查与身高无关。

## 7. 新联动矩阵（`r1/data/linkage-matrix.md/.csv`，修正语义）

每区域同时给出 **meanAll**（whole-region）与 **meanAffected**（affected-only）及 **coverage**。α=+1 的 whole-region 联动（mm）：

| 参数 | 主要 whole-region 联动（meanAll） | 与旧 affected-only 的差异 |
|---|---|---|
| shoulderWidth | 上臂/前臂/手 24.8–24.9（coverage 1.0），肩带 11.94，胸 1.68 | 胸旧 9.29→今 meanAll 1.68（coverage 0.18） |
| ribcageWidth | 上臂/前臂/手 34.0–34.2（coverage 1.0），胸 13.90，腹 5.84 | 腹旧 8.1→今 5.84 |
| chestDepth | 胸 20.78，腹 11.56，肩带 6.55，**上臂 1.55** | 上臂旧 7.2→今 meanAll 1.55（coverage 低，旧值高估） |
| waistWidth | 腹 17.70，**骨盆 3.21**，胸 1.33 | 骨盆旧 8.7→今 3.21 |
| hipWidth | 骨盆 18.61，**大腿 1.29**，腹 0.39 | 大腿旧 5.3→今 1.29 |

**结论**：胸廓宽确实显著移手臂（coverage 1.0，whole-region 与 affected 一致）；但 chestDepth→上臂、waistWidth→骨盆、hipWidth→大腿的 whole-region 影响远小于旧 affected-only 数字——旧报告把这些区域少数顶点的位移当成了整个区域的平均变化。参数响应表与 UI 提示应以 meanAll + coverage 为准。

## 8. 截面闭合和解剖归属证据（`r1/data/section-validation.md/.csv` + `images/section-base-*.png`）

- 验证范围：base + 10 变体 + 5 参数 ×±1 极值 = 21 配置 × 胸/腰/髋 = **63 截面**。
- 每截面输出：loop count / closed loop count / open chains / 选中 loop ID / 质心 x / 躯干确认 / 围度 / 选择原因。
- **63 截面全部 openChains=0，全部 torsoConfirmed=YES，0 个 NOT_VERIFIED**。
- base 躯干环质心 x：胸 −0.0005、腰 +0.0005、髋 −0.0000 m（≈矢状面），证实 largestArea 选中的确是躯干环；手臂环（A-pose）位于 |x|~0.4 m 被排除。胸 3 环、腰 5 环、髋 1 环。
- 截面可视化（`section-base-chest/waist/hip.png`）：红色躯干环闭合且关于 x=0 对称，灰色手臂环远离中心线，腰部多出前臂/手小环。图像描述模型确认（**仅供参考，负责人人眼为准**）。
- 口径声明：largestArea 仍是候选策略，解剖正确性由空间确认背书；若某截面无法可靠识别将标 NOT_VERIFIED（本轮无）。

## 9. 全量测试结果（`r1/logs/`）

| 测试 | 结果 |
|---|---|
| WP3 专项（`wp3-tests.log`） | **33 passed / 0 failed**（原 14 + 6 联动语义 + 截面验证等） |
| 原有 Host 全量（`ht-tests.log`） | **65 passed / 0 failed** |
| 引擎 Host CTest（`engine-ctest.log`） | **22/22**（WP1 模板格式/加载、WP2 往返/哈希、Legacy 回归） |
| 实验（`experiment.log`） | 10 变体 + 身高专项 + 150 图 + 63 截面；全部 flips=0 / selfInt=0；15.8 s |

Android 生产路径：源文件/JNI/PAHT v3 **零改动**（git diff 证明，§10）；本轮 Host-only，**未构建 APK、未部署设备**。

## 10. 视觉证据更新与白名单证明

- `r1/images/` 153 张 = 150 变体多视角 + 3 截面可视化；缩放变体图因锚点修正重生成。全部无损重压缩（像素往返断言通过），94.3 MB→2.62 MB。
- 白名单证明：`git status --porcelain` 已跟踪改动仅 4 个 `tools/human-template-poc/host/src/` 文件；`app/`、`native/renderer/`、`native/runtime/`、`native/procedural/human_template/`、`app/src/main/cpp/` 匹配为空。`.paht` 资产 SHA 未变。
- 历史数据保留：原始 `../data/`、`../images/` 未覆盖；R1 结果独立存 `r1/`，supersession 映射见 `r1/README.md`。

## 11. 未解决问题 / 未验证

| 项 | 状态 |
|---|---|
| `analyzeTopology` 退化绝对阈值的尺度相关性 | 已记录根因；**未修改**（白名单外）；WP3-Production 建议改尺度相对阈值 |
| α 超出 ±1 / macro 组合 / authored slider 范围 | NOT RUN（沿用 WP3-HOST-01 边界） |
| macro height 单独用于 statureM | NOT VERIFIED（明确否定，沿用） |
| ANSUR 验收口径对齐 | NOT RUN（WP3-Production 前置） |
| 峰值内存 | NOT MEASURED（Cygwin 采样不可用） |
| 截面质心为折线顶点均值（非面积质心） | 躯干环 x 对称故 x≈0 不影响判定；非对称环需注意（已注明） |
| Android 集成 / 生产 schema / PAHT v3 | 未改（授权禁止） |

## 12. WP3-H 最终状态建议

```text
WP3_HOST_R1 = PASS
```

- 四个审查问题：P0-1 修复并 9 项验证全过；P0-2 查明为 CSV 列误读 + 退化绝对阈值特性（flips 实为 0，三处统一）；P1-1 语义拆分并 6 项单元测试；P1-2 解剖验证 63 截面 0 NOT_VERIFIED + 可视化。
- 全量回归 33 + 65 + 22 全过；Android 生产路径零改动；历史数据保留。
- **建议**：WP3-H 参数化实测结果现已真实、准确、可重复，可作为下一阶段可信依据。按负责人路线，下一步进入 **WP3-R2（Human Parameter Solver Design）**，重点研究语义参数→Target 组合求解、非加性联动处理、超范围回退；**暂不批准 Android WP3-Production**。
- 本轮未 commit/push，等负责人审查后授权入库。