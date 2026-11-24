# 03 — Canonical Acceptance Sheet v0（候选）

> 状态：**`CANDIDATE` / v0 候选 —— 未 APPROVED。**
> 负责人已批准"参考体系采用方案 C（外部视觉参考 + 解剖工程参考 + 自有 Canonical Acceptance Sheet）"，
> 但**未批准任何具体比例数值**。本文件只提供**带来源的候选数值与协议**，
> 所有数值须经负责人审核后方可成为 Canonical Target。
>
> **AI 不得宣称本项目生成的模型"符合真实人体"。** 本 Sheet 只定义"目标是什么"，
> "生成的模型是否符合"是 PoC-SM 的 Before/After 验收项，当前 `NOT RUN`。

---

## 0. Sheet 身份

| 字段 | 值 |
|---|---|
| Sheet ID | `CAS-ADULT-MALE-v0` |
| 版本 | v0（CANDIDATE） |
| 角色 | **成人男性** |
| 体型原型 | **待负责人确认**（见 §2，三个候选） |
| 视角轴 | 7 个固定机位（见 §4） |
| 姿态轴 | bind / A-Pose / T-Pose / 动作姿态（见 §5） |
| 图像形式 | **Neutral Gray Clay**（唯一可用于几何合格判定） |
| 数值来源 | ANSUR II / NC State 一手汇总表 + CDC NHANES SR50（一手），原件已留存 |
| 状态 | `CANDIDATE` → 待负责人签字 → `APPROVED` |

---

## 1. 数值来源与授权

| 来源 | 编号 | 一手/二手 | 授权 | 留存 |
|---|---|---|---|---|
| **Anthropometric Data for U.S. Adults**（36 项，英寸），© 11/2024 The Ergonomics Center。引用：Gordon, Claire C. et al. (2014). *2012 Anthropometric Survey of U.S. Army Personnel: Methods and Summary Statistics.* | `SRC-ANTH-01` | **一手** | NC State Ergonomics Center 编制；未标注开放许可 → **内部研究使用，不对外再发布** | `source/ANSUR-II-Anthropometry-Summary-Tables.pdf`，SHA256 `a1ea470e6d67d5870a1b35da9fdf4a7d60f1982ee0b187827b1b3a7a3f820ee6` |
| **Anthropometric Reference Data for Children and Adults: United States, August 2021–August 2023**（Statistical Report No. 50） | `SRC-ANTH-02` | **一手** | U.S. National Center for Health Statistics；政府公开出版物 | `source/CDC-NHANES-SR50-Anthropometric-Reference-Data.pdf`，SHA256 `ccb8d52b6e3f09bc50d5734493e512c348c85ad2ea98ea3cd559019da8f13f16` |
| OpenStax *Anatomy and Physiology 2e*（解剖 landmark 定义） | `SRC-ANAT-01` | 一手（教科书） | **CC BY 4.0** | 引用要点，署名 |
| Penn State OPEN Design Lab — ANSUR II 页面（样本代表性声明） | `SRC-ANTH-03` | 一手 | 公开页面 | 引用要点 |

### 1.1 两份来源的**样本代表性限定**（必须随数值一同引用）

`[VERIFIED — SRC-ANTH-03 原文]`：

> "Despite the presence of reservists in the sample, it is still not an approximation of the US Civilian population. Consequently, while there is useful information here, designs and standards based on these data will not accommodate most user populations in the intended manner."

`[VERIFIED]` 即：**ANSUR II 是美军现役/预备役人员样本（男 4,082 / 女 1,986，2012 年，93 项测量）**，**不代表美国民用人口**。

`[RECOMMENDATION]` 因此本 Sheet 的数值**按来源分列**，不混合成单一"人体真值"：
- **形态尺寸（breadth / depth / height / length）** 取 ANSUR II；
- **围度（circumference）** 取 CDC NHANES 民用样本；
- 两者**不可互相推导**，因为人群不同。

---

## 2. 体型原型：三个候选（待负责人选一）

| 候选 | 依据 | 身高 | 腰围（中位） | 臀围（中位） | 特点 | 风险 |
|---|---|---|---|---|---|---|
| **A — ANSUR II 50 百分位** | `SRC-ANTH-01` 男性 50th %ile | 1754.9 mm | 无围度数据 | 无围度数据 | 形态尺寸最完整（36 项）；但**军人样本** | 与民用体型观感有差距；腰/胸围度需外部补 |
| **B — NHANES 民用 20+ 中位** | `SRC-ANTH-02` 男性 20+ 50th %ile | 需与 SRC-ANTH-01 混用 | **1009 mm** | **1036 mm** | 民用样本、围度完整、含测量位置定义 | 20+ 合并组含中老年男性，围度偏大；身高无对应 50th %ile（同报告无身高表） |
| **C — NHANES 20–29 中位（推荐）** | `SRC-ANTH-02` 男性 20–29 50th %ile + SRC-ANTH-01 形态 | 1754.9 mm | **934 mm** | **1030 mm** | 年轻成人群；围度合理；与风格化写实角色的常见体脂观感更接近 | 仍是**真人实测**围度，未做"风格化"调整 |

`[RECOMMENDATION]` **推荐 C**：风格化写实角色通常对应年轻成人的体型观感；20–29 组与 20+ 合并组的腰围中位数相差 **75 mm**（934 vs 1009），这是一个不能靠"取平均"糊过去的差异。

`[OPEN QUESTION]` 选 A / B / C？以及：是否需要在选定原型之上做**风格化调整**（例如按负责人给定的风格夸张度调整）？若需要，调整系数由负责人给出，AI 不自定。

---

## 3. 必测测量项（负责人指定的 8 项）

| # | 测量项 | 状态 | 值（原型 C） | 来源 | 测量定义 | 测量位置 | 单位 | 计算方式 | 置信 |
|---|---|---|---|---|---|---|---|---|---|
| M-1 | **身高 stature** | ✅ 有值 | **1754.9 mm** | SRC-ANTH-01 #1 | 站立头顶至地面垂直距离 | 地面（壁装身高计） | mm | 69.09 in × 25.4 | `VERIFIED` |
| M-2 | **肩宽（体表）** shoulder breadth (bideltoid) | ✅ 有值 | **509.0 mm** | SRC-ANTH-01 #17 | 双侧三角肌外侧最宽处的水平直线距离 | 三角肌最宽截面（双臂自然下垂） | mm | 20.04 in × 25.4 | `VERIFIED` |
| M-2b | 肩宽（骨架） biacromial | ✅ 有值 | **415.0 mm** | SRC-ANTH-01 #18 | 双侧肩峰（acromion）外缘水平直线距离 | 肩峰点 | mm | 16.34 in × 25.4 | `VERIFIED` |
| M-3 | **胸廓宽度** chest breadth | ❌ **TBD** | — | 缺口 | 胸廓最宽处左右半宽 | 胸廓最大横截面 | mm | — | `TBD` |
| M-4 | **胸廓深度** chest (bust) depth | ✅ 有值 | **253.0 mm** | SRC-ANTH-01 #20 | 胸廓前后径（矢状面） | 胸部最大前后距离 | mm | 9.96 in × 25.4 | `VERIFIED` |
| M-5 | **腰围** waist circumference | ✅ 有值 | **934 mm**（20–29 中位）/ 1009 mm（20+ 中位） | SRC-ANTH-02 Table 12 | 腰围 | **髋嵴（ilium）上缘最外侧缘**，软尺水平环绕 | mm | 报告原值（cm→mm） | `VERIFIED` |
| M-5b | **腰宽** waist breadth | ❌ **TBD** | — | 缺口 | 腰部最窄处左右横向宽度 | 腰部最小水平截面 | mm | — | `TBD` |
| M-6 | **骨盆宽度** hip breadth | ✅ 有值 | **343.9 mm** | SRC-ANTH-01 #19 | 髋部最宽处水平距离（双侧大转子水平） | 髋部最宽截面 | mm | 13.54 in × 25.4 | `VERIFIED` |
| M-7 | **盆深** pelvic depth | ⚠️ **代理值** | **251.0 mm**（abdominal depth） | SRC-ANTH-01 #21 | 腹部前后径（矢状面） | 腹部最大前后距离 | mm | 9.88 in × 25.4 | `PARTIALLY VERIFIED` —— 名称为"腹深"非"盆深"，**属代理**；严格盆深 `TBD` |
| M-8 | **主要解剖 landmarks** | ✅ 见 §3.2 | — | SRC-ANAT-01 | 见 §3.2 | 见 §3.2 | — | — | `VERIFIED`（定义）/ 位置 `TBD`（需负责人确认在本项目坐标系中的落点） |

### 3.2 解剖 landmark 定义表

`[VERIFIED — 定义来自 SRC-ANAT-01（OpenStax, CC BY 4.0）]`；**"本项目落点"一列一律 `TBD`**，需负责人在 PoC-SM 开始前给出。

| Landmark | 解剖定义（来源原文要点） | 层级 | 本项目落点 |
|---|---|---|---|
| Jugular notch（胸骨上切迹） | 位于胸骨柄顶端的可触凹陷，颈根 | T1 | `TBD` |
| Sternal angle（胸骨角） | 第二肋与胸骨相接处；第一肋被锁骨遮蔽，**第二肋是可触到的最高肋** | T1 | `TBD` |
| Xiphoid process（剑突） | 胸骨下端 | T1 | `TBD` |
| Costal margin（肋弓） | 下位肋骨的下缘 | T1 | `TBD` |
| Clavicle（锁骨） | 肩前部的 S 形骨，全程可触 | T1 | `TBD` |
| Acromion（肩峰） | 肩胛骨外上角；ANSUR II 的 "shoulder height" 即量至此 | T1 | `TBD` |
| Iliac crest（髂嵴） | 髋骨弓形上缘，沿腰线可触 | T1 | `TBD` |
| ASIS（髂前上棘） | 髂嵴前端 | T1 | `TBD` |
| Umbilicus（脐） | 约位于第三、四腰椎间盘高度 | T1 | `TBD` |
| Linea alba（腹白线） | 自剑突至耻骨联合 | T1 | `TBD` |

`[RECOMMENDATION]` 本项目的 `AnatomicalLandmarks` 契约应直接采用这些**国际标准命名**（FMA 概念名），以便与 BodyParts3D（R-B02，CC BY 4.0，FMA concept id）对接。SPEC §7.1 已批准的 ADR-011 五层契约中包含 `AnatomicalLandmarks` 层，此举不新增契约，只填内容。

### 3.3 派生比例（由 §3 一手值计算，**是参考值不是目标值**）

`[VERIFIED]` 计算方式为 §3 数值之比，无任何中间假设。

| 比例 | 计算 | 值 | 来源 |
|---|---|---|---|
| 肩宽（体表）/ 身高 | 509.0 / 1754.9 | **0.290** | SRC-ANTH-01 |
| 肩宽（骨架）/ 身高 | 415.0 / 1754.9 | **0.237** | SRC-ANTH-01 |
| 髋宽 / 身高 | 343.9 / 1754.9 | **0.196** | SRC-ANTH-01 |
| 胸深 / 身高 | 253.0 / 1754.9 | **0.144** | SRC-ANTH-01 |
| 腹深 / 身高 | 251.0 / 1754.9 | **0.143** | SRC-ANTH-01 |
| **肩髋比（体表肩宽/髋宽）** | 509.0 / 343.9 | **1.480** | SRC-ANTH-01 |
| 肩髋比（骨架） | 415.0 / 343.9 | **1.207** | SRC-ANTH-01 |
| 头身比（ANSUR II 口径） | 1754.9 / 199.9 | **8.78 头身** | SRC-ANTH-01 #1/#26 |
| 腰高比（原型 C） | 934 / 1754.9 | **0.532** | SRC-ANTH-02 / SRC-ANTH-01 |
| 臀围 / 腰围（原型 C） | 1030 / 934 | **1.103** | SRC-ANTH-02 |
| 坐高 / 身高 | 918.0 / 1754.9 | **0.523** | SRC-ANTH-01 #8 |
| 臂展 / 身高 | 1812.0 / 1754.9 | **1.033** | SRC-ANTH-01 #32 |

`[VERIFIED — 交叉校验]` 前 5 项与 SRC-ANTH-01 之外的独立二次汇编（easyanatomylab 转述 NC State 表）**逐项一致**：肩高 82.0%、肘高 61.7%、髋高 51.2%、指高 37.3%、体表肩宽 29.0%、骨架肩宽 23.7%、髋宽 19.6%。两路独立读数一致 → 数值读出无误。

### 3.4 头身比的**口径陷阱**（必须记录）

`[VERIFIED]` 两个"头身比"数值不可直接比较：

| 口径 | 值 | 来源 | 头部测量定义 |
|---|---|---|---|
| ANSUR II | **8.78 头身** | SRC-ANTH-01 #1 / #26 | head length（头部前后长）7.87 in |
| Mather 2010 | **约 7.7 头身**（德国 + 北约男兵，n=4,789） | Perception 2010;39:1390-1395 | head height（头部垂直高） |

`[INFERENCE]` 差异来自**头部测量定义不同**（length vs height），不是数据冲突。`[RECOMMENDATION]` Canonical Sheet 必须固定一种头部定义并在全项目统一；本 Sheet v0 采用 ANSUR II 的 **head length** 口径。

---

## 4. 相机轴（Camera Axis）—— 与姿态轴分开记录

`[VERIFIED]` 沿用 G2 已冻结的 `CameraProtocol::fixedViews()`，**不新造机位**，保证 PoC-SM 的 Before/After 可与 G2 证据同框对比。

| # | View 名 | 方位角 | 仰角 | 主要观察 |
|---|---|---:|---:|---|
| C1 | `front` | 0° | 0° | 胸廓宽、左右对称、中线 |
| C2 | `front-45-left` | 45° | 0° | 胸廓前后不对称、腋窝前缘 |
| C3 | `side-left` | 90° | 0° | 胸深、腹深、盆深、腰背过渡 |
| C4 | `side-left-30up` | 90° | 30° | 肩带顶面、腋窝凹面 |
| C5 | `back` | 180° | 0° | 背阔/斜方、肩胛、对称 |
| C6 | `back-45-right` | 225° | 0° | 后胸廓、腋窝后缘 |
| C7 | `front-30up` | 0° | 30° | 锁骨/肩带、上胸、肩根连接 |

### 4.1 相机轴统一参数（G2 已冻结，直接继承）

| 项 | 值 |
|---|---|
| 投影 | 正交（orthographic） |
| 视图帧 | `viewFrame()` —— 与数值轮廓协议**同一个** |
| 光照 | 视空间固定 (0.55, 0.78, −0.45)，全程不变 |
| 着色 | Lambert：0.28 环境光 + 0.72 × max(0, N·L) |
| 背景 | RGB (28, 28, 31) |
| 分辨率 | 480 × 480 |
| 取景 | 同一视图内**所有路线/所有姿态**共享同一 frame + 8% 边距 |
| 朝向 | +Z 向前，+X 角色左（SPEC §8.1） |
| 裁剪带 | 几何验收用躯干带；姿态验收用 `band = none` 全身 |

`[OPEN QUESTION]` **相机高度绑定方式**：解剖轴（hips→chest 中点）vs 固定世界 Y —— 待负责人裁定（见本轮 10 的开放项）。

---

## 5. 姿态轴（Pose Axis）—— 与相机轴分开记录

`[FACT/VERIFIED]` **T-Pose / A-Pose 不是机位**。姿态改变的是骨架与蒙皮状态，必须经过 CPU 蒙皮才能产生；换机位拍同一套静态网格**不会**得到 T-Pose。因此本 Sheet 强制两轴正交记录。

| # | Pose ID | 定义 | 用于 | 状态 |
|---|---|---|---|---|
| P0 | `bind` | 项目定义的 bind pose（建议 = 站立、双臂自然下垂、A-Pose 与 bind 合并时需注明） | 几何验收（G-P1-GEO）、拓扑验收（G-P1-TOPO） | 角度 `TBD` |
| P1 | `A-Pose` | 双臂自然下斜 | 驱动验收（G-P1-ANIM） | **下斜角度 `TBD`** |
| P2 | `T-Pose` | 双臂水平展开 | 驱动验收（G-P1-ANIM） | 定义为水平（`VERIFIED`，口径由负责人确认） |
| P3 | `elbow-flex-90` | 肘屈曲 90° | 关节变形 | 角度 `TBD`（建议 90°） |
| P4 | `shoulder-flex-90` | 肩前屈 90° | 关节变形 | 角度 `TBD`（建议 90°） |
| P5 | `hip-flex-90` | 髋屈曲 90° | 关节变形 | 角度 `TBD` |
| P6 | `knee-flex-90` | 膝屈曲 90° | 关节变形 | 角度 `TBD` |

### 5.1 A-Pose 角度的**可用线索**（不作为目标值）

`[PARTIALLY VERIFIED — T3 层论坛帖（R-B12）]` 帖中从业者给出：

> "The arms only go up by about 10 degrees and then the collar/scapula with the trapezius and related muscles come into play to raise them further."
> "The mean of shoulder rotation up and down is about 45 degrees downward. And the muscle rest state is about 75 degrees downward (gravity assist)."

`[RECOMMENDATION]` 由此，**A-Pose 的手臂下斜角落在 45°–75°（相对水平向下）区间是有依据的**，但该来源是 T3 层论坛帖、且是 2008 年从业者经验值，**不能直接作为目标角度**。请负责人从 45°/60°/75° 中指定一个，或提供参考图反推。

### 5.2 姿态轴的必要实现（协议扩展，非新几何算法）

`[RECOMMENDATION]` 现有 `ClayRenderer` 只接受静态 `MeshData`、且裁剪到躯干带，**出不了全身姿态图**。所需最小扩展：

1. `applyPose(mesh, skeleton, pose) -> posedMesh` —— CPU 侧，复用 `Skinning.cpp` 已有的 `cpuSkinPositions`；
2. 裁剪带参数化为 `band = none | torso`；
3. 姿态定义表（角度）作为外部数据驱动，不硬编码。

`[INFERENCE]` 该扩展**不触碰几何算法、不触碰 ABI**，属于评估协议设施，因此适合纳入 AUTH-08 范围（见本轮 08）。

---

## 6. 视角 × 姿态 证据矩阵

| 用途 | 相机轴 | 姿态轴 | 图像类型 | Gate |
|---|---|---|---|---|
| 比例 / 解剖 / 连续性 | C1–C7 | P0 | Neutral Gray Clay | `G-P1-GEO` |
| 轮廓量测 | C1, C3, C5 | P0 | Silhouette | `G-P1-GEO` |
| 区域语义 | C1, C2, C3, C4 | P0 | Anatomical Region Visualization | `G-P1-GEO` |
| 拓扑 | —（正交/俯视叠加） | P0 | Wireframe / Topology | `G-P1-TOPO` |
| 驱动与变形 | C1, C3, C4, C6 | P0, P1, P2, P3, P4 | Joint Deformation Comparison | `G-P1-ANIM` |
| 材质 / 光照 / 性能 | C1, C2, C3 | P0, P1 | 彩色材质 + 实机 | `G-FINAL-VIS` |

`[RECOMMENDATION]` **几何类判定只在 P0（bind pose）下做**，避免骨骼变形污染几何读数；驱动类判定必须在多姿态下做。

---

## 7. 显式禁止事项

| # | 禁止 | 理由 |
|---|---|---|
| X-1 | **不得**把当前 Baseline 的腰胸比 **0.4821** 与髂嵴宽/盆深比 **1.5977** 作为本 Sheet 的目标值 | 这两个数是 **G2 实测的现有生成模型产出**，不是人体参考值；用它们当目标 = 让实现定义标准 |
| X-2 | **不得**把本 Sheet 的任何数值称为"AI 生成结果符合真实人体" | 本 Sheet 定义目标；符合性是 PoC-SM 的验收项，当前 `NOT RUN` |
| X-3 | **不得**用 ANSUR II 的**围度**替代 CDC 的围度，或反之 | 样本不同（军人 vs 民用），测量位置不同 |
| X-4 | **不得**在未指定测量位置的情况下引用"腰围""胸围" | 不同位置可差 9 cm 以上（见 §8.1） |
| X-5 | **不得**把 BodyParts3D 的网格当外表面验收图或生产资产 | 见本轮 02 的 L-B1 / L-B2 |
| X-6 | **不得**自行决定 A-Pose 角度、目标体型或风格化调整系数 | 属负责人决定项 |
| X-7 | **不得**把本 Sheet 标为 APPROVED | 状态为 `CANDIDATE` |

---

## 8. TBD 登记（提交负责人审核）

### 8.1 TBD-1：胸廓宽度 chest breadth

- **缺失原因**：ANSUR II 的 36 项**不含胸廓宽度**；CDC SR50 报告**仅**为儿童报告 chest circumference，成人 20+ 无胸围值。
- **候选来源**：NHANES *Anthropometric Reference Tables*（与 SR50 同源，含 Chest breadth / Biacromial breadth / Bi-iliac breadth / Seat breadth / Sagittal abdominal diameter）—— 本轮**未抓取**，可按此定位。
- **替代方案（属建模假设，非测量值）**：由胸深 + 假定截面积数反推宽度 → **必须标记为 ASSUMPTION**，不得当作实测。

### 8.2 TBD-2：腰宽 waist breadth

- **缺失原因**：ANSUR II 无腰部横向宽度；CDC SR50 有腰围与矢状腹径但无腰宽。
- **替代方案（属建模假设）**：由腰围（1009 mm / 934 mm）与矢状腹径按椭圆近似反推横向宽度 → **必须标记为 ASSUMPTION**。

### 8.3 TBD-3：腰胸比 waist-to-chest ratio

- **定义**：腰围 ÷ 胸围（两者均须固定测量位置）。
- **现状**：腰围有值（`SRC-ANTH-02`）；**成人胸围无值**（见 TBD-1）→ 比例 `TBD`。
- **注意**：现行 Baseline 产出 0.4821，**不是**该定义下的可比值（其胸/腰口径未记录）→ 属 X-1。

### 8.4 TBD-4：髂嵴宽/盆深比 iliac-crest-width / pelvic-depth ratio

- **定义**：髂嵴宽（bi-iliac breadth）÷ 盆深（骨盆前后径）。
- **现状**：ANSUR II 的 "hip breadth"（343.9 mm）量于**髋部最宽处（双侧大转子水平）**，**不等同于髂嵴宽**；"abdominal depth"（251.0 mm）是**腹深**，是盆深的代理。
- **因此该比例 `TBD`**，需 NHANES 的 Bi-iliac breadth + Sagittal abdominal diameter 或其他严格定义源。
- **注意**：现行 Baseline 产出 1.5977，其口径同样未记录 → 属 X-1。

### 8.5 TBD-5：本体落点

§3.2 全部 10 个 landmark 的**本项目坐标系落点**（位置 + 所属 RegionId）`TBD`。

### 8.6 TBD-6：体型原型与风格化调整

见 §2，负责人选 A / B / C，并决定是否做风格化调整及调整系数。

### 8.7 TBD-7：姿态角度

P0 / P1 / P3 / P4 / P5 的角度值。

---

## 9. 验收用法（Canonical Sheet 如何被使用）

`[RECOMMENDATION]` Canonical Sheet 是**验收目标**，不是建模输入：

1. PoC-SM 实现**不读取**本 Sheet 的数值作为参数——否则会退化成"照抄一个比例表"，失去程序化生成的意义；
2. PoC-SM 产出模型后，按本 Sheet 的**测量定义**重新量测产出模型；
3. 与本 Sheet 数值比对，差异记入 `G-P1-GEO` 的 Before/After；
4. `TBD` 项不参与判定，但必须出现在报告的"未测项"清单里，**不得默认为通过**。

`[INFERENCE]` 第 1 条是本 Sheet 最重要的使用纪律：Canonical Sheet 应当**约束验收而非约束生成**，否则"程序化"就退化为"参数回放"。

---

## 10. 签署

```
Canonical Acceptance Sheet 版本： CAS-ADULT-MALE-v0
状态： CANDIDATE / APPROVED（负责人填写）
体型原型： A（ANSUR II 50th） / B（NHANES 20+ 50th） / C（NHANES 20–29 50th）  ← 圈选
风格化调整系数：____________________（若选 C 且需调整）
批准为 Canonical Target 的测量项： M-1, M-2, M-2b, M-4, M-5, M-6, M-7（代理）, M-8 / 其他：______
明确排除为 TBD 的测量项： M-3, M-5b, M-5/6 派生的比值、landmark 落点、姿态角度
负责人签字：____________________  日期：____________
```

> AI 不得代签。签字栏为空即视为未批准。
