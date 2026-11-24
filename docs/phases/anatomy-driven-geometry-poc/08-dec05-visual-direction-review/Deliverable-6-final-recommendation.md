# Deliverable 6 — Final Recommendation

> 状态：**草案 / DRAFT — 待负责人审批**。本文件不签署 DEC-05、不将 G2/G3 标记为 PASS、
> 不修改任何项目文档或代码。
> 证据标签：`[FACT]` / `[INFERENCE]` / `[RECOMMENDATION]` / `[OPEN QUESTION]` +
> `VERIFIED` / `PARTIALLY VERIFIED` / `DECLARED ONLY` / `UNVERIFIED` / `REJECTED`。

---

## 1. 推荐采用的视觉方向

**`[RECOMMENDATION]`** 项目最终视觉方向：**High-quality Stylized Realism（高品质风格化写实 3D 人物）**。

**目标层级（负责人已明确，本轮不再待决）：**

| 层级 | 定位 | 当前状态 |
|---|---|---|
| **Level 1 — Structurally Valid** | 工程基础门槛 | PoC-SM 必须达到 |
| **Level 2 — High-quality Stylized Human** | **项目核心视觉目标** | **已定** |
| **Level 3 — Production Visual Quality** | **长期方向** | **已定**，不作当前判据 |

**阶段分工（负责人已明确）：** P1 先证明人体结构、曲面和驱动质量；P2 再证明换装和模块化外观。

`[FACT/VERIFIED]` 该方向与项目既有硬约束在结构上同向：C++17 / Android Kotlin+JNI+NDK / EGL / GLES3 / 自研 Renderer / 无外部模型资产 / 低端 Android。**高品质视觉目标 ≠ 高面数网格，也 ≠ 外部模型。**

`[INFERENCE]` G2 的实测数据为这个方向提供了**技术依据**，而非仅是审美偏好：

- `[FACT/VERIFIED]` R3 的失败模式是**形态表达力不足**（上胸缺口 13.5 cm、有效参数 7/20、模板加密几乎不收敛），不是面数不够；
- `[FACT/VERIFIED]` R2 的 E2 投影误差仅 **0.98 mm**、0 sliver、正交 quad 比 0.935、顶点 ID 跨参数组恒定——**后端已经够好，缺的是形态**；
- `[FACT/VERIFIED]` 因此"提升视觉完成度"只能从**形态关系**入手（Anatomy-driven Shape Model），不能从"加面数/换渲染"入手。这与 Level 2 的定义完全一致。

---

## 2. 五 Gate 分离（对验收结构的核心建议）

`[FACT/VERIFIED — 负责人指示]` 不把 P1 几何验收与最终渲染品质混成一个 Gate：

| Gate | 应该验证什么 | 不应该用什么代替 | 当前证据 |
|---|---|---|---|
| G-P1-GEO | 解剖比例、曲面连续性、人体体积 | 高质量皮肤贴图 | **不成立**（R3 上胸 13.5 cm 缺口；R1/R2 肩根接缝 47.66° > ≤5°） |
| G-P1-TOPO | 肩根、腋下、胯部、关节结构 | 单纯统计面数 | **基本成立**（R2：0 sliver / 0 非登记 boundary / 0 non-manifold / 0 degenerate / components 1 / 0.935 正交 quad 比 / ID 稳定） |
| G-P1-ANIM | 关节弯曲、体积保持、变形连续性 | 静态 T-Pose | **CPU 侧成立**（权重合法；刚体探针 22/22 零误差；bind residual ≤2.4e-7）；**GPU parity UNVERIFIED** |
| G-P2-MOD | 服装、发型、配件独立切换 | 单张穿着完整服装的效果图 | 未开始 |
| G-FINAL-VIS | 整体角色完成度、材质、光照 | 单纯几何测试通过 | 无实机数据 |

`[INFERENCE]` **这张表就是本轮最重要的结论。** 它用五个 Gate 独立地解释了为什么"先做 Shape Model、保留 R2 后端"是对的：

- 如果只看一个混同的"视觉 Gate"，R2 会因形态不达标而整体被否，**连同它已经合格的拓扑与驱动能力一起丢掉**；
- 分开看，R2 在 TOPO 与 ANIM(CPU) 两关**已经站得住**，唯一真正缺的是 GEO；而 GEO 缺的正是 09 文档说的"形态语义"；
- 所以路径是唯一的：**用已合格的 R2 拓扑后端，去承载一个形态正确的新 Shape Model**。

`[RECOMMENDATION]` 这条应写入 `09-final-recommendation.md` 的"交付状态"，并作为 PoC-SM 的设计依据。

---

## 3. 推荐的主视觉参考（Layer A）

`[RECOMMENDATION]` **当前不建议指定任何单一参考作为 Layer A Canonical Target。**

理由（全部来自本轮实查）：

| 候选 | 问题 |
|---|---|
| R-A01 TurboSquid Young Man | 可作**风格化角色质量参照**（负责人已确认此用法）；但商品页描述其为带绑定、PBR、贴图的成品商业模型——**不能证明人体拓扑质量，不能证明低端 Android 实时性能，不能证明程序化生成能力** |
| R-A02 CGTrader Cartoon Family 2 | 比例偏卡通，非自然成人比例 → 不作 Layer A；仅 C 层观察 |
| R-A03 David Rig | 抓取失败，无法确认对应作品 → `UNVERIFIED`，不采用 |
| R-A04 Maxine Alvarez | AI 角色互动游戏，非几何资料 → 不作 A/B 层 |
| R-A05 DeepMotion | 技术/产品来源，网站品牌不是几何标准 → `REJECTED` 作参考 |
| R-A06 ArtStation MetaHuman | 写实完成度可观察；依赖外部生态，与生产约束冲突 → 仅观察 |
| R-A07 Sketchfab Male turnaround | **唯一授权已核验（CC BY 4.0）**；但为 Rodin Gen-1 生成模型 → 仅观察 + 需署名 |
| R-A08 Sketchfab Streetwear | 授权未核验；50 万面 → `UNVERIFIED` |

`[RECOMMENDATION]` **主视觉参考 = 自有 Canonical Reference Sheet**（详见 §8 与 Deliverable 5 Q-3）。在它存在之前：

- Layer A 的"风格化写实完成度"由**负责人判断**（负责人本轮给出的文字描述即为当前最高规格的定义）；
- Layer A 的"比例与体积关系"可由 AI 制作的参数化比例基准承担；
- **不得**因为某个商业模型好看就自动升级为 Canonical。

---

## 4. 推荐的人体结构参考（Layer B）

`[RECOMMENDATION]` 进入工程研究文档的只有 2 条：

| Reference ID | 名称 | 许可 | 用途 | 证据状态 |
|---|---|---|---|---|
| **R-C06** | MakeHuman Community | **CC0**（FAQ 原文已核验："The asset license is CC0"） | 人体比例的**参数维度设计**（各围度如何参数化） | 许可已核验 |
| **R-C02** | Blender 4.2 Manual — Bone Positioning Guide | **网站内容 CC BY 4.0 已核验**；**手册正文许可未核验**（通行说法 CC BY-SA 3.0，本轮两次抓取许可页均被 Cloudflare 拦截，未取得逐字证据） | 骨骼位置 / roll / alignment 的通用做法 | **正文与许可均未逐字捕获 → 仅作线索** |

`[REJECTED / UNVERIFIED]` 作为工程依据的候选：`R-C07`（MessyPoly，页面无许可声明）、`R-C08`（basemesh，Cloudflare 拦截）、`R-C09`（bindpose，仅搜索摘要）—— 授权无法确认，仅作线索去追原始出处。

`[RECOMMENDATION]` Layer B 的技术内容**必须由项目自己的合同补足**，不能依赖外部参考：

| Layer B 待验证项 | 项目内已有的合同依据 |
|---|---|
| 肩根连续连接、无独立肩球 | SPEC §10.1 T2 肩根边界/连接拓扑 |
| 腋窝/胯部结构 | SPEC §10.1 T2 + T3 肩部形变环 |
| 关节结构与变形拓扑 | SPEC §13（非流形=0、退化=0、components=1） |
| 关节弯曲的体积保持 | SPEC §9（LBS 先作基线；体积损失由动作数据触发 corrective 评估） |
| 拓扑质量的可观测量 | G2 实测：sliver / 最小内角 / 正交 quad 比 / 顶点 ID 稳定性 |

`[OPEN QUESTION]` 任务书 B 组的 4 组拓扑参考（`R-C01` Meshy / `R-C03` CG Topology / `R-C04` Renderosity Armpit / `R-C05` mutante28）本轮**图片与原始链接均未进入会话**，无法核验。这是当前 Layer B 的**最大缺口**（OQ-29）。

---

## 5. 推荐的模块化外观参考（Layer C）

`[RECOMMENDATION]` 本轮**只做观察，不进工程研究文档**，P2 阶段另立 rubric：

| Reference ID | 名称 | 观察什么 |
|---|---|---|
| R-B01 | Modular Chibi Boy | 服装/发型/配件的**组织方式**（任务书明确：不得作人体比例标准） |
| R-B03 | REV WORLDS — Avatar Fashion | 外观切换 UX、模块边界与遮挡思路 |
| R-A02 | Cartoon Family 2 | 角色组配置与变体组织 |
| R-A04 | Maxine Alvarez | 外观切换 UX |

`[FACT/VERIFIED]` 按五 Gate 分离，`G-P2-MOD` 属 P2 阶段，**本期不评**。把 Layer C 混入 P1 评审会直接违反任务书"这一层不能降低 P1 的基础人体质量要求"。

---

## 6. 当前不推荐的参考

| Reference ID | 名称 | 不推荐理由 | 级别 |
|---|---|---|---|
| **Vincent（Blender Studio）** | — | 负责人明确指令：**不得继续沿用作为主视觉标杆**；当前呈现的简化粗糙风格属被排除方向 | `REJECTED` 作主标杆；降为历史不足案例 |
| R-A05 | DeepMotion | 技术/产品来源，网站品牌不是人体几何标准 | `REJECTED` 作视觉/几何参考；可作技术背景阅读 |
| R-A02 | Cartoon Family 2 | 成人比例不自然（角色身高 135/100/160 cm 为家族设定） | `REJECTED` 作 Layer A；CONDITIONAL 作 Layer C |
| R-A03 | David Rig | 无法确认对应作品 | `UNVERIFIED` — 不采用，除非负责人提供可核验页面/作品名 |
| R-A08 | Sketchfab Streetwear | 授权未核验 + 50 万面 + AI 生成 | `UNVERIFIED` / `REJECTED` 作 Canonical |
| R-B01 | Modular Chibi Boy | 大头小身体比例属明确排除方向 | `REJECTED` 作 Layer A；仅 Layer C 观察 |
| — | 任何 3D 扫描 / 照片级真人模型 | 与"不依赖外部模型资产"冲突；非本项目能力证明 | `REJECTED` |
| — | 任何"搜索结果缩略图"当授权证据 | 任务书明令禁止 | `REJECTED` |

---

## 7. DEC-05 是否可以提交负责人审批？

`[RECOMMENDATION]` **可以提交审批，但必须标注为"附条件草案"，不能当作已可执行的视觉标准。**

理由与条件：

| # | 条件 | 说明 |
|---|---|---|
| C-1 | 提交的是**修订草案**，非批准文本 | Deliverable 3 已按此写；SPEC 的 DEC-05 行状态保持 `OPEN`，签字栏留空 |
| C-2 | 明确写出**当前参考矩阵不足以支撑 Layer A Canonical** | 这是本轮最重要的结论之一，隐去会让审批者误以为参考已就绪 |
| C-3 | 明确写出 **§13 阈值仍为 TBD，且不得由 AI 补齐** | 只有 ≤5° 是合同既有值 |
| C-4 | 附上**五 Gate 结构**作为验收方案 | 负责人本轮已确认方向，这是把方向落成可执行结构的部分 |
| C-5 | 附上**缺口清单**（OQ-28 Canonical Sheet、OQ-29 Layer B 链接） | 让审批者知道批准后还差什么 |
| C-6 | **不以本草案代替负责人签字** | AI 不签 DEC-05 |

`[FACT/VERIFIED]` 若按 C-1~C-6 提交，DEC-05 的审批对象是**方向 + 结构 + 参考体系骨架 + 缺口**，不是"一份已经可以照着做图的完整标准"。后者的前置是 OQ-28/OQ-29。

---

## 8. 当前是否具备启动后续阶段的条件？

`[RECOMMENDATION]` **分阶段回答：几何实现阶段"有条件具备"，视觉判定阶段"尚不具备"。**

| 后续阶段 | 是否具备 | 依据与前置 |
|---|---|---|
| **PoC-SM 代码实施**（新 Shape Model v2） | **有条件具备** | 前置：OQ-02（AUTH-08 基线 commit + worktree 路径）。已具备的技术基础：R2 拓扑后端（G-P1-TOPO 基本成立）、CPU 蒙皮链路（权重合法 + 刚体探针 22/22）、09 文档的分层契约（DEC-09 已批准） |
| **PoC-SM 的视觉验收**（`G-P1-GEO` 判 PASS） | **尚不具备** | 前置：OQ-28（Canonical Reference Sheet 或等价基准）+ OQ-03（≤5° 阻塞确认）。否则只能产出数值，不能判合格 |
| **`G-P1-TOPO` / `G-P1-ANIM` 验收** | **部分具备** | TOPO 可凭项目内合同判定；ANIM 的 CPU 侧已成立，但姿态轴协议未定义（OQ-30/32/33）、GPU parity 未验（OQ-19） |
| **P2 模块化** | **不具备** | 属 `G-P2-MOD`，P1 之后；Layer C 参考仅观察级 |
| **`G-FINAL-VIS`** | **不具备** | 无实机渲染/性能数据；材质与光照基线未定义（OQ-35） |
| **G3 签署** | **有条件具备** | 前置：OQ-05 签署 G3 DEFER 记录；OQ-03/04 确认 |
| **G4 完整角色扩展** | **不具备** | SPEC 要求 G3 已签署且视觉 Gate 通过 |

`[INFERENCE]` 因此最有效的推进顺序是**并行而非串行**：

```
轨道 A（负责人侧，解输入阻塞）
  OQ-28 Canonical Sheet / OQ-29 Layer B 链接
  OQ-02 AUTH-08 基线与路径
  OQ-03 ≤5° 阻塞确认 / OQ-04 ADR 签署 / OQ-05 G3 签署

轨道 B（AI 侧，可立即开始，边界清晰）
  ① 把五 Gate 结构写进 SPEC 与 04-rubric（仅在轨道 A 批准后落文档）
  ② 姿态轴协议（OQ-30/32/33）——纯协议扩展，不碰几何算法
  ③ 参数量化基准表（Layer B 的自有部分，见 §9）
  ④ 维持 G2 证据冻结，不动 R2/R3 代码量
```

`[RECOMMENDATION]` **轨道 B 的 ③ 值得强调**：G2 暴露的核心问题是"形态语义缺失"，而 §13 的比例指标（腰/胸宽度比 0.4821、髂嵴宽/盆深比 1.5977）**至今是裸值，没有对照基准**。为这些量建立**可解释的参数化比例基准**（不是拍脑袋的目标区间，而是"人体比例的哪些维度必须可控、各维度的相互约束关系"），既不需要等 Canonical Sheet，也不越界定义"好看"，而且正好是 PoC-SM 要实现的东西。

---

## 9. 交付与边界声明

### 9.1 本轮实际产出

| 文件 | 内容 |
|---|---|
| `Deliverable-1-visual-direction.md` | 视觉方向决策、Level 1/2/3、五 Gate 分离及其与 G2 证据的对应 |
| `Deliverable-2-reference-source-matrix.md` | 20 条参考的完整矩阵、三条通用证伪规则（E-1~E-3） |
| `Deliverable-3-DEC-05-revised-draft.md` | DEC-05 完整修订草案（签字栏留空） |
| `Deliverable-4-visual-rubric-revision.md` | rubric 修订建议：8 维度、视角×姿态正交、5 种图像证据、V-1~V-7、按 Gate 分表签署 |
| `Deliverable-5-open-questions.md` | 7 个指定问题 + OQ-27~OQ-35 + 对 `07-open-issues.md` 的建议状态更新 |
| `Deliverable-6-final-recommendation.md` | 本文件 |

### 9.2 本轮**未**做的事（逐条对照任务书 §九禁止项）

| 禁止项 | 是否触碰 |
|---|---|
| 修改 C++ 源码 | ❌ 未修改 |
| 修改 Renderer | ❌ 未修改 |
| 修改几何算法 | ❌ 未修改 |
| 修改骨骼 ABI / JNI ABI | ❌ 未修改 |
| 修改生产资产系统 | ❌ 未修改 |
| 导入任何外部模型 | ❌ 未导入 |
| 创建 Worktree / 分支 | ❌ 未创建 |
| 修改 Git 配置 | ❌ 未修改 |
| Commit / Push | ❌ 未执行 |
| 自行签署 ADR | ❌ 未签署 |
| 自行将 DEC-05 标记为 APPROVED | ❌ 保持 `OPEN` |
| 自行将 G2/G3 标记为 PASS | ❌ 未标记 |

`[FACT/VERIFIED]` 全部 6 份草案位于**仓库外**目录 `E:/AI-Tools/DEC-05-Review-Drafts/`；仓库 `E:/AI-Tools/ProceduralAvatarEngine-PoC` 状态保持干净，HEAD 仍为 `7c451ae`，未产生任何 diff。

### 9.3 证据纪律自检

`[FACT/VERIFIED]` 本轮自检结果：

| 检查项 | 结果 |
|---|---|
| 是否把网页缩略图当授权证明 | 否 —— 授权仅在打开许可页确认后记录（R-C01 / R-C02 / R-A07） |
| 是否把商品描述当独立技术验证 | 否 —— R-A01 明确标注"商品页描述 ≠ 技术验证" |
| 是否把作者宣传语当拓扑质量证明 | 否 —— 拓扑结论一律引用项目内实测（sliver / non-manifold / 边界环） |
| 是否把静态图当动作变形证据 | 否 —— D6 单列且标 `NOT RUN` |
| 是否把外部模型预览当项目生成能力证明 | 否 —— 规则 E-1 显式禁止，R-A01 明确"不能证明程序化生成能力" |
| 是否把 DEC-05 草案说成已批准 | 否 —— 全部标 `DRAFT` / `OPEN`，签字栏留空 |
| 是否把无图像证据的视觉 PASS 写成 PASS | 否 —— 无一项视觉判 PASS；`G-P1-GEO` 明确记为**不成立** |

---

## 10. 一句话建议

`[RECOMMENDATION]` **批准 DEC-05 修订草案的方向与结构（五 Gate + 三层参考 + Level 2 核心），同时把 Layer A 的 Canonical 参考与 Layer B 的拓扑链接作为明确的待补输入挂到 OQ-28/OQ-29；PoC-SM 的代码授权（OQ-02）可以与之并行推进，因为它在 `G-P1-TOPO` 与 `G-P1-ANIM(CPU)` 两关已有实证基础——真正缺的那一关（G-P1-GEO），正是一份新 Shape Model 要解决的事。**
