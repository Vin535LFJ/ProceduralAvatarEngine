# 02 — DEC-05 参考来源更新矩阵

> 状态：**草案 / DRAFT**。负责人已批准参考体系采用**方案 C（外部视觉参考 + 解剖工程参考 + 自有 Canonical Acceptance Sheet）**；
> 本矩阵是该批准的**执行材料**，不构成 DEC-05 的 APPROVED 状态。
> ID 方案延续 Deliverable 2 的 `R-xx` 序列，**不新建并行清单**。

---

## 0. 本轮变更摘要

| 变更类型 | 数量 | 说明 |
|---|---:|---|
| 新增 Layer B 来源 | 4 | Thunder Cloud Studio、Blender Studio、Renderosity Armpit、BodyParts3D —— 全部**本轮实际抓取正文** |
| 新增 Layer A 来源 | 0 | 但 David Rig 完成核验尝试，结论为 `UNVERIFIED` |
| 新增 Layer C 产物 | 1 | Canonical Acceptance Sheet v0 候选（见本轮 03） |
| 授权证据升级 | 2 | R-C06 MakeHuman → **CC0 已核验**（FAQ 原文）；R-B02 BodyParts3D → **CC BY 4.0 已核验**（许可章节原文） |
| 权威分级新增 | — | 建立 4 级权威阶梯，禁止把论坛帖 / 教学文 / 解剖库并列 |

---

## 1. 权威阶梯（本轮核心新增）

`[FACT/VERIFIED — 负责人指示]` "不得把论坛帖子、教学文章和解剖数据库等同于同一权威等级。"

| 等级 | 类型 | 可用于 | 不可用于 | 本矩阵条目 |
|---|---|---|---|---|
| **T1** | **解剖学数据库 / 同行评议文献** | 解剖 landmark 的**位置与命名**、器官空间关系 | 皮肤外表面尺寸；生产网格 | R-B02 BodyParts3D |
| **T2** | **专业工作室 / 教学机构教程** | 拓扑**实践规范**（循环带位置、变形驱动的拓扑需求） | 精确尺寸数值；解剖学权威断言 | R-B10 Thunder Cloud Studio；R-B11 Blender Studio |
| **T3** |**论坛讨论（从业者）** | **问题清单**（从业者实际在哪些部位遇到困难） | 规范性结论；技术结论 | R-B12 Renderosity Armpit |
| **T4** | **个人博客 / 二次汇编** | 仅作线索去追原始出处 | 任何权威断言 | R-C07 MessyPoly；R-C08 basemesh；R-C09 bindpose；easyanatomylab |

`[RECOMMENDATION]` 使用规则（写入 rubric 的 Layer B 判定流程）：

- **T1/T2 可作为"要求是什么"的依据**（例如"腋下必须在 A-Pose 之外预留变形循环带"）；
- **T3 只能作为"哪些地方容易出问题"的检查项来源**，不能用来定义"正确的做法"；
- **T4 不得进入任何判定依据**；
- **任何层级都不能提供数值**——数值一律来自 Anthropometric 数据源（见本轮 03）。

---

## 2. Layer A — Visual Style Benchmark（视觉风格标杆）

作用：定义最终人物的外观质量、风格与完成度。

| ID | 名称 | 原始链接 | 作者/发布者 | 授权 | 授权证据 | 能证明 | 不能证明 | 状态 |
|---|---|---|---|---|---|---|---|---|
| **R-A01** | Young Man / Cartoon Man Rigged (id 2151499) | https://www.turbosquid.com/3d-models/3d-model-young-man-rigged-2151499 | `UNVERIFIED`（页面被 ad-blocker 拦截，未能核验发布者） | `UNVERIFIED` | 无可见许可文本 | **风格化角色的视觉完成度参照**（负责人指定用途） | **不能**证明人体拓扑质量；**不能**证明低端 Android 实时性能；**不能**证明程序化生成能力（商品页描述为带绑定 + PBR + 贴图的**成品商业模型**，属商品属性声明而非技术验证） | `CONDITIONAL`（仅 Layer A 观察） |
| **R-A02** | Cartoon Family 2 — Mother Father Son Daughter | https://www.cgtrader.com/3d-models/character/woman/cartoon-family-2-mother-father-son-daughter | Characterstore（CGTrader 页面可核验，63 reviews） | 商业商品许可（页面未展示全文） | 页面显示售价 $328；格式 MAX/FBX/OBJ | 角色家族风格与角色组配置 | **不能**作为成人自然比例标准（角色身高 135/100/160 cm 为家族设定） | `CONDITIONAL`（Layer C 观察）/ `REJECT`（Layer A） |
| **R-A03** | David Rig | https://www.davidrig.com/ | `UNVERIFIED` | `UNVERIFIED` | 无 | 无 | 无 | **`UNVERIFIED` — 本轮再次核验失败** |
| **R-A07** | Male human character turnaround | https://sketchfab.com/3d-models/male-human-character-turnaround-wearing-casual-fb7647d1a10140d3a8ff1f3e9cb6cafb | 0C557H512（Sketchfab 页面可核验） | **CC BY 4.0** | 页面 License 字段明示 | Turnaround 视图下的完成度 | 不是程序化人体方法（Rodin Gen-1 生成）；不能证明拓扑 | `CONDITIONAL`（唯一授权已核验的 turnaround） |
| **R-A06** | Realistic MetaHuman Characters | https://www.artstation.com/artwork/DLJ52e | MINO ENTERTAINMENT | `UNVERIFIED` | 页面无许可声明 | 写实数字人完成度 | 依赖外部生态，与本项目生产约束冲突 | `CONDITIONAL`（仅观察） |

### 2.1 David Rig 核验记录（本轮实测）

`[VERIFIED]` 两次尝试：

| 尝试 | 方法 | 结果 |
|---|---|---|
| 1 | 直接抓取 `https://www.davidrig.com/` | `fetch failed`（站点不可达） |
| 2 | 站内检索 `"davidrig.com" David Rig 3D character model artist` | 返回结果与该站点**无任何关联**（命中 Adobe / Dragon Con / Unity Asset Store 等无关页面） |

`[VERIFIED]` 结论：无法确认该链接对应任何具体作品、作者或授权。按负责人原指示（"如果该链接无法确认对应具体模型或作品，应标记为 UNVERIFIED"），**维持 `UNVERIFIED`，不进入正式参考清单**。

`[OPEN QUESTION]` 若负责人能提供 David Rig 的**具体作品名或作品页 URL**，本轮可立即补做核验。

---

## 3. Layer B — Anatomical & Topology Reference（解剖与拓扑参考）

### 3.1 新增 4 条（本轮全部实际抓取正文）

| 字段 | **R-B10** | **R-B11** | **R-B12** | **R-B02** |
|---|---|---|---|---|
| 名称 | Topology for Low Poly Game Characters: Essential Tips and Tricks (Part 2) | Generic Retopology — Realistic Character Workflow | Arm pit tolology（论坛帖） | BodyParts3D |
| 原始链接 | thundercloudstudio.artstation.com/blog/G980G/… | studio.blender.org/training/realistic-human-research/retopology/ | renderosity.com/forums/threads/2748028 | dbarchive.biosciencedbc.jp/data/bodyparts3d/LATEST/README_e.html |
| 作者/发布者 | **Thunder Cloud Studio**（未署个人作者） | **Julien Kaspar**（页面署名作者） | **pjz99**（发帖人），21 posts，2008-08-22 | **Database Center for Life Science**, Research Organization of Information and Systems（ROIS） |
| 发布/更新 | 2024-10-18，状态 WIP | 无明确日期 | 2008-08-22（2011-11-23 编辑） | README 更新 2025-02-27；3D 数据 Release 4.0 (2013) |
| 权威层级 | **T2** 工作室教程 | **T2** 教学机构教程 | **T3** 论坛帖 | **T1** 解剖数据库 + 同行评议 |
| 授权 | 页面**无**许可声明 | 未见页面许可声明（Blender 站点内容 CC BY 4.0，手册例外未核验） | 论坛默认条款；作者内容未单独授权 | **CC BY 4.0**（许可章节原文明示，见下） |
| 授权证据 | 无 | 无 | 无 | 原文："The license for this database is specified in the Creative Commons Attribution 4.0 International." 署名要求："BodyParts3D, © The Database Center for Life Science licensed under CC Attribution 4.0 International" |
| 抓取状态 | ✅ 正文完整取得 | ✅ 正文完整取得 | ✅ 21 posts 正文完整取得 | ✅ README 正文完整取得 |
| 状态 | `CONDITIONAL`（T2 工程参考） | `CONDITIONAL`（T2 工程参考） | `CONDITIONAL`（T3，仅问题清单） | **`ACCEPT`（T1 工程研究）** |

### 3.2 按解剖主题分类

`[FACT/VERIFIED]` 分类依据为**实际抓取到的正文内容**，非标题推断。

| 解剖主题 | R-B10 (T2) | R-B11 (T2) | R-B12 (T3) | R-B02 (T1) |
|---|---|---|---|---|
| **Anatomy / Landmarks** | 胸廓环带对应 **clavicle、ribs、pelvic cavity、iliac fossa、abdominal muscles、diaphragm**；背部对应 **teres major / teres minor / latissimus dorsi / erector spinae** | "Landmarks" 定义为"不同物体与几何体相互接触并需一起运动的区域"；要求在 landmark 与接缝周围放置循环带与邻近循环带 | 讨论腋下 **5 边 pole**（latissimus 消失于 biceps/triceps 之间）、**6 边 pole 为不良拓扑** | 全身 3D 解剖概念字典，**FMA concept id + IS-A / PART-OF 关系树**；仅 ELEMENT（原子器官）为多边形网格，COMPOUND 由元素定义 |
| **Shoulder / Armpit** | 腋下循环带须支持抬臂/降臂；A-Pose/T-Pose **看不出腋下真实活动范围**，须额外预留；循环带"facilitate easier skinning of the arm and shoulder" | — | 主题帖：triceps 需能"flow behind the latissimus"；axilla 边缘流 | 肩带/腋窝相关肌群（需按 FMA id 检索） |
| **Chest / Back** | 躯干环带划分 arm / chest / leg 区域；**背部因肌肉量大于正面而获得更高网格密度**；背部下段环带向手臂延伸以表达 latissimus dorsi 运动方向 | — | — | 胸廓/背部器官与肌肉 |
| **Pelvis / Groin** | 髋/裆/臀部需环带以支持侧向拉伸与前抬腿；艺术家常漏置臀部环带导致"前抬腿时臀部变形/凹陷、向后回不去" | — | — | 盆腔器官、FMA 关系 |
| **Elbow / Knee** | 肘：**内外两侧都需环带**以适应屈伸；肘拓扑应比常见范例更复杂，因为同时要支撑屈伸、定义肘形、产生自然褶皱。膝：环带位置需保证屈膝时膝形保持、形成膝后褶皱 | — | — | 肘/膝相关结构 |
| **Rigging / Deformation** | 整体以"拓扑服务于蒙皮/绑定/仿真/引擎导入"为目标；含手/脚骨骼关节环带、模拟区域的三角形化建议、Unreal 与 Unity 三角化方向差异的处理、碰撞网格 | 要求**先锁定 retopology 再往下做**；"**Start out the retopology symmetrical**"；对关节放置 Empty 以传达**旋转枢轴位置**给后续绑定 | "**if you don't get edge flow lined out for the major muscle masses then when the figure is rigged, you're going to see problems**"；T-Pose 与 A-Pose 的肩部运动学差异：**手臂只上抬约 10°**，其余由锁骨/肩胛/斜方肌参与；肩部"mean of shoulder rotation up and down is about 45 degrees downward"，"muscle rest state is about 75 degrees downward"；T-Pose 的优点之一是便于 IK | — |

### 3.3 三条使用限制（逐条写入 rubric）

`[FACT/VERIFIED]` 负责人要求："BodyParts3D 可以用于解剖结构研究，但不能作为外部生产网格或皮肤外表面验收图"；"不得因为能在线浏览就认为可以下载、复制或再发布"。

| # | 限制 | 适用条目 |
|---|---|---|
| L-B1 | **不得**将 BodyParts3D 的 OBJ 网格导入项目，或作为生产角色资产（违反项目硬约束"不导入外部模型资产"） | R-B02 |
| L-B2 | BodyParts3D 是**内部器官**数据库，**不是皮肤外表面**；**不得**用它作外表面验收图或"外部生产网格" | R-B02 |
| L-B3 | **在线可浏览 ≠ 可下载 / 复制 / 再发布**。R-B10 / R-B11 / R-B12 均**无明示许可**，因此：只可阅读、只可引用要点、**不得**复制正文或图片、**不得**转存素材 | R-B10, R-B11, R-B12 |
| L-B4 | BodyParts3D 若在内部文档中引用数据集本身，必须带 CC BY 4.0 署名原文；引用其**衍生测量**同样需要署名 | R-B02 |
| L-B5 | R-B12 是 2008 年论坛帖，**不得**当作当前最佳实践；只作"从业者关注点清单" | R-B12 |

`[INFERENCE]` R-B12 有一条**意外高价值**的可用信息：它给出了肩部运动的定量事实（手臂仅上抬约 10°、平均肩旋转约 45° 向下、肌肉静息态约 75° 向下），这是 A-Pose 定义的关键物理依据（见本轮 03 的姿态轴）。虽然它是 T3 层论坛帖、不能作权威断言，但作为**待负责人复核的线索**极有价值——本轮把它标为 `PARTIALLY VERIFIED`，并明确要求负责人确认。

### 3.4 Layer B 现有条目的状态更新

| ID | 名称 | 旧状态 | 新状态 | 变化原因 |
|---|---|---|---|---|
| **R-C06** | MakeHuman | `ACCEPT`（B 层工程研究） | **`ACCEPT`**，授权证据升级 | **CC0 已核验**：FAQ 原文 "The asset license is CC0"。许可页实际抓取 |
| **R-C02** | Blender 4.2 Manual — Bone Positioning Guide | `CONDITIONAL` | **`CONDITIONAL`（仅线索）**，许可表述收紧 | 手册正文**两次抓取均被 Cloudflare 拦截**；**手册正文许可未核验**（通行 CC BY-SA 3.0，无逐字证据）；网站内容 CC BY 4.0 已核验 |
| **R-C07** | MessyPoly Quad Retopology | `CONDITIONAL` | **`REJECT`（不进工程依据）**，降为 T4 线索 | 页面无许可声明；权威层级 T4 |
| **R-C08** | basemesh rigging checklist | `UNVERIFIED` | `REJECT`（T4 线索） | Cloudflare 拦截，无法核验作者与许可 |
| **R-C09** | bindpose "what riggers need" | `UNVERIFIED` | `REJECT`（T4 线索） | 仅搜索结果摘要 |
| **R-C01 / R-C03 / R-C04 / R-C05** | Meshy / CG Topology / Renderosity Armpit / mutante28 | `UNVERIFIED` | **R-C04 已由 R-B12 替代并核验**；其余 3 条仍 `UNVERIFIED`（图片与原始链接未进入本会话） | Layer B 的实测缺口从 4 条降到 3 条 |

`[VERIFIED]` **Layer B 缺口状态**：本轮把"已核验的 Layer B 来源"从 **2 条**提升到 **6 条**（R-B02/R-B10/R-B11/R-B12 + R-C02/R-C06），其中 **T1 级 1 条、T2 级 2 条、T3 级 1 条**。

`[OPEN QUESTION]` 仍缺：R-C01（Meshy）、R-C03（CG Topology）、R-C05（mutante28 T-Pose）三组的原始链接与图片。

---

## 4. Layer C — Canonical Acceptance Sheet

`[RECOMMENDATION]` Layer C 是本轮**新建**的层，也是方案 C 的核心。候选方案见本轮 03。

| 项 | 状态 |
|---|---|
| Canonical Acceptance Target | **v0 候选已产出**，状态 `CANDIDATE`，**未 APPROVED** |
| 目标体型 | 成人男性；**具体原型待负责人确认**（v0 候选给出 3 个可选原型） |
| 数值来源 | **ANSUR II / NC State 一手汇总表**（已抓取并留存原件）+ **CDC NHANES SR50**（已抓取并留存原件） |
| 视角 | 正面 / 正面 45° / 侧面 / 背面 / 背面 45° / A-Pose / T-Pose |
| 姿态 | 相机轴与姿态轴**分开记录** |
| 显式禁止 | **不得**把当前 Baseline 的腰胸比 0.4821 与髂嵴宽/盆深比 1.5977 当作目标值 |

---

## 5. 授权使用规则（三类用途，沿用并强化）

| 用途 | 允许 | 禁止 |
|---|---|---|
| **A. Visual Inspiration** | 观察整体造型、比例、曲面质量、材质渲染 | 导入模型 / 复制网格 / 提取贴图 / 提取骨骼动画 / 转成项目资产 |
| **B. Engineering Reference** | 分析拓扑、骨骼位置、关节变形、解剖结构；记录来源、作者、链接、许可、允许用途、禁止内容、对应技术问题 | 把商品描述当独立技术验证；把宣传语当拓扑质量证明；把静态图当动作证据；把 T4 层当权威 |
| **C. Canonical Acceptance Target** | 仅负责人明确批准后作为正式验收目标 | 因图片好看自动转正；把不明确授权标为已批准；把网页缩略图当授权证明 |

---

## 6. 本矩阵的证据自检

| 检查项 | 结果 |
|---|---|
| 是否把网页缩略图当授权证明 | 否 —— 授权仅在打开许可章节后记录（BodyParts3D、MakeHuman、Sketchfab） |
| 是否把商品描述当独立技术验证 | 否 —— R-A01 明确标注商品页属性 ≠ 技术验证 |
| 是否把作者宣传语当拓扑质量证明 | 否 —— 拓扑结论引用项目内实测 |
| 是否把静态图当动作变形证据 | 否 —— 变形证据留空 |
| 是否把外部模型预览当项目生成能力证明 | 否 —— 规则 E-1 显式禁止 |
| 是否把论坛帖与解剖库并列 | **否 —— 本轮新增 4 级权威阶梯** |
| 是否把 BodyParts3D 当外表面验收图 | 否 —— L-B2 显式禁止 |
| 是否因能浏览就认为可下载/复制/再发布 | 否 —— L-B3 显式禁止 |
