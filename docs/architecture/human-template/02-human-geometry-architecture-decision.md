# 人体几何架构决策建议（R7）

任务：P1-VR-ARCH-1 / Deliverable R7  
日期：2026-11-26（合并版）  
状态：**建议稿（PROPOSED）**。Gate 判定权在负责人。本文不宣布 PASS，不修改任何已冻结的非目标或历史决策；附带的 ADR-014 是草案。

## 1. 推荐结论

**建议判定：CONDITIONAL GO。**

把 PAE 人体生成的主要几何表示改为 `BaseTopology + ShapeTarget + Rig V2 + Skinning`：以许可证明确的成熟人体四边形拓扑为模板来源，形态由参数和 Shape Target 驱动，骨架与权重由 PAE 自己定义/生成。条件见 §4。

这不是把外部模型当静态成品：基础网格只作为拓扑与形态增量的来源，解析、稳定 ID、区域/Landmark、参数变形、Rig/权重、Canonical/Render Mesh 和 Android 渲染都由 PAE 自己完成。这条路在 host 上已被两条互相独立的实现验证（R6），比继续修补 Sweep/Bridge 拼接更有可持续性；但它与项目现行章程文本冲突，且若干关键项未验证，所以不能是无条件 GO。

## 2. 任务书 15 个问题的回答

| # | 问题 | 回答（证据） |
|---|---|---|
| 1 | MakeHuman Base Mesh 是否适合做 PAE 基础拓扑？ | **适合作为拓扑来源**。13,380 顶点、全四边形、闭合、χ=2、绕向一致、带 UV、97.8% 四价、含手/足/脸独立拓扑，target 之后自相交 0（R2）。不是为 PAE 风格定制：形态中性、足弓浅、无材质分区、无区域标签。 |
| 2 | 是否可以直接复用 CC0 Targets？ | **机制上可以，已验证 26 个 target 文件**：0 基索引、XYZ、加法叠加、±1.0 无翻面、骨架随动。限制：局部性是软的；incr 与 decr 不对称；macro height 不能单独用；其余 1,254 个未验证（R3）。 |
| 3 | 哪些区域需要拓扑修复或补充？ | 无需“修复”。需要**补充**：PAE 区域标注、表面 Landmark、材质分区、手掌/指根与足弓的形态或局部加密（为大幅度手指动作）、三角化剖分规则（细长四边形最小角 3.19°）。26 个 sliver 与价态 3/5 的极点需要逐面定位清单（未做）。 |
| 4 | 哪些形态靠 Target，哪些 PAE 自写 Shape Basis？ | 靠 target：身高/肩宽/胸廓/腰/髋/肢体长度围度/肌肉脂肪/头形。PAE 自写：风格化（`stylization`）、与 target 不对应的字段（biacromial 等）、姿势修正形态（R3 §6）。 |
| 5 | 是否可以自动生成自己的 Rig？ | **已验证**：PAE 能从模板的关节辅助体构建自己的骨架变体（102 / 20 骨）并随形态重解析（R4 §3）。**未验证**：PAE 自己设计关节定义（不依赖源 rig 文件）、bone roll 作为旋转轴、锁骨/肩胛联动。 |
| 6 | 22 骨骼是否应当退役为 LegacyRigV1？ | **建议保留为 LegacyRigV1 并冻结**，新路线用 Rig V2。22 骨在数量上能容纳 20 骨 core，但语义不一一对应，不应改写现有 BoneId（ABI，ADR-002）。 |
| 7 | RigV2 需要多少骨骼、为什么？ | 由动作需求决定：**20 根**覆盖肩/肘/髋/膝/腕/踝的大关节；**+15/侧**用于逐指；扭转骨 +2/侧/肢体用于前臂小腿扭转。建议 VR-1B 先用 20 骨 core（与 MVP 非目标“逐指动作”一致），deform 102 骨作为可选层级（R4 §2）。 |
| 8 | 如何自动生成和压缩权重？ | 第一轮：已许可权重 + Top-4（丢 0.575 %）+ 无权重回退规则（已实现）。fallback：PAE 区域感知逆距离 + 图平滑（需收紧候选骨，当前 Top-4 丢 12.9 %）。不建议为此引入 Biharmonic（R4 §7）。 |
| 9 | 是否应采用完整 Quad Mesh 作内部模板、最终再三角化？ | **是**。四边形是区域选择、加密、细分的基础；模板保持四边形，Render Mesh 阶段三角化，剖分规则单独设计（R5 §3.1）。 |
| 10 | 如何实现 LOD 和稳定顶点对应？ | 未做。可行方向：以四边形环线为细分层级，子级顶点存父面重心坐标；R2 的 `LodCorrespondence` 可迁移。**UNVERIFIED**。 |
| 11 | 如何保持现有 Android 渲染管线可复用？ | RenderThread、资源生命周期、64 B 顶点打包、CPU/GPU skinning 都不需要重写；需要新增：可变骨骼调色板（UBO 分块或纹理）、Rig V2 的 BoneId 映射、模板加载。**全部 NOT RUN**。 |
| 12 | R1/R2 哪些代码仍有价值？ | `CharacterDefinition`/`ResolvedCharacterState`（语义层）、`AnatomicalLandmarks`、`RegionWeightGenerator`（可作为 fallback）、`LodCorrespondence`、`PatchAssembler` 的校验逻辑（可用于模板完整性检查）、拓扑/自相交检查、Sweep/Loft/Tube（服装、附件、局部几何）。不再作为人体皮肤的主生成路径。 |
| 13 | 是否应把主要几何表示改为 BaseTopology + ShapeTarget + Rig + Skinning？ | **建议是**，在 §4 条件满足的前提下。 |
| 14 | 这条路线能否停止 Sweep/Bridge 反复修补？ | 能。肩、腋、髋、手、足不再是拼接问题，而是同一张闭合曲面上的绑定问题。新的问题类别是 LBS 变形质量（corrective、DQS、局部加密），这类问题有成熟解决路径。 |
| 15 | 下一阶段怎样在一个真实、完整的人体模型上验证，而不是再从孤立肩部开始？ | VR-1B 直接以完整人体为单位：模板 → 参数 → Rig V2 → 权重 → 姿势 → Render Mesh → Android，验收对象是全身 7 视角 + 关节近景（R8）。 |

## 3. 决策点的明确建议

| 决策点 | 建议 | 理由 |
|---|---|---|
| 是否正式采用 Base Human Topology | **采用**，前提是 §4 的治理与许可证条件被负责人批准 | host 链路成立（两条独立实现）；完整人体；R1/R2 的拼接缝问题消失 |
| 是否直接复用 CC0 Targets | **采用**，作为低层形态增量 | 需版本锁定、范围/组合规则和极值测试 |
| 是否保留 R2 作为局部几何辅助算法 | **保留**，降级为辅助 | Sweep/Loft/PatchAssembler 用于服装、附件、道具与局部几何；不再是皮肤主路径 |
| 是否需要替换当前 mode-3 Canonical | **VR-1B 验收通过后替换**；验收前 mode-3 保持为历史基线 | 不在未验证时改动生产路径 |
| 是否采用 Rig V2 | **采用，先 20 骨 core** | 覆盖已测的大关节；逐指是 MVP 非目标；102 骨超出 UBO[22] |
| 22 骨是否退役 | 不退役，冻结为 Legacy 与回归基线 | BoneId 0–21 是 ABI |
| 后续 VR-1B 的正式路线 | 见 R8 | — |

## 4. CONDITIONAL GO 的条件

1. **治理条件（最优先）**：项目章程“零外部 3D 美术资产”、ADR-004“不得改回外部模型”、产品范围中“拓扑模板”与“禁止外部人体模型作为运行时依赖”的歧义，必须由负责人通过 ADR-014（草案见下）明确裁定：由 CC0 资产离线编译的 PAE 模板是否算作允许的“拓扑模板”。**未获批准前不得进入 Android 运行时。**
2. **许可证条件**：`default.mhskel` 与 `default_weights.mhw` 的 CC0 依据只是文件内声明，状态为 `PENDING_CONFIRMATION`；需负责人/法务确认。确认前 Rig 与权重按“PAE 自生成”规划（R8 的 D2）。
3. **视觉验收**：负责人亲自查看 R6 的 101 张图（特别是 `closeup-*.png` 与握拳图），确认“更自然”的判断。
4. **遗留缺陷的处置计划**：握拳限制（声明手指动作范围或局部加密）；肩外展 3 倍边拉伸的 corrective/DQS 评估；26 个 sliver 的逐面定位。175 个无权重顶点的回退规则本轮已实现。
5. **口径先于数值**：PAE 自定义测量平面需与 `CharacterDefinition` 的既有口径（ANSUR II p50 比例）对齐，否则参数映射不能用于验收。
6. **Android 路线确认**：可变骨骼调色板方案先做设计评审，再写代码。

如果负责人不批准条件 1，本路线仍然成立为**离线工具链**：用 host 工具把 CC0 模板 + target 转换成 PAE 自有的参数化数据并由 PAE 代码生成——是否算“由代码和参数生成”，同样需要 ADR 给出定义。

## 5. 风险

| 风险 | 缓解 |
|---|---|
| 章程冲突未解除 | 条件 1 不通过则不启动任何运行时工作 |
| 骨架/权重许可证依据不足 | 走 PAE 自建骨架（关节辅助体已在模板里）+ 距离法权重，并优先改进距离法 |
| 中性 Base 不等于目标成人亚洲形象 | 官方 Target + PAE Basis + 负责人视觉验收 |
| Target/Base 版本错配 | hash/topology version 锁定和加载拒绝 |
| Top-4 与纯 LBS 在关节处失真（握拳 714 翻面、肩外展边拉伸 3.2 倍） | 限制动作幅度并如实记录；full-weight 对照；corrective / DQS / 局部加密留到下一阶段 |
| 设备上 LBS 变形质量不达标 | 限制测试姿势幅度并如实记录 |
| 模板体积（4.1 MB）与 Android 内存 | 位移量化到 0.1 mm、int16/int32；Runtime LOD；骨骼 deform 子集；评估 APK 体积影响 |
| 调色板改动触及 RenderThread 生产路径 | 只新增路径，不改 LegacyRigV1 分支；白名单列明 |
| 动作“通过”但方向错误 | 每个动作带方向断言（本轮链 B 的教训）；数值有限不等于正确 |
| 新旧路径回归 | Legacy 与 R1/R2 保留，独立对照 APK |
| 生产范围失控 | VR-1B 白名单和分阶段 Gate |

## 6. 治理状态

本文件是架构决策建议，不修改冻结 ADR、授权文件或项目非目标。负责人批准前：

- 不启动 VR-1B 生产实现；
- 不替换 mode-3 Canonical；
- 不修改 LegacyRigV1 / UBO / packing / JNI / RenderThread；
- 不宣布人体视觉 Gate PASS。

本轮经负责人要求提交到 git 的内容只有：host-only 工具（`tools/human-template-poc/`）、研究/架构文档和证据；没有任何生产路径改动。

---

## 附：ADR-014 草案（PROPOSED，未签署）

**标题**：采用离线编译的人体基础拓扑模板作为 PAE 人体几何的主要表示

**状态**：PROPOSED / 待负责人决定  
**日期**：2026-11-26

### 背景

PAE 现行人体几何由 SDF/Surface Nets、椭球/胶囊、变截面扫掠、面片与桥接从零拼装（ADR-004、ADR-008～011）。P1、R1、R2 三轮证明这些方法能生成局部几何，但在成人肩部、腋窝、手、足、面部持续遇到形态与接缝问题（R1 边界 0 但接缝角 114.5°；R2 边界 32、sliver 56）。P1-VR-ARCH-1 在仓库外验证了另一条路线（R6）：读取 CC0 许可的成熟人体四边形拓扑，用 Shape Target 驱动形态，由 PAE 定义 Rig V2 并绑定权重，在 host 上完成六类基础动作，全部由 PAE 自己的 C++17 实现，两条独立实现互相印证。

### 需要修订的既有文本

| 文本 | 现行内容 | 拟修订 |
|---|---|---|
| `docs/project-charter.md` 硬性约束 | “零外部 3D 美术资产：禁止 GLB/GLTF/FBX/OBJ/DAE…全部几何/骨骼/权重/材质/场景由代码和参数生成” | 增加例外：**许可证允许商业使用与再分发、来源与哈希可追溯、经离线编译为 PAE 自有二进制模板的“拓扑模板与形态目标”属于正式资产来源**；运行时仍然不加载任何 DCC 格式或外部模型文件 |
| `docs/project-charter.md` 硬性约束 | “22 骨版本化骨架” | 保留为 LegacyRigV1；新增“Rig V2：可变骨骼数，骨骼数由动作需求决定” |
| ADR-004 | “不得改回外部模型” | 明确“外部模型”指运行时加载的成品模型；离线编译的拓扑模板不属于此类 |
| `docs/foundation/00-product-scope.md` | “拓扑模板”未定义来源 | 同上例外 |
| `docs/research/parametric-human-modeling/02-…` §3 | MakeHuman 网格/target“不导入” | 该研究记录保留原文；由本 ADR 取代其中“不导入”一行 |

### 决策内容（若批准）

1. 人体皮肤的主要几何表示改为 `PAEHumanTemplate`（BaseTopology + Region + Landmark + ShapeTarget + Rig V2 + Weights + Provenance），格式见 R5。
2. 模板只能由离线编译器从逐文件登记过许可证与 SHA-256 的来源生成；任何 `LICENSE_UNVERIFIED` 或 `PENDING_CONFIRMATION` 的文件不得进入分发用的模板。
3. 运行时不依赖 Blender、MakeHuman、MPFB2、Python 或在线服务；不加载 OBJ/FBX/glTF。
4. LegacyRigV1（22 BoneId、UBO[22]、64 B 打包、JNI）保持冻结；Rig V2 走独立路径。
5. R1/R2 代码与证据保留为历史基线；Sweep/Loft/Patch 降级为服装、附件与局部几何的辅助算法。
6. mode-3 Canonical 在 VR-1B 验收通过前不被替换。

### 不改变的文本

- MVP 非目标（写实人体、毛发/布料物理、逐指动作与表情捕捉…）不变。
- 五个 Gate 的判定条件、AUTH-08 FB-12、OQ-03 不变。
- `AGENT.md` 轮次验收契约不变。

### 后果

- 正面：人体生成从“拼接局部几何”变为“参数驱动一张完整曲面”；面部/手/足具备可扩展拓扑；形态参数有现成的数据基础。
- 负面：引入对第三方数据许可证的长期依赖（CC0，风险低但须存档）；网格 26,756 三角形高于 R2；LBS 变形质量成为新的主要问题；需要新的骨骼调色板路径。
- 风险登记：许可证依据不足（骨架/权重）→ 回退为 PAE 自生成；视觉不达标 → 回退到 R2 辅助路线并保留模板作参考。

### 待负责人填写

- [ ] 批准 / 驳回 / 修改 ADR-014
- [ ] 裁定 `default.mhskel`、`default_weights.mhw` 的可用性
- [ ] 裁定 Gate：GO / CONDITIONAL GO / NO-GO
