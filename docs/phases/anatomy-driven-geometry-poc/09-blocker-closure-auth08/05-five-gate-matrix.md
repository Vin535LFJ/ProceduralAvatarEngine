# 05 — 统一五 Gate Matrix

> 状态：**草案 / DRAFT**。Gate 定义**全部来自**已存在的 Deliverable 1 / 3 / 4 / 6 与负责人本轮指令，
> **未新增、未删除、未改名**任何 Gate。
> 判定规则来自负责人本轮指令的禁止清单，逐条落为可执行条款。

---

## 0. 第五个 Gate 的正式名称与原始定义（负责人指定查找项）

`[VERIFIED]` 已报告的四个 Gate 是 `G-P1-TOPO`、`G-P1-ANIM`、`G-P1-GEO`、`G-FINAL-VIS`。
**第五个是 `G-P2-MOD`。**

| 项 | 内容 |
|---|---|
| **正式名称** | **`G-P2-MOD` — P2 模块化** |
| **原始定义（负责人指令原文）** | 验收对象：**P2 模块化**；应该验证什么：**服装、发型、配件独立切换**；不应该用什么代替：**单张穿着完整服装的效果图** |
| **首次出现位置** | Deliverable 1 §4.2 五 Gate 表第 4 行（由负责人 2026-11-24 指示确立） |
| **一致性核对** | Deliverable 3 §5.1、Deliverable 4 §2.4、Deliverable 6 §2 **四处定义一致**，措辞无冲突 |

`[VERIFIED]` 五个 Gate 的定义来源一致性核对：

| Gate | D-1 §4.2 | D-3 §5.1 | D-4 §2.4 | D-6 §2 | 一致 |
|---|---|---|---|---|---|
| `G-P1-GEO` | ✔ | ✔ | ✔ | ✔ | ✔ |
| `G-P1-TOPO` | ✔ | ✔ | ✔ | ✔ | ✔ |
| `G-P1-ANIM` | ✔ | ✔ | ✔ | ✔ | ✔ |
| `G-P2-MOD` | ✔ | ✔ | ✔ | ✔ | ✔ |
| `G-FINAL-VIS` | ✔ | ✔ | ✔ | ✔ | ✔ |

---

## 1. 唯一 Gate Matrix

| Gate | 输入 | 评价内容 | 必须证据 | PASS 条件 | 当前状态 | 阻塞关系 |
|---|---|---|---|---|---|---|
| **`G-P1-GEO`**<br>P1 人体几何 | ① Canonical Acceptance Sheet 的测量定义与数值<br>② PoC-SM 产出的躯干表面（bind pose）<br>③ 共享 Target Reference | 解剖比例、曲面连续性、人体体积 | Neutral Gray Clay（7 机位）<br>Silhouette（轮廓量测）<br>Anatomical Region Visualization<br>landmark 位置误差<br>截面径向偏差<br>法线场一致性<br>接缝二面角 | ① 剖面比例与 Canonical Sheet 一致性达标（阈值待 OQ-28 批准）<br>② 接缝最大二面角 **≤5°**（SPEC §13 既有合同值）<br>③ 无独立肩球 / 圆管接缝 / 拼接痕迹<br>④ 肩根/腋窝/胯部无穿透或自交<br>⑤ 三视角无严重形态缺陷 | **FAIL**（R1/R2 接缝 47.66°、R3 16.13°；R3 上胸缺口 13.5 cm）<br>比例/landmark 项 `NOT RUN`（无 Canonical Sheet） | 被 OQ-01 阻塞（无验收目标）；≤5° 被 OQ-03 阻塞（性质未定）；被 OQ-14/16/17 阻塞 |
| **`G-P1-TOPO`**<br>P1 拓扑 | ① 固定四边模板拓扑<br>② 共享稳定顶点 ID 与面邻接<br>③ 参数组差分 | 肩根、腋下、胯部、关节结构 | Wireframe / Topology 视图<br>关节环标注<br>拓扑审计数值（components / boundary / non-manifold / degenerate / sliver / 最小内角 / 正交 quad 比）<br>跨参数组顶点 ID 稳定性 | ① components = 1（已登记 crop 除外）<br>② 未登记 boundary = 0<br>③ non-manifold = 0<br>④ degenerate = 0<br>⑤ 无 sliver（aspect >10 或最小内角 <15°）<br>⑥ 顶点 ID 与面邻接跨参数组恒定<br>⑦ 肩根为**连续解剖连接**（无独立肩球、无简单圆管替代） | **PASS（候选 R2）**<br>sliver 0 / 未登记 boundary 0 / non-manifold 0 / degenerate 0 / components 1 / 正交 quad 比 0.935 / ID 恒定 | 被 OQ-17 阻塞（跨区影响阈值）；被 OQ-09 阻塞（审核人复核 crop/裂缝区分） |
| **`G-P1-ANIM`**<br>P1 动画 | ① 骨骼 + 权重<br>② CPU 蒙皮<br>③ 姿态轴定义 | 关节弯曲、体积保持、变形连续性 | 权重合法性审计<br>CPU 刚体探针<br>Joint Deformation Comparison（bind / A-Pose / T-Pose / 肘屈90° / 肩前屈90° / 髋屈90° / 膝屈90°）<br>局部体积变化率<br>GPU parity 专项报告 | ① 权重 ≤4 影响骨、非负、和 = 1±1e-3、骨 ID ∈ [0,21]<br>② CPU 侧变形正确（刚体顶点零误差）<br>③ 必检动作姿态下无体积塌陷、无自交<br>④ **GPU 数值 parity 已验证** | **PARTIAL**<br>① 权重合法 ✔（三路线）<br>② CPU 刚体探针 22/22 零误差、bind residual ≤2.4e-7 ✔<br>③ 动作姿态测试 `NOT RUN`（姿态协议未定义）<br>④ GPU parity `UNVERIFIED` | 被 OQ-30/32 阻塞（姿态定义）；被 OQ-33 阻塞（协议扩展是否授权）；被 OQ-19 阻塞（GPU parity） |
| **`G-P2-MOD`**<br>P2 模块化 | ① 基础人体网格<br>② 服装 / 发型 / 配件资产<br>③ 切换与遮挡系统 | 服装、发型、配件独立切换 | 同一基础人体的多外观配置对照<br>模块边界与遮挡/穿插检查<br>皮肤外观切换证据 | ① 服装、发型、配件可独立替换且不影响基础人体拓扑<br>② 模块边界无穿插、无破面<br>③ 换装后人体质量不低于 `G-P1-GEO` 已达水平 | **NOT RUN**（未开始） | 属 P2 阶段；被 `G-P1-GEO` 阻塞（"这一层不能降低 P1 的基础人体质量要求"） |
| **`G-FINAL-VIS`**<br>最终视觉 | ① 通过 `G-P1-GEO` 的模型<br>② 材质与光照配置<br>③ 目标 Android 设备 | 整体角色完成度、材质、光照 | 彩色材质渲染（代表姿态 + 代表机位）<br>实机录屏 / FPS / 帧时间<br>生成耗时、峰值内存、三角形数 | ① `G-P1-GEO` 已 PASS（**受理前提**，不满足则不进入评审）<br>② 达到负责人批准的完成度层级（Level 2 核心目标）<br>③ **设备性能独立达标**（不得由几何 PASS 推导）<br>④ 未违反 C++17/NDK/GLES3/无外部资产等工程边界 | **NOT RUN**（无实机数据）<br>设备性能基线未定 | 被 `G-P1-GEO` 阻塞（受理前提）；被 OQ-35 阻塞（材质/光照基线与性能预算） |

---

## 2. 禁止清单 → 可执行条款

`[VERIFIED — 负责人指令原文]` 五条禁止，逐条转成**判定规则**：

| # | 禁止 | 可执行规则 | 违反时的处置 |
|---|---|---|---|
| N-1 | 用拓扑 PASS 抵消 GEO FAIL | `G-P1-TOPO` 与 `G-P1-GEO` **独立判定、独立签署**。`G-P1-TOPO` = PASS **不得**写入 `G-P1-GEO` 的判定依据栏 | 评审记录作废，需重签 |
| N-2 | 用 CPU 变形 PASS 代替 GPU Parity | `G-P1-ANIM` 的 PASS 条件含 **④ GPU 数值 parity 已验证**。CPU 侧通过**只能**记为 PARTIAL | 不得判 `G-P1-ANIM` = PASS |
| N-3 | 用静态 T-Pose 代替动作变形测试 | `G-P1-ANIM` 的必须证据含 **≥4 个动作姿态**的变形对比。单一 T-Pose 静置图**不构成**动作变形证据 | 不得判 `G-P1-ANIM` = PASS |
| N-4 | 用最终渲染效果代替基础几何质量 | `G-FINAL-VIS` 的受理前提是 `G-P1-GEO` 已 PASS。彩色材质渲染图**不得**用于满足 `G-P1-GEO` | `G-FINAL-VIS` 不予受理 |
| N-5 | 用几何 PASS 代替设备性能 PASS | `G-FINAL-VIS` 内含**独立判定**的设备性能项（FPS / 帧时间 / 生成耗时 / 峰值内存 / 三角形数），其判定**不引用**任何几何结论 | 不得判 `G-FINAL-VIS` = PASS |

### 2.1 关于 N-5 与五 Gate 结构的结构性说明

`[INFERENCE]` 五个 Gate 中**没有单独的"性能 Gate"**——设备性能被安排在 `G-FINAL-VIS` 之下（对应 rubric 的 D8 维度）。但负责人同时要求"不得用几何 PASS 代替设备性能 PASS"。

`[RECOMMENDATION]` 为在**不新增第六个 Gate** 的前提下满足该要求，采取以下做法：

1. `G-FINAL-VIS` 的 PASS 条件拆为**两个独立判定项**：**(a) 完成度层级**（主观，负责人判）与 **(b) 设备性能**（客观，实机数据）；
2. 两项**分别签署、分别记录**，**(b) 不通过则整体不判 PASS**，且 (b) 的判定记录中**不得引用**任何几何指标；
3. 二者在 rubric 的签署块中分列两行（见 Deliverable 4 §8）。

`[VERIFIED]` 这不违反"禁止自行凭空添加或删除 Gate"——Gate 数量仍为 5，只是把 `G-FINAL-VIS` 内部的性能项显式独立出来。

### 2.2 跨 Gate 通用规则

| # | 规则 | 来源 |
|---|---|---|
| U-1 | 每个 Gate 独立判定，**不合成总分、不加权平均** | 负责人指令；rubric H-6 |
| U-2 | 静态外观（Layer A）与拓扑/驱动（Layer B）分开评价、分开签署 | rubric H-7 |
| U-3 | 几何合格判定**只认 Neutral Gray Clay**；彩色材质渲染不得充当几何证据 | rubric H-8；N-4 |
| U-4 | 同一轮评估内，所有路线 / 参数组 / 姿态使用同一相机、同一光照、同一背景、同一材质、同一分辨率、同一裁剪范围 | rubric H-9 |
| U-5 | **TBD 指标不得判 PASS**，也不得默认通过；必须出现在"未测项"清单 | SPEC §13；任务书证据纪律 |
| U-6 | AI 不宣布任何 Gate 为 PASS；`G-FINAL-VIS` 的受理与结论均由负责人行使 | 负责人指令 |

---

## 3. Gate 当前状态汇总（全部基于 G2 实测值，非阈值）

`[VERIFIED]` 状态依据一律来自 `evidence/g2/fair-comparison.md` 与 `evidence/reports/g2-fair-comparison-report.md` 的实测值。

| Gate | 路线 | 状态 | 关键实测 |
|---|---|---|---|
| `G-P1-GEO` | R1 | **FAIL** | 接缝 47.66°（>5°） |
| `G-P1-GEO` | R2 | **FAIL** | 接缝 47.66°（>5°）；landmark 与截面为"自比 0"，**非合格证据** |
| `G-P1-GEO` | R3 | **FAIL** | 接缝 16.13°（>5°）；上胸缺口 13.5 cm；landmark mean 54.4 mm / max 126.8 mm；截面 \|dev\| mean 26.8 mm |
| `G-P1-TOPO` | R1 | **FAIL** | 顶点 ID 跨参数组**变化**（5108→5234→5314）；sliver 30；正交 quad 比 0.637 |
| `G-P1-TOPO` | R2 | **PASS** | sliver 0；未登记 boundary 0；non-manifold 0；degenerate 0；components 1；正交 quad 比 0.935；ID 恒定 |
| `G-P1-TOPO` | R3 | **PARTIAL** | sliver 20；正交 quad 比 1.000；ID 恒定；但形态不合格使其失去意义 |
| `G-P1-ANIM` | 全部 | **PARTIAL** | 权重合法 ✔；CPU 刚体 22/22 ✔；GPU parity `UNVERIFIED`；动作姿态 `NOT RUN` |
| `G-P2-MOD` | — | **NOT RUN** | 未开始 |
| `G-FINAL-VIS` | — | **NOT RUN** | 无实机数据 |

`[INFERENCE]` **这张表是"为什么下一步必须先修 Shape Model"的最简证明**：
R2 唯一已经过关的是 `G-P1-TOPO`；`G-P1-ANIM` 的 CPU 侧已成立、差 GPU parity 与姿态协议；
而 `G-P1-GEO` **三者皆 FAIL**。把 `G-P1-GEO` 和 `G-P1-TOPO` 合成一个 Gate 会连带否掉 R2 已经合格的拓扑能力；
把它们分开，则结论清晰：**R2 作为固定拓扑后端保留，形态必须由新的 Shape Model 解决**。

---

## 4. Gate 与阶段、OQ 的映射

| Gate | 所属阶段 | 依赖的 OQ |
|---|---|---|
| `G-P1-GEO` | PoC-SM | OQ-01（含 OQ-28/29/30/31/32）、OQ-03、OQ-14、OQ-16、OQ-17 |
| `G-P1-TOPO` | PoC-SM | OQ-09（审核）、OQ-17 |
| `G-P1-ANIM` | PoC-SM | OQ-30、OQ-32、OQ-33、OQ-19 |
| `G-P2-MOD` | PoC-MOD（P2） | 依赖 `G-P1-GEO` 先通过 |
| `G-FINAL-VIS` | 贯穿 | `G-P1-GEO` 通过 + OQ-35 |

`[RECOMMENDATION]` Gate 依赖链是一条**单向链**：`G-P1-GEO` → `G-P1-TOPO` / `G-P1-ANIM` → `G-FINAL-VIS` → `G-P2-MOD`。
`G-P1-TOPO` 与 `G-P1-ANIM` 相对独立，可并行；`G-FINAL-VIS` 与 `G-P2-MOD` 串行在后。

---

## 5. 本文件的边界

`[VERIFIED]` 本文件：

- **未**新增、删除或改名任何 Gate；
- **未**设定任何新的数值阈值——出现的阈值（≤5°、components=1、boundary=0、non-manifold=0、degenerate=0、权重合法性）**全部是 SPEC 既有合同值**；
- **未**把任何 Gate 判为 PASS——所有状态均为实测值推导，且 `G-FINAL-VIS` / `G-P2-MOD` 保持 `NOT RUN`；
- **未**替代负责人行使 `G-FINAL-VIS` 的受理权与结论权。
