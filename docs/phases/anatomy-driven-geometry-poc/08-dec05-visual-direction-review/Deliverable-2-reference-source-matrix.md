# Deliverable 2 — Reference Source Matrix

> 状态：**草案 / DRAFT**。所有"推荐状态"均为 AI 建议，不是负责人批准。
> 证据等级按本轮实查记录。`UNVERIFIED` 表示无法打开原始页面或无法确认具体作品/授权，**不根据名称推断视觉内容**。

## 说明

- 用途层级：**A** = Final Visual Target；**B** = Anatomical & Topology Target；**C** = Modular Appearance Target。
- 授权类型只记录**实际查到的**，不推断。商业预览图 ≠ 使用权；网页可浏览 ≠ 可复制/修改/再发布；公开展示 ≠ CC0。
- "用户提供的图片"在本轮任务书中只有文字描述，图片未随文本进入本会话，相关条目一律 `UNVERIFIED`。

### 2.0 三条通用证伪规则（负责人本轮追加）

`[FACT/VERIFIED]` 以下规则适用于**矩阵中每一条**，避免"看起来好 = 适合本项目"的滑坡：

| # | 规则 | 依据 |
|---|---|---|
| **E-1** | **外部成品模型不能证明本项目的生成能力。** 商业模型是"人手工做出来的成品"，无论多精细，都不构成"程序化系统能产出它"的证据 | 负责人指示（TurboSquid Young Man 的处理方式即为范例） |
| **E-2** | **外部成品模型不能证明目标平台的实时性能。** 商品页的 PBR / 高面数 / 绑定信息与低端 Android 的实时约束无关 | 负责人指示（TurboSquid 案例） |
| **E-3** | **商品页描述不等于技术验证。** "带绑定、带 PBR、带贴图"是**商品属性声明**，不是拓扑质量或动画质量的独立证据 | 负责人证据纪律 + §11.1 同源规则 |

`[RECOMMENDATION]` 因此本矩阵中每条参考的"技术用途"列都单独声明它**不能**证明什么。当前唯一同时满足"授权已核验 + 明确不可用于生产"的参考是 **R-A07（Sketchfab CC BY 4.0）**，它也仅限观察。

---

## A. 人物整体外观与高品质 3D 风格

| Reference ID | 名称 | 原始链接 | 作者/发布者 | 用途层级 | 视觉用途 | 技术用途 | 授权类型 | 授权证据 | 使用限制 | 推荐状态 |
|---|---|---|---|---|---|---|---|---|---|---|
| R-A01 | Young Man / Cartoon Man Rigged (id 2151499) | https://www.turbosquid.com/3d-models/3d-model-young-man-rigged-2151499 | `UNVERIFIED`（页面仅显示 JS/ad-blocker 提示，无法核验发布者） | A（风格质量参照） | **可作风格化角色质量参照**（负责人指示）——观察整体造型与完成度 | **不能证明人体拓扑质量**；**不能证明低端 Android 实时性能**；**不能证明程序化生成能力**（商品页描述其为已绑定、带 PBR 与贴图的**成品商业模型**） | `UNVERIFIED` | 页面无可见许可文本（抓取被拦截）；负责人转述商品页含 rigged / PBR / texture | 不得导入/复制/提取贴图/骨骼/BlendShape；不得把商业成品当作自研几何能力证明 | **CONDITIONAL — 仅 Layer A 风格观察；拓扑/性能/能力三不作证** |
| R-A02 | Cartoon Family 2 — Mother Father Son Daughter | https://www.cgtrader.com/3d-models/character/woman/cartoon-family-2-mother-father-son-daughter | Characterstore（CGTrader 页面可核验，63 reviews） | C（非 A） | 角色家族风格、角色组配置 | 65k polys / 70k verts；CAT rig；64 blendshapes；角色身高 135/100/160 cm | 商业商品许可（页面未展示具体许可全文） | 页面显示售价 $328、格式 MAX/FBX/OBJ | 商品预览图不等于使用权；不可导入本项目 | **CONDITIONAL（仅 C 层观察）/ REJECT（A 层：比例偏卡通、非自然成人）** |
| R-A03 | David Rig | https://www.davidrig.com/ | `UNVERIFIED`（抓取失败） | A/B（候选） | 无法核验 | 无法核验 | `UNVERIFIED` | 无 | 不得据此推断作品内容 | **UNVERIFIED → 不采用，除非负责人提供可核验页面/作品名** |
| R-A04 | Maxine "Max" Alvarez — Character Life Simulation | https://www.jabali.ai/game/ff09579a-.../maxine-max-alvarez/ | Роман Бородин（Jabali AI 页面可核验） | C（仅外观切换 UX） | 页面描述可选择服装、地点、剧情 | 页面描述的是 AI 角色互动游戏，非几何/拓扑资料 | `UNVERIFIED`（未找到素材许可） | 仅"About/How to play"文字 | 不能当作人体几何标准；不得导出素材 | **CONDITIONAL（C 层 UX 观察）/ REJECT（A/B 层）** |
| R-A05 | DeepMotion | https://www.deepmotion.com/ | DeepMotion（产品站，可核验） | 非参考层 | 无具体人体几何可核验 | AI 动作捕捉 / Text-to-3D Animation 产品能力 | 软件/服务条款，非图像许可 | 页面为产品与案例文字 | 网站品牌形象不是人体几何标准 | **REJECT 作为几何/视觉参考；可作技术背景阅读** |
| R-A06 | Realistic MetaHuman Characters — KB Kookmin Bank (2022) | https://www.artstation.com/artwork/DLJ52e | MINO ENTERTAINMENT（ArtStation 页面可核验；client Salin） | A/B（候选，非 Canonical） | 写实数字人男女角色、完整制作管线描述 | 概念→建模→绑定→动画全流程；外包工作室作品说明 | `UNVERIFIED`（页面无许可声明） | 页面文字："male and female digital humans... full pipeline" | 不可导入；MetaHuman 依赖外部生态，与本项目生产约束冲突 | **CONDITIONAL（仅观察写实完成度）/ NOT Canonical** |
| R-A07 | Male human character turnaround wearing casual | https://sketchfab.com/3d-models/male-human-character-turnaround-wearing-casual-fb7647d1a10140d3a8ff1f3e9cb6cafb | 0C557H512（Sketchfab 页面可核验） | A/B（候选） | turnaround 展示；149.8k tris / 74.9k verts | Rodin Gen-1 生成模型，非程序化人体方法 | **CC BY 4.0（页面明确显示）** | 页面："License: CC Attribution Creative Commons Attribution" | 需署名；不可当作项目生成能力证明；不可导入生产 | **CONDITIONAL（唯一授权已核验的 turnaround 参考）** |
| R-A08 | Stylized male streetwear character turnaround | https://sketchfab.com/3d-models/stylized-male-streetwear-character-turnaround-74ca17c5eb0848c3912aaa8a34bac9d7 | alesyeah15（页面可核验） | A（候选） | 风格化男性 turnaround；500k tris / 250k verts | Rodin Gen-1 生成 | `UNVERIFIED`（页面未显示 license） | 无 | 高面数、AI 生成，与项目约束不符 | **UNVERIFIED / REJECT 作为 Canonical** |

## B. 模块化外观与服装

| Reference ID | 名称 | 原始链接 | 作者/发布者 | 用途层级 | 视觉用途 | 技术用途 | 授权类型 | 授权证据 | 使用限制 | 推荐状态 |
|---|---|---|---|---|---|---|---|---|---|---|
| R-B01 | Modular Chibi Boy \| Fab | 用户提供的图片与原始商品页（本轮未附图片/链接） | `UNVERIFIED` | C（仅模块组织） | 仅观察服装/发型/配件切换组织方式 | 模块边界与遮挡 | `UNVERIFIED` | 无 | **不得作为人体比例标准** | **CONDITIONAL（C 层组织方式）/ REJECT（比例）** |
| R-B02 | Cartoon Family 2 | 同 R-A02 | Characterstore | C | 角色家族配置 | 角色组与服饰 | 商业商品 | 同 R-A02 | 不可导入 | **CONDITIONAL** |
| R-B03 | REV WORLDS — Avatar Fashion | https://rev-worlds.com/ | `UNVERIFIED`（抓取失败） | C | 无法核验 | 无法核验 | `UNVERIFIED` | 无 | 不得推断内容 | **UNVERIFIED → 需负责人提供可访问页面** |
| R-B04 | Character Creator (CC5) | https://www.reallusion.com/character-creator/ | Reallusion（页面可核验） | C（仅系统能力观察） | 写实/风格化角色、发型与服装切换 | 3D Hair & Beard / 3D Outfit & Accessory / Cloth auto-conform / CC5 70k poly vs Game Base 10k | 软件许可（页面仅说明软件购买/更新，**未说明预览图使用许可**） | 页面文字："change hairstyles, dress characters for animation tests" | 不可导入素材；预览图使用需另行确认 | **CONDITIONAL（C 层系统能力参考）** |
| R-B05 | Maxine 外观切换 | 同 R-A04 | Роман Бородин | C | 页面描述服装/地点选择 | 非几何资料 | `UNVERIFIED` | 同 R-A04 | 同上 | **CONDITIONAL** |

## C. 人体比例、姿态与解剖结构

| Reference ID | 名称 | 原始链接 | 作者/发布者 | 用途层级 | 视觉用途 | 技术用途 | 授权类型 | 授权证据 | 使用限制 | 推荐状态 |
|---|---|---|---|---|---|---|---|---|---|---|
| R-C01 | Meshy — Skinny Woman Wearing Pants | 用户提供的图片/页面（本轮未附） | `UNVERIFIED` | B（候选） | 无法核验 | 无法核验 | `UNVERIFIED` | 无 | 不得作为比例标准 | **UNVERIFIED** |
| R-C02 | Blender 4.2 LTS Manual — Bone Positioning Guide | https://docs.blender.org/manual/en/4.2/animation/armatures/bones/structure.html | Blender Foundation（站点可核验；该页正文抓取仅返回横幅） | B | 不适用 | 骨骼位置/层级/roll 的通用指南（**正文未核验**） | Blender **网站**内容 CC BY 4.0（`blender.org/about/license` 已核验："Most of the blender website is available as Creative Commons Attribution"）；**手册正文许可未核验**（业界通行说法为 CC BY-SA 3.0，但本轮两次抓取该许可页均被 Cloudflare 拦截，未取得逐字证据）；**手册个别例外未核验** | https://www.blender.org/about/license/ | 仅作文字性工程参考；不得当作本项目骨骼 ABI 依据 | **CONDITIONAL（PARTIALLY VERIFIED）** |
| R-C03 | CG — Topology | 用户提供的拓扑图片与来源（本轮未附） | `UNVERIFIED` | B | 无法核验 | 无法核验 | `UNVERIFIED` | 无 | 不得推断 | **UNVERIFIED** |
| R-C04 | Renderosity — Armpit Topology | 用户提供的图片与来源（本轮未附） | `UNVERIFIED` | B | 无法核验 | 无法核验 | `UNVERIFIED` | 无 | 不得推断 | **UNVERIFIED** |
| R-C05 | 3D Male Model T Pose — mutante28 | 用户提供的 DeviantArt 页面（本轮未附链接） | `UNVERIFIED` | B | 无法核验 | 无法核验 | `UNVERIFIED` | 无 | 不得推断 | **UNVERIFIED** |
| R-C06 | MakeHuman Community / MakeHuman 1.3.0 | https://static.makehumancommunity.org/ 与 FAQ | MakeHuman Community（静态站可核验） | B（工程研究） | 参数化人体比例/体型控制 | 开源人体生成器；可作为比例与 morph 目标研究 | **CC0（FAQ 明确："The asset license is CC0"）** | https://static.makehumancommunity.org/makehuman/faq/are_makehuman_files_free.html | 仅作工程研究；**不得导入为生产资产**（项目硬约束） | **ACCEPT（B 层工程研究，非生产资产）** |
| R-C07 | Intermediate Quad Retopology: Edge Flow, Poles, and Useful Density | https://www.messypoly.com/learn/quad-retopology-deformation-edge-flow | MessyPoly（页面可核验，更新 2026-07-16） | B | 不适用 | 以动作测试定义拓扑：elbow 0–135°、shoulder elevation 160°、knee 145°；引用 ETH Zurich Animation-Aware Quadrangulation 与 Pixar OpenSubdiv | `UNVERIFIED`（页面无 license 声明） | 页面无许可文本 | 仅作工程文字参考；不可复制图文 | **CONDITIONAL（B 层工程研究）** |
| R-C08 | Rigging-Friendly Base Mesh Checklist | https://basemesh.pro/blogs/blog-rigging-checklist.html | `UNVERIFIED`（Cloudflare 拦截） | B | 无法核验原始页面 | 搜索摘要提到肩部菱形腋窝流、scapula、A/T-pose bind pose | `UNVERIFIED` | 搜索摘要，非原始页面 | 不得当作完整证据 | **UNVERIFIED（仅搜索摘要）** |
| R-C09 | What do riggers need from a model | http://bindpose.com/what-riggers-need-from-model | `UNVERIFIED`（仅搜索结果摘要） | B | 不适用 | edge flow / quads / resolution 的一般性原则 | `UNVERIFIED` | 搜索摘要 | 同上 | **UNVERIFIED（仅搜索摘要）** |

---

## 关键缺口（必须报告，不得凑数）

1. `[FACT/VERIFIED]` **没有任何一条候选被核核验为 Layer A 的 Canonical Acceptance Target**：能打开的页面要么授权不明、要么风格与 High-quality Stylized Realism 冲突、要么是 AI 生成/商业预览。
2. `[FACT/VERIFIED]` 唯一授权明确可核验的角色 turnaround 是 R-A07（CC BY 4.0），但它是 Rodin Gen-1 生成、149.8k 三角面，只能作为**视觉观察**，不能作为项目生成能力或工程拓扑标准。
3. `[FACT/VERIFIED]` 任务书中说的"用户提供的图片"（Chibi Boy / Meshy / CG Topology / Armpit Topology / mutante28）本轮未随文本进入会话，全部 `UNVERIFIED`，需负责人重新附上原图与原始链接。
4. `[RECOMMENDATION]` 按任务书第六节三条路径处理缺口：(1) 使用合法授权的商业参考图仅作视觉观察；(2) 负责人提供自有参考图；(3) 制作自有 Canonical Reference Sheet。优先 (3) + (2)。
