# VR-1B-WP3-HOST-01 — 人体参数化映射研究实测报告

> 日期：2026-11-27
> 授权：VR-1B-WP3-HOST-01（负责人会话批准；Host-only，不修改 Android 生产路径）。
> 基线：`poc-sm/shape-model-v2` @ `091a414`（含 WP1/WP2/WP2.1，DEVICE_STATIC_RENDER_ACCEPTED）。
> 工具：`tools/human-template-poc/host/` 新增 `wp3_measure.*`、`wp3_quality.*`、`wp3_experiment_main.cpp`、`wp3_tests.cpp`（`pae_wp3_experiment` / `pae_wp3_tests`）。
> 证据：`evidence/vr1b-wp3-host-01/`（150 张图 + 13 份数据/报告 + 运行/测试日志）。
> 前序初步研究：`18a-wp3-preliminary-shape-study.md`（本轮取代其口径；历史数值不改，差异见 §6）。
> **Git 状态（2026-11-27 修订）：已提交并推送。Host 工具代码 `d7e83ee`；报告与证据 `56c5f9f`；远端 `poc-sm/shape-model-v2` 与本地一致。本轮生产 Android 代码未修改。**
> **审查修正：本报告部分口径已由 `19-wp3-host-r1-audit-correction-report.md`（WP3-H-R1）复测取代——身高缩放锚点（§5B）、联动矩阵语义（§4）、shorter-pae 质量列读法（§7）、截面解剖验证（§6）。历史数值原样保留，R1 复测数据在 `evidence/vr1b-wp3-host-01/r1/`。**

## 0. 状态

```text
WP3_HOST_PROOF = IMPLEMENTED / MEASURED
```

五类核心参数均有实测几何响应；身高问题独立解释并实验对比；incr/decr 非对称分别建模；组合无翻面/无自交（±1 内）；helper/joint 随形态移动；变体从同一基础模板确定性重建；150 张多视角证据；Android 生产路径零改动。

## 1. 固定资产与参数基线

- Base mesh：`base.obj` SHA-256 `8e761e6624b8f54536409135d1636da63b32486a90d4897f84e121d144f6fb4c`，commit `a8bc2d54ff0ac92e78ff71431b1023eda42bf482`，CC0-1.0。体表 13,380 顶点 / 13,378 四边形；full OBJ 19,158 顶点（helper/joint 5,778）。
- 分发模板 `.paht` v3 SHA-256 `d3bcbbece3db11c0df79cb7202fdf1730b325108509d60ad9faa209a8a5e417b`；**18/18 target 与资产逐值等价**。
- 基准测量（PAE 自定义曲面口径）：stature 1.6659 m；biacromial-proxy 0.3712；shoulder-yoke 0.4010；outer-shoulder 0.5595；chest 0.2704×0.1972 / 围 0.7987；waist 0.2958×0.1831 / 围 0.7911；hip 0.3601×0.2061 / 围 1.0036；上臂围 0.2493 m。
- 基准质量：1 连通分量、0 边界、0 非流形、0 退化、0 自交；体积 0.054895 m³。
- CharacterDefinition statureM 默认 1.7549 m（ANSUR II 男性 p50）——与 base mesh 差 89 mm 是**参考人群口径差异**，不是缺陷（§5）。

## 2. Target 使用清单与许可

见 `data/target-manifest.md`：18 个 target 逐个登记文件名、SHA-256、体表/helper 行数、α 范围。全部 CC0 文件头 + LICENSE.md §C 列举。`default.mhskel` 仅作 host-only 关节命名参考（PENDING_CONFIRMATION，不分发）；`default_weights.mhw` 本轮未使用。

## 3. 参数映射与 α 响应曲线（实测，非假设）

`data/response-curves.csv/.md`：5 参数 × α∈{−1,−0.75,−0.5,−0.25,0,0.25,0.5,0.75,1}，正负方向分别用 incr/decr 独立文件（非对称，不做对称插值）。每行含全部测量 + 翻面/自交/边拉伸/体积比。

**±1 端点实测（m）**：

| 参数 | 测量 | −1 | base | +1 | 非对称 |
|---|---|---:|---:|---:|---|
| shoulderWidth | biacromial-proxy | 0.3444 | 0.3712 | 0.4210 | +49.8 / −26.8 mm |
| shoulderWidth | shoulder-yoke | 0.3740 | 0.4010 | 0.4508 | +49.8 / −27.0 mm |
| shoulderWidth | outer-shoulder | 0.5325 | 0.5595 | 0.6093 | +49.8 / −27.0 mm |
| waistWidth | waist 围 | 0.6941 | 0.7911 | 0.8913 | +100.2 / −97.0 mm |
| waistWidth | waist 宽 | 0.2789 | 0.2958 | 0.3127 | +16.9 / −16.9 mm |
| hipWidth | hip 围 | 0.9118 | 1.0036 | 1.1877 | +184.0 / −91.8 mm |
| hipWidth | hip 宽 | 0.3273 | 0.3601 | 0.4258 | +65.7 / −32.8 mm |

**围度 ≠ 宽度**：waist 宽 +16.9 mm 对应围 +100.2 mm（深度同时 +46.1 mm）；hip 宽 +65.7 mm 对应围 +184.0 mm。仅测宽度会严重低估围度响应。

**临界 α**：±1 内所有曲线 0 翻面、0 自交——未发现临界点（范围外未测，标记实验）。

## 4. 参数联动矩阵（α=+1，平均位移 mm）

`data/linkage-matrix.csv/.md`。非目标区域联动（实测，必须列入参数响应表）：

> **R1 更正**：本表“平均位移”是 **affected-only mean**（仅 target 实际移动的顶点），不是 whole-region mean。修正后的 meanAll/meanAffected/coverage 见 `19` 报告 §7 与 `r1/data/linkage-matrix.*`。下表历史数值保留。

| 参数 | 主要联动 |
|---|---|
| shoulderWidth | 上臂/前臂/手 ~24.9 mm 外移，肩带 18.2，helper 22.3 |
| ribcageWidth | **上臂/前臂/手 34.0–34.2 mm**（比胸廓自身 14.5 还大），腹 8.1，颈 1.3 |
| chestDepth | 胸 20.8，**腹 11.7**，肩带 7.6，上臂 7.2，骨盆 2.6 |
| waistWidth | 腹 17.7，**骨盆 8.7**，胸 3.1 |
| hipWidth | 骨盆 18.6，**大腿 5.3**，腹 1.5 |

## 5. 身高专项研究

`data/height-study.md/.csv`。

- **基线差异解释**：base mesh 1.6659 m 是 MakeHuman 中性基准；1.7549 m 是 ANSUR II 男性 p50——不同参考人群。macro height +1 = 2.3884 m（+722.5 mm），因为该 target 编码 MakeHuman macro 组合内的全滑块范围，不是标定米制缩放。
- **A. Macro 单独使用**：非线性（−1→1.2995、+0.25→1.8465、+0.5→2.0271、+1→2.3884），且肢段比例漂移（upperArm/stature 0.1319→0.1467，thigh/stature 0.2498→0.2662）。**NOT VERIFIED，不得把 statureM 直接接到 macro height。**
- **B. PAE 自有均匀缩放**（绕足底平面，full OBJ 含 helper 同步）：1.55/1.6659/1.7549/1.85 m **精确达成**；肢段比例恒定（0.13188/0.24984）；关节高度精确随 k 缩放（1.3197→1.3902 @1.7549）；0 翻面/0 自交/边比 1.0。**推荐为 WP3-Production 的身高方法。**
- **C. 仅垂直缩放**：对照记录，改变头身比例，不推荐。

> **R1 更正**：B 节均匀缩放原实现误用 `D3{sole,sole,sole}` 作缩放中心，引入 ~5.68 cm 水平漂移（1.55 m）。已修正为 pivot=(rootX=0 矢状对称面, soleY, rootZ=root 骨 head)，9 项锚点检查全过；缩放变体图已重生成。见 `19` 报告 §5 与 `r1/data/anchor-study.*`。身高/比例/关节同步结论不变。

## 6. 测量协议与本轮修正

`data/measurements-protocol.md`：每项测量的平面位置/采样/区域/方向/公式/单位/精度全部定义。**所有值为 PAE 自定义曲面测量，不是 ANSUR/MakeHuman 官方值。**

**本轮发现并修正的口径缺陷**：初版整截面周长把 A-pose 下垂手臂的截面环也算入（腰围误报 1.3086 m、上臂围误报 1.2889 m）。修正：截面按环分离，躯干围度取**最大面积环**，上臂围取**距骨中点最近的环**。修正后腰围 0.7911 m、上臂围 0.2493 m（解剖合理）。单元测试覆盖双环分离（`wp3_tests` 14/14）。初步研究（18a）的围度未测，宽度数据仍有效；差异以本报告为准。

## 7. 体型变体（10 个，参数向量显式，同一基础模板，无叠加）

`data/variants.csv`、`variants-quality.csv`、`variants.md`。

> **R1 澄清**：`variants-quality.csv` 中 shorter-pae 的 `2` 在 **degenerate** 列（退化三角形），**flipped_faces 列实为 0**（与本表一致）。2 个退化三角形（face 320/6947）源于检查器绝对阈值 + 缩小 k²，非翻面、非几何缺陷。见 `19` 报告 §6 与 `r1/data/flip-investigation.md`。

| 变体 | 参数向量 | stature | waist 围 | hip 围 | 翻面 | 自交 |
|---|---|---:|---:|---:|---:|---:|
| baseline | (base) | 1.6659 | 0.7911 | 1.0036 | 0 | 0 |
| broader-shoulder | shoulderWidth+1 | 1.6659 | 0.7911 | 1.0036 | 0 | 0 |
| broader-ribcage | ribcageWidth+1 | 1.6659 | 0.7912 | 1.0036 | 0 | 0 |
| narrower-waist | waistWidth−1 | 1.6659 | 0.6941 | 1.0036 | 0 | 0 |
| wider-hip | hipWidth+1 | 1.6659 | 0.8042 | 1.1877 | 0 | 0 |
| taller-pae | uniformScaleTo 1.7549 | 1.7549 | 0.8334 | 1.0573 | 0 | 0 |
| shorter-pae | uniformScaleTo 1.55 | 1.5500 | 0.7361 | 0.9338 | 0 | 0 |
| combo-athletic | shoulder+0.75 waist−0.5 armShape+0.5 | 1.6659 | 0.7421 | 1.0036 | 0 | 0 |
| combo-pear | hip+0.75 waist−0.25 shoulder−0.5 | 1.6659 | 0.7763 | 1.1416 | 0 | 0 |
| combo-tall-heavy | uniform 1.7549 + waist+0.75 hip+0.75 | 1.7549 | 0.9223 | 1.2026 | 0 | 0 |

## 8. 确定性证明（`data/determinism.md`）

- α=0 逐位恢复基础网格：max diff **0.000e+00**。
- 参数应用顺序无关：max diff 2.776e-17（浮点加法序，实质逐位）。
- 重复生成无漂移：max diff 0.000e+00。
- Stable Vertex ID：full 19,158 / body 13,380 全程不变（仅位置变）。
- helper/joint 随形态：shoulder+0.6/waist−0.4 下 helper 平均位移 2.9 mm，upperarm01.L 关节位移 14.9 mm（非零，同步）。

## 9. 质量检查（PAE 自己的检查器，只读复用）

`analyzeTopology` + `canonical::detectSelfIntersections`（直接编入 host 工具，native/ 零修改）+ `compareSurfaces`。每个单参数极值与每个变体：顶点/面/边数、连通分量、边界、非流形、退化、重复顶点、法线有效性、自交对数、翻面、边长分布（min/p50/p99/max）、体积、bbox——全部在 `variants-quality.csv` 与曲线 CSV。**±1 内全部 0 翻面 / 0 自交 / 0 非流形。**

## 10. 视觉证据

150 张：10 变体 × 5 视角（front/back/side-left/side-right/front-45-left）× 3 模式（clay/wireframe/normal），共享相机帧可直接对比。PAE Host ClayRenderer + CameraProtocol 真实运行生成。图像描述模型确认各变体完整且特征区分明显（**仅供参考，负责人人眼审查为准**）。

## 11. 运行时间与内存

总运行 15.6 s（含 150 张渲染与全部质量检查）。峰值内存：Cygwin 下 psapi 与 /proc 采样均不可用，**NOT MEASURED**（如实记录）。

## 12. 未验证 / 失败 / 有效范围

| 项 | 状态 |
|---|---|
| α 超出 ±1 | 未测（标记实验，不作为批准范围） |
| MakeHuman authored slider 范围 / macro 组合（性别/年龄/肌肉/体重） | NOT RUN |
| macro height 单独用于 statureM | **NOT VERIFIED（明确否定）** |
| ANSUR 验收口径对齐 | NOT RUN（WP3-Production 前置） |
| 变形后自交（±1 内） | 已测 = 0；范围外未测 |
| 骨性肩宽（解剖肩峰） | 无肩峰 landmark，用关节中心代理（已注明） |
| Android 集成 / CharacterDefinition 生产 schema / PAHT v3 | 未改（授权禁止） |
| 峰值内存 | NOT MEASURED |

## 13. 后续生产参数接口建议（WP3-Production，需另行授权）

1. 身高用 PAE 均匀缩放（host 已验证），不接 macro height。
2. 参数→α 用实测非对称响应曲线（本报告 CSV），组合目标需迭代求解（初步研究已证组合非加性）。
3. 围度语义参数必须用截面周长口径（本报告协议），不能用宽度代替。
4. 联动矩阵纳入参数响应表与 UI 提示（胸廓宽会显著移手臂）。
5. 模板补 joint→顶点分组以支持 Android rig 随形态（WP4 前置）。
6. 测量口径对齐 ANSUR/CharacterDefinition 后再谈验收数值。