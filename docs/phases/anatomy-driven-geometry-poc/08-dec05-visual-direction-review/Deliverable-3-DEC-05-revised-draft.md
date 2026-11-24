# Deliverable 3 — DEC-05 Revised Draft

> 状态：**草案 / DRAFT — 不是批准文本**。负责人签字栏为空。
> 本草案只提交给负责人审阅；不修改 SPEC 中的 DEC-05 行，不把 DEC-05 标为 APPROVED。
> 措辞：原 DEC-05 问题"目标参考形态与视觉验收数据"扩展为三部分：视觉目标、三层参考体系、视觉质量等级。

---

## DEC-05（修订草案）— 目标参考形态与视觉验收数据

| 字段 | 草案内容 |
|---|---|
| ID | DEC-05 |
| Question | 目标参考形态、参考图授权体系与视觉验收标准 |
| Options | ①负责人提供授权参考图 + rubric；②仅灰模协议无外部参考；③AI 自定 |
| **Recommendation（非批准）** | ①负责人提供/批准授权参考图与 rubric，并采用三层参考体系 + 三级质量目标；AI 不得自定、不得自签 |
| Rationale | 没有负责人认可的目标形态，AI 只能做数值对照，不能宣布视觉合格（原 DEC-05 已成立）。本轮补充：所有人选参考均未核验为 Layer A Canonical Target，必须建立授权可追溯的参考体系 |
| Validation | 固定机位视觉 Gate，owner 独立否决；不得用不同相机/光照/材质掩盖缺陷 |
| Approval Owner | **项目负责人** |
| 状态 | `OPEN` — 草案待审阅 |

### 1. 视觉目标（草案）

`High-quality Stylized Realism`：自然可信的人体比例与解剖结构；肩/胸/腹/盆/四肢连续曲面；无球体拼接、圆管四肢、明显低模/玩具感；保留艺术化特征但不牺牲结构；可程序化生成、可实时驱动；不追求照片级扫描（详见 Deliverable 1）。

`[FACT]` Vincent 不再作为主视觉标杆，降级为历史不足案例（待负责人确认）。

### 2. 三层参考体系（草案）

| 层级 | 定义 | 当前候选状态 | 用途限制 |
|---|---|---|---|
| **Layer A — Final Visual Target** | 项目最终外观目标 | **无 ACCEPTED 候选**；R-A07（CC BY 4.0 turnaround）为唯一授权可核验候选但仅 CONDITIONAL 观察；其余 UNVERIFIED/REJECT | 不得用单张正面图代替完整人体质量；不得当作生成能力证明 |
| **Layer B — Anatomical & Topology Target** | P1 人体几何与拓扑验收参考 | R-C06 MakeHuman（**CC0，已核验**，工程研究 ACCEPT）；R-C02 Blender 手册（**网站 CC BY 4.0 已核验 / 手册正文许可未核验**，仅作文字线索）；R-C07/R-C08/R-C09 授权 UNVERIFIED（线索，不进正式文档）；R-C01/R-C03/R-C04/R-C05 用户图片本轮未附，UNVERIFIED | 静态外观与拓扑分开评价；漂亮 ≠ 适合动画；线多 ≠ 解剖正确 |
| **Layer C — Modular Appearance Target** | P2 外观模块化参考 | R-B04 Character Creator（CONDITIONAL 系统能力）；R-B01 Chibi Boy（仅模块组织，比例 REJECT）；R-A02/R-A04/R-B05（CONDITIONAL） | 不得降低 P1 基础人体质量要求；商业预览不得导入 |

### 3. 参考图清单（草案，完整矩阵见 Deliverable 2）

只列推荐状态为 `ACCEPT` 或 `CONDITIONAL` 者；`UNVERIFIED` 与 `REJECT` 不进入正式清单，保留在矩阵中备查。

| ID | 名称 | 层级 | 授权 | 允许的用途 | 禁止的用途 |
|---|---|---|---|---|---|
| R-C06 | MakeHuman Community | B | **CC0（已核验）** | 比例/体型控制的工程研究 | 作为生产资产导入（项目硬约束） |
| R-C02 | Blender 4.2 LTS Manual — Bone Positioning | B（仅线索） | 网站内容 CC BY 4.0（已核验）；**手册正文许可未核验** | 骨骼位置/roll 的一般性线索 | 作为本项目骨骼 ABI 依据；复制图文；作可引用出处 |
| R-A07 | Male human turnaround (0C557H512) | A（观察） | CC BY 4.0（已核验） | 视觉观察，需署名 | 导入生产；证明项目生成能力 |
| R-B04 | Reallusion Character Creator | C | 软件许可；预览图许可 UNVERIFIED | 模块化系统能力观察 | 导入素材；推断预览图授权 |
| R-B01 | Modular Chibi Boy | C（组织方式） | UNVERIFIED | 仅观察模块/配件组织 | 作为人体比例标准 |

**Canonical Acceptance Target：空。** 需负责人提供自有参考图，或批准制作自有 Canonical Reference Sheet，或明确指定某张合法商业参考仅作视觉观察并签字。

### 4. 授权规则（草案）

| 用途 | 允许 | 禁止 |
|---|---|---|
| **A. Visual Inspiration** | 观察整体造型、比例、曲面质量、材质渲染 | 导入模型/复制网格/提取贴图/提取骨骼动画/转换成项目资产 |
| **B. Engineering Reference** | 分析拓扑、骨骼位置、关节变形、解剖结构；记录来源/作者/原始链接/许可/允许用途/禁止内容/对应技术问题 | 把商品描述当独立技术验证；把宣传语当拓扑质量证明；把静态图当动作证据 |
| **C. Canonical Acceptance Target** | 仅负责人明确批准后可作为正式验收目标 | 因图片好看自动转正；把不明确授权标为已批准；把网页缩略图当授权证明 |

### 5. 视觉质量等级（草案 — 层级已由负责人明确）

`[FACT/VERIFIED — 负责人指示 2026-11-24]` **Level 2 为项目核心视觉目标；Level 3 为长期方向；P1 先证明人体结构/曲面/驱动质量，P2 再证明换装与模块化外观。**

| 级别 | 定位 | 当前阶段是否要求 | 说明 |
|---|---|---|---|
| Level 1 — Structurally Valid | 工程基础门槛 | **是，PoC-SM 门禁** | 比例合理、区域可辨、曲面连续、无球体/圆管拼接、关节结构正确、基础骨骼可驱动、三视角无严重缺陷 |
| **Level 2 — High-quality Stylized Human** | **项目核心视觉目标** | **是（核心）** | 需 Canonical Reference Sheet 才能判定；阈值 `TBD`。**PoC-SM 的实际判据 = Level 1 全部达成 ∩ `G-P1-GEO` 从"不成立"转为"成立"**，而非"Level 1 达成即可" |
| Level 3 — Production Visual Quality | 长期方向 | 否（长期） | 多体型/多外观/实机性能；阈值 `TBD`。不得为追求静态视觉破坏工程约束 |

`[RECOMMENDATION]` 三级不得平均成总分；Level 1 是门禁，Level 2 是核心目标，Level 3 是长期方向。

### 5.1 五 Gate 验收结构（草案 — 负责人已指示分离）

`[FACT/VERIFIED — 负责人指示]` **不要把 P1 的几何验收和最终渲染品质混成一个 Gate。**

| Gate | 验收对象 | 应该验证什么 | 不应该用什么代替 | 判定者 |
|---|---|---|---|---|
| `G-P1-GEO` | P1 人体几何 | 解剖比例、曲面连续性、人体体积 | 高质量皮肤贴图 | 负责人（视觉）+ 脚本（数值） |
| `G-P1-TOPO` | P1 拓扑 | 肩根、腋下、胯部、关节结构 | 单纯统计面数 | 脚本（合同值）+ 负责人（结构） |
| `G-P1-ANIM` | P1 动画 | 关节弯曲、体积保持、变形连续性 | 静态 T-Pose | 负责人（姿态图）+ 脚本（权重/CPU） |
| `G-P2-MOD` | P2 模块化 | 服装、发型、配件独立切换 | 单张穿着完整服装的效果图 | P2 阶段另定 |
| `G-FINAL-VIS` | 最终视觉 | 整体角色完成度、材质、光照 | 单纯几何测试通过 | 负责人 |

`[RECOMMENDATION]` 三条实现规则（详见 Deliverable 4 §2.4）：

- **正交判定** —— 一个 Gate 的 PASS 不能被另一个 Gate 的 PASS 推导；
- **禁止替代** —— "不应该用什么代替"栏里的东西即使做得再好也不能满足该 Gate；
- **分表签署** —— 五个 Gate 分别签署，不合成单一"视觉 Gate"结论；
- **下游不予受理** —— `G-P1-GEO` 未通过时，`G-FINAL-VIS` 不进入评审（不是判 FAIL）。

`[FACT/VERIFIED]` 该分离与当前 G2 证据直接对应（详见 Deliverable 1 §4.2 与 Deliverable 6 §2）：`G-P1-TOPO` 基本成立、`G-P1-ANIM` 的 CPU 侧成立、`G-P1-GEO` 不成立。

### 6. 固定视角与姿态（草案）

按任务书第七节：Front、Front 3/4、Side、Back、Back 3/4、T-Pose、A-Pose。

`[FACT/VERIFIED — 协议缺口]` 现有 `CameraProtocol.h:fixedViews()` 提供的是 **G2 路线比较用的 7 机位**（front / front-45-left / side-left / side-left-30up / back / back-45-right / front-30up），**不是**视觉验收视角，且缺 T-Pose / A-Pose。

`[INFERENCE]` 更根本的问题是 **T-Pose / A-Pose 不是相机方位，而是角色姿态**：同一套静态网格换机位不会变成 T-Pose，它必须经过骨架与蒙皮。因此正确结构是**正交的两轴**：

```
视角轴（7 机位，继承 G2 已冻结的 CameraProtocol）
    × 姿态轴（bind pose / A-Pose / T-Pose / 肘屈90° / 肩前屈90°）
```

`[RECOMMENDATION]` 最小充分证据集：

| 用途 | 视角 | 姿态 | 归属 Gate |
|---|---|---|---|
| 几何/解剖/连续性 | 现有 7 机位 | bind pose | `G-P1-GEO` |
| 驱动/变形 | front / side-left / back-45-right / side-left-30up | bind / A-Pose / T-Pose / 肘屈90° / 肩前屈90° | `G-P1-ANIM` |
| 拓扑 | 无视角（wireframe / 关节环标注） | bind | `G-P1-TOPO` |
| 完成度与性能 | 全部视角 | 代表姿态 | `G-FINAL-VIS` |

`[RECOMMENDATION]` 图像证据至少五种，各自单一职责、不得混用：**Neutral Gray Clay**（几何主证据，唯一可用于几何合格判定）、**Silhouette**（比例轮廓）、**Anatomical Region Visualization**（区域语义）、**Wireframe / Topology**（拓扑）、**Joint Deformation Comparison**（驱动）。

`[FACT/VERIFIED]` 统一参数：相机正交；使用与数值轮廓协议**同一个** `viewFrame()`；光照视空间固定 (0.55, 0.78, −0.45)；Lambert 0.28 环境光 + 0.72 × max(0, N·L)；背景 (28,28,31)；480×480；同视图内所有路线共享 frame + 8% 边距。**禁止**用不同相机/光照/材质掩盖缺陷。

`[OPEN QUESTION]` 相机高度绑定方式（解剖轴 vs 固定世界 Y）、A-Pose/T-Pose 的确切角度、必检动作集 —— 见 Deliverable 5 的 OQ-30/31/32。

### 7. 一票否决条件（草案）

**视觉类（需负责人目视判定）：**

- 独立肩球、圆管式四肢、明显 mesh append 痕迹；
- 肩根/腋下/胯/肘/膝出现穿透或自交；
- 只在正面好看，侧面/背面/动作姿态明显失真；
- 用高质量贴图掩盖基础几何缺陷；
- 未经负责人批准即宣布视觉 PASS；
- 为静态视觉破坏 C++17/NDK/GLES3 工程约束或冻结 ABI。

**合同类（可脚本自动判定，阈值均为 SPEC 既有值，非新增）：**

| # | 条件 | 合同依据 | 当前状态 |
|---|---|---|---|
| V-4 | non-manifold edge > 0 或 degenerate triangle > 0 | SPEC §13 | PASS |
| V-5 | 权重非法（>4 影响骨 / 负权 / 和偏离 >1e-3 / 骨 ID ∉ [0,21]） | SPEC §11.1 | PASS |
| V-6 | 组件数 ≠ 1（已登记肩根 crop 除外） | SPEC §10.1 | PASS |
| V-7 | 肩根接缝最大二面角 > 5° | SPEC §13（既有合同值） | **FAIL**（R1/R2 47.66°、R3 16.13°） |

`[FACT/VERIFIED]` V-7 归属 `G-P1-GEO`，是当前唯一可判 FAIL 的数值项。

### 8. 尚未解决的问题（OPEN QUESTION）

**已由负责人答复（不再待决）：**

1. ~~是否批准 High-quality Stylized Realism 为最终视觉方向？~~ → **已定：Level 2 核心、Level 3 长期**。
2. ~~是否批准 Level 1 为 PoC-SM 门禁、Level 2 为核心目标？~~ → **已定**，并补充 P1/P2 分工。
3. ~~Vincent 是否正式降级？~~ → **已定：不再沿用为主视觉标杆**。

**仍待负责人决定：**

4. Canonical Acceptance Target 走哪条路径（负责人自有图 / AI 制作比例与体积关系基准 / 组合）—— 见 OQ-28；
5. 任务书中"用户提供的图片"需重新附上原图与原始链接（Layer B 的 4 组拓扑参考本轮未进入会话）—— 见 OQ-29；
6. 五 Gate 是否写入 SPEC Gate 链作为 GATE-04 之后的细分 —— 见 OQ-27；
7. 姿态轴协议（A-Pose/T-Pose 定义、必检动作集、相机高度绑定）—— 见 OQ-30/31/32；
8. 姿态轴实现（`applyPose` + `band = none`）是否纳入 AUTH-08 范围 —— 见 OQ-33；
9. `G-FINAL-VIS` 的材质与光照基线由谁定义 —— 见 OQ-35；
10. "高品质 ≠ 高面数"是否需要写成可核验的评审规则 —— 见 OQ-34。

### 9. 签署

```
DEC-05 状态： OPEN / APPROVED（负责人填写）
本轮批准的层级：  Level 1 门禁 / Level 2 核心目标 / Level 3 长期方向
负责人签字：____________________  日期：____________
Canonical Acceptance Target：________________________（负责人填写）
五 Gate 结构：  采纳 / 修改后采纳 / 暂不采纳
```
