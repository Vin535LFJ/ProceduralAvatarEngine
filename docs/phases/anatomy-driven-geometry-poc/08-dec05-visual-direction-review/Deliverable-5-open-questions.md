# Deliverable 5 — Open Questions

> 状态：**草案 / DRAFT**。本文件不修改 `07-open-issues.md`，只提出建议的状态更新。
> 编号沿用项目已有的 `OQ-xx` 序列（`docs/phases/anatomy-driven-geometry-poc/07-open-issues.md`），不另起并行编号。

---

## A. 负责人本轮点名的 7 个问题

### Q-1 — 是否批准 High-quality Stylized Realism 作为最终视觉方向？

**答案：已由负责人明确，本轮不再待决。**

`[FACT/VERIFIED — 负责人指示 2026-11-24]` 项目最终视觉方向为 **High-quality Stylized Realism**；**Level 2 — High-quality Stylized Human 设为项目核心视觉目标**，**Level 3 — Production Visual Quality 作为长期方向**。

对 SPEC 寄存器的影响：`DEC-05` 的 Question 字段需从"目标参考形态与视觉验收数据"扩展为"视觉方向 + 三层参考体系 + 三级质量目标 + 五 Gate 验收"（见 Deliverable 3 草案）。

**对 OQ-01 的影响**：不再只是"参考图从哪来"，而是"参考体系 + 目标层级 + 验收 Gate 结构"三件事一起定。仍需负责人提供的是**可核验的 Layer A 参考**（见 Q-3）。

---

### Q-2 — 是否批准 Level 2 作为当前项目核心目标？

**答案：已批准。** 见 Q-1。

`[FACT/VERIFIED]` 阶段分工同时确定：**P1 先证明人体结构、曲面和驱动质量；P2 再证明换装和模块化外观。**

`[INFERENCE]` 这一分工与五 Gate 分离（Deliverable 1 §4.2、Deliverable 4 §2.4）互为表里：P1 对应 `G-P1-GEO` / `G-P1-TOPO` / `G-P1-ANIM` 三个 Gate，P2 对应 `G-P2-MOD`，`G-FINAL-VIS` 横跨两者。

---

### Q-3 — 是否需要单独制作自有 Canonical Reference Sheet？

**答案：需要。AI 强烈建议制作，且这应是解锁视觉判定的唯一路径。**

理由：

1. `[FACT/VERIFIED]` 本轮对 20 个候选来源做了实查，**没有任何一个**同时满足"可核验 + 授权明确 + 可作为 Layer A Canonical Target"。唯一授权已核验的是 R-A07（Sketchfab，**CC BY 4.0**），但它是 Rodin Gen-1 生成模型，只适合观察。
2. `[FACT/VERIFIED]` 任务书明确："不能因为某张商业模型图片看起来很好，就自动把它变成正式验收目标"，并要求"不得为了凑齐参考图而降低标准"。
3. `[INFERENCE]` 没有自有 Canonical Reference Sheet，`G-P1-GEO` 与 `G-FINAL-VIS` 永远 `NOT RUN`——这与 §13 全 TBD 的状态叠加，会让 PoC-SM 无法判定。

**建议的两条路径**（需负责人选一条或组合）：

| 路径 | 做法 | 成本 | 优点 | 缺点 |
|---|---|---|---|---|
| **A. 负责人提供自有参考图** | 负责人给出认可的目标图（可来自授权素材或委托美术） | 取决于负责人 | 权威、可直接作 Canonical | 依赖负责人时间 |
| **B. AI 制作自有 Canonical Reference Sheet** | 用程序化手段做一张**参数化目标比例表 + 示意轮廓图**（非成品渲染），声明"这是比例与体积关系的参考，不是外观风格参考" | 中 | 立即可用；与"程序化"目标一致 | 不含材质/表面风格信息 |
| **C. A + B 组合** | 负责人提供风格参考（Layer A 外观）+ AI 提供比例/结构参考（Layer B 结构） | 高 | 覆盖最完整 | 工作量最大 |

`[RECOMMENDATION]` **选 C**，但先做 B 的最小版本（比例与体积关系表），因为 B 恰好对应当前最缺、也最可程序化表达的部分——G2 暴露的正是比例与体积关系问题（R3 上胸缺口 13.5 cm、腰/胸宽度比 0.4821、髂嵴宽/盆深比 1.5977 都还是 `TBD` 区间里的裸值，没有对照基准）。

`[OPEN QUESTION — 需负责人决定]` 走 A、B 还是 C；以及 B 若获批，交付时机是 PoC-SM 之前还是之后。

---

### Q-4 — 哪些外部参考仅用于视觉观察？

**答案（可直接执行）：下列参考仅限"视觉观察"，不进入工程研究文档，不作任何技术断言。**

| Reference ID | 名称 | 限制 | 状态 |
|---|---|---|---|
| R-A01 | TurboSquid Young Man | 仅风格化角色质量参照；**不能**证明拓扑、性能、程序化能力 | CONDITIONAL |
| R-A02 | CGTrader Cartoon Family 2 | 仅角色家族/配置观察；**不能**证明人体比例可信 | CONDITIONAL（C 层） |
| R-A04 | Maxine "Max" Alvarez | 仅外观切换 UX 观察；**非几何资料** | CONDITIONAL（C 层） |
| R-A06 | ArtStation MetaHuman (KB Kookmin) | 仅写实完成度观察；依赖外部生态，与本项目约束冲突 | CONDITIONAL |
| R-A07 | Sketchfab Male turnaround (CC BY 4.0) | 仅观察；需署名；**唯一授权已核验** | CONDITIONAL |
| R-A08 | Sketchfab Stylized streetwear | 仅观察；授权未核验 | UNVERIFIED |
| R-B01 | Modular Chibi Boy | 仅服装/发型/配件的**组织方式**观察 | CONDITIONAL（C 层） |
| R-B03 | REV WORLDS Avatar Fashion | 仅模块化外观 UX 观察 | CONDITIONAL（C 层） |

`[RECOMMENDATION]` 这 8 条**全部**降为"仅观察"，且**不构成 Layer A Canonical**。任务书已明确 Chibi Boy"不能作为本项目的最终人体比例标准"。

---

### Q-5 — 哪些参考可以进入工程研究文档？

**答案：只有 2 条进入正式工程研究文档，另有 3 条仅作线索追踪。**

| Reference ID | 名称 | 可进入的技术问题 | 授权 | 证据状态 |
|---|---|---|---|---|
| **R-C06** | MakeHuman Community | 人体比例控制的**参数维度设计**（身高、头身比、各围度如何参数化） | **CC0**（FAQ 原文："The asset license is CC0"） | 许可页已核验 |
| **R-C02** | Blender 4.2 Manual — Bone Positioning Guide | 骨骼位置 / roll / alignment 的**通用做法** | **网站内容 CC BY 4.0 已核验**；**手册正文许可未核验**（业界通行 CC BY-SA 3.0，本轮两次抓取许可页均被 Cloudflare 拦截，未取得逐字证据）；**该页正文本身也未捕获** | 部分核验 |

`[FACT/VERIFIED]` R-C06 的授权已实际打开 FAQ 页面确认，不是从名称推断。`[PARTIALLY VERIFIED]` R-C02 的**技术正文与手册许可都未取得逐字证据**，因此它只能作"通用做法的线索"，不能作引用出处。

`[RECOMMENDATION]` 进入工程研究文档时必须：

- 标注来源、作者、原始链接、许可、允许用途、禁止用途、对应技术问题（任务书 §六-B 要求）；
- R-C06 特别标注：**只研究其参数维度设计思路，不得导入任何 MakeHuman 资产**（项目硬约束：生产资产必须由自研程序化几何生成）；
- R-C02 只作骨骼通用实践线索，**不是**人体几何标准，也不是可引用的许可来源。

`[REJECTED / UNVERIFIED]` 其余进入工程研究文档的候选 —— `R-C07`（MessyPoly，页面无许可声明）、`R-C08`（basemesh，Cloudflare 拦截）、`R-C09`（bindpose，仅搜索结果摘要）—— 授权无法确认，**不进入**工程研究文档作为依据，仅作线索去追原始出处。

---

### Q-6 — 哪些授权问题仍未确认？

| # | 授权问题 | 影响 | 严重度 |
|---|---|---|---|
| L-1 | R-A01/R-A02/R-A03/R-A04/R-A06/R-A08 的**原始页面许可文本**未取到（被 ad-blocker / 需 JS 渲染 / 抓取失败） | 这些参考能否升级为 Layer A 候选 | 中 —— 它们当前只作观察，授权缺失不阻塞 |
| L-2 | **R-C01/R-C03/R-C04/R-C05 的原始链接与图片本轮未获得**（任务书以"用户提供的图片"指代，图片未随文本进入会话）：Meshy、CG Topology、Renderosity Armpit、mutante28 T-Pose | Layer B 的拓扑/解剖参考几乎为空 | **高** —— Layer B 正是 P1 验收依据 |
| L-3 | **Blender 手册正文的许可与例外条款**（本轮两次抓取 `blender.org/about/license` 均被 Cloudflare 拦截；`docs.blender.org/.../bones/structure.html` 正文也只返回横幅） | R-C02 能否作为可引用的工程依据 | 中 —— 当前仅作线索 |
| L-4 | **是否存在可自由使用的 Canonical 级开放授权人体参考** | 若存在可直接解决 Q-3 的路径 B/C | 中 |
| L-5 | 自有 Canonical Reference Sheet 的**版权归属与对外发布范围**（若项目未来要展示角色） | Q-3 选 B 时的前置条件 | 中 |
| L-6 | 商业参考图在**内部评审文档**中的引用是否需要额外授权（通常"引用说明"与"复制资产"权利不同，但需负责人确认公司口径） | 评审材料的合规性 | 低-中 |
| L-7 | R-C08 / R-C09 的原始页面与许可（Cloudflare 拦截 / 仅搜索摘要） | 是否可从线索升级为工程依据 | 低 |

`[RECOMMENDATION]` L-2 是当前最实质的缺口：**Layer B（解剖与拓扑目标）目前几乎没有可用参考**，而 P1 的三个 Gate 中有两个（TOPO、ANIM）依赖 Layer B。若负责人能提供那 4 组拓扑图片的原始链接，本轮可以立即补做核验并纳入矩阵。

---

### Q-7 — 是否需要调整当前 P1/P2 的验收标准？

**答案：需要，且调整方向已由负责人给出。**

`[FACT/VERIFIED — 负责人指示]` 调整为**五 Gate 分离**（Deliverable 1 §4.2 / Deliverable 4 §2.4）：

| Gate | 应该验证什么 | 不应该用什么代替 |
|---|---|---|
| G-P1-GEO | 解剖比例、曲面连续性、人体体积 | 高质量皮肤贴图 |
| G-P1-TOPO | 肩根、腋下、胯部、关节结构 | 单纯统计面数 |
| G-P1-ANIM | 关节弯曲、体积保持、变形连续性 | 静态 T-Pose |
| G-P2-MOD | 服装、发型、配件独立切换 | 单张穿着完整服装的效果图 |
| G-FINAL-VIS | 整体角色完成度、材质、光照 | 单纯几何测试通过 |

`[RECOMMENDATION]` 对现有 SPEC / 文档的具体调整项：

1. `04-visual-rubric.md` 按 Deliverable 4 替换（当前 §0 增 H-6~H-9、§3 主体改为 8 维度、§3.4 增 V-4~V-7、§5 改为按 Gate 分表签署）；**当前 rubric 的 F/S/B 三组表保留为"看哪里"的导航**。
2. 新增 `05-posture-pose-draft.md`：姿态轴（A-Pose / T-Pose / 动作姿态）+ 5 种图像类型（Clay / Silhouette / Region / Wireframe / Deformation）。
3. SPEC §13 的阈值表按 Gate 分列，避免"几何通过 ⇒ 视觉通过"的误读。
4. SPEC Gate 链是否把五 Gate 写为 GATE-04 之后的细分序列（见 §B Q-08）。

---

## B. 新增待决项（建议追加到 `07-open-issues.md`）

| ID | 问题 | 影响的 Gate / 决定 | 状态 | 负责人填写 |
|---|---|---|---|---|
| **OQ-27** | 五 Gate 分离是否写入 SPEC Gate 链（作为 GATE-04 之后的细分验收序列）？ | SPEC 结构变更 | `OPEN` | ____________ |
| **OQ-28** | Canonical Reference Sheet 走 A（负责人提供）/ B（AI 制作比例与体积关系表）/ C（组合）？ | `G-P1-GEO`、`G-FINAL-VIS` 能否判 PASS | **BLOCKING / OPEN** | ____________ |
| **OQ-29** | 任务书 B 组（Meshy / CG Topology / Renderosity Armpit / mutante28）与 Chibi Boy 的**原始链接** | Layer B 参考是否可核验 | **BLOCKING（对 Layer B）** | ____________ |
| **OQ-30** | A-Pose / T-Pose 的确切姿态定义（手臂下斜角度、bind pose 基准） | `G-P1-ANIM` 的姿态轴协议 | `OPEN` | ____________ |
| **OQ-31** | 相机高度绑定方式：解剖轴（hips→chest 中点）vs 固定世界 Y | 图像协议一致性 | `OPEN`（AI 建议解剖轴） | ____________ |
| **OQ-32** | G-P1-ANIM 的必检动作集（建议：肘屈 90°、肩前屈 90°、髋屈曲 90°、膝屈 90°） | `G-P1-ANIM` 判据 | `OPEN` | ____________ |
| **OQ-33** | 姿态轴实现需要扩展 clay 渲染协议（`applyPose` + `band = none`），是否纳入 AUTH-08 范围 | AUTH-08 范围定义 | `OPEN` | ____________ |
| **OQ-34** | "高品质视觉目标 ≠ 高面数"的**可核验形式**：是否需要在 SPEC 增一条"面数上限"约束或"不得以面数换视觉"的评审规则 | 防止工程目标被静默稀释 | `OPEN` | ____________ |
| **OQ-35** | G-FINAL-VIS 的材质与光照基线由谁定义（当前无任何实机渲染/性能数据） | `G-FINAL-VIS`、`G-FINAL-PERF` | `OPEN` | ____________ |

---

## C. 建议对 `07-open-issues.md` 做的状态更新

`[RECOMMENDATION]` 以下更新**待负责人批准后**才写入项目文档：

| ID | 现状态 | 建议更新 | 理由 |
|---|---|---|---|
| OQ-01 | BLOCKING / OPEN | 保持 BLOCKING，**但范围扩大**为"Canonical Reference Sheet + 三层体系 + 五 Gate"（见 OQ-28/29） | DEC-05 从"给参考图"升级为"给参考体系" |
| OQ-02 | BLOCKING / OPEN | 保持 BLOCKING；新增范围提示：PoC-SM 代码 + 姿态轴协议（OQ-33） | 协议扩展需与 AUTH-08 一起授权 |
| OQ-03 | BLOCKING（负责人已倾向采纳） | **明确为阻塞** | 五个 Gate 分离后，≤5° 明确落在 `G-P1-GEO`，且是合同已有值 |
| OQ-14 | PoC-SM 阻塞验收条件 | 归入 `G-P1-GEO`；同时作为 `G-P1-TOPO` 的肩根结构子项 | 按五 Gate 归位 |
| OQ-15 | R3 暂停投入 | 保持；并补注"其失败是 `G-P1-GEO` 失败，非 `G-P1-TOPO` 失败" | 与五 Gate 对齐，防止误判 R3 的拓扑能力 |
| OQ-17 | 阈值 TBD，需负责人批准后判定 | 拆为两个 Gate：`G-P1-GEO`（区域比例）+ `G-P1-TOPO`（面片级影响） | 按验收对象分离 |
| OQ-19 | G4 前专项完成 | 保持；补注"GPU parity 属 `G-P1-ANIM`，CPU 侧已成立" | 五 Gate 归位 |
| OQ-22..26 | 已关闭 | 保持关闭；OQ-26 补注符号修正已随 `a32251b` 推送并重建 APK | 事实对齐 |

---

## D. 一句话状态

`[FACT/VERIFIED]` 本轮**没有**产生新的技术阻塞项。产生的全部是**输入缺失类**问题：Layer A 的 Canonical 参考（OQ-28）、Layer B 的拓扑参考链接（OQ-29）、以及协议细节（OQ-30~OQ-35）。

`[RECOMMENDATION]` 阻塞优先级排序：

1. **OQ-28 + OQ-29** — 决定 Layer A / Layer B 能否落地，进而决定 `G-P1-GEO` 能否判 PASS；
2. **OQ-02** — 决定 PoC-SM 代码何时能开始；
3. **OQ-03 + OQ-05** — 决定 G3 能否正式签署（DEFER）；
4. OQ-30~OQ-35 — 协议细节，可在 PoC-SM 启动后并行补齐。
