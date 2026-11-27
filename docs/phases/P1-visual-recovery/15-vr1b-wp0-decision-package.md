# VR-1B WP0 决策包（PAE-CONSOLIDATION-02 起草，03 落定）

> 日期：2026-11-27（03 轮更新；同日 WP1/WP2 实施并通过负责人真机验收）
> 状态：**WP0 各项已由负责人决定**；**WP1/WP2 = DEVICE_STATIC_RENDER_ACCEPTED**（负责人 2026-11-27 人眼验收，见 16 号报告 §5）；WP2.1（APK 内置模板加载，加法 AAssetManager JNI）已授权；WP3–WP8 仍须另行批准。
> **批准依据（转录，非代签）**：负责人 2026-11-27 会话提交 PAE-CONSOLIDATION-03 计划并指示"按照以上计划执行"；
> 该计划 §二/§十一 批准限定范围的 ADR-014，§十二 决定 Rig/Weights 采用 PAE 自建，§三/§六/§十三 记录基础拓扑验收为 `ACCEPTED_FOR_VR1B`，§十四 授权 WP1/WP2。
> ADR-014 正式文本：`docs/architecture-decisions/ADR-014-offline-compiled-human-base-topology.md`（ACCEPTED，2026-11-27）。
> 依据：R7 架构决策建议、R8 VR-1B 计划、R1 许可证审计、收敛收尾报告。

## 1. ADR-014 正式签署包（已批准 → 正式文本见 ADR-014 文件）

**标题**：采用离线编译的人体基础拓扑模板作为 PAE 人体几何的主要表示
**状态**：**ACCEPTED（2026-11-27）**。正式决策记录与批准范围见 `docs/architecture-decisions/ADR-014-offline-compiled-human-base-topology.md`；本节保留起草时的范围说明供对照。

### 1.1 决策背景

- P1、VR-1A-R1/R2 证明从零拼装（SDF/椭球/扫掠/桥接）能生成局部几何，但成人肩、腋、手、足、面部持续遇到形态与接缝问题（R1 接缝角 114.5°；R2 边界 32、sliver 56）。
- P1-VR-ARCH-1 用两条互相独立的 PAE C++17 实现在 host 上验证了另一条路线：读取 CC0 成熟人体四边形拓扑（13,380 顶点 / 13,378 全四边形、闭合 χ=2）→ Shape Target 改形态 → Rig + Top-4 权重 → 方向正确的基础动作 → 导出 PAE 自有 `.paht` 模板（往返逐字节一致）。65 项测试、101 张图、独立 Clone 重建全部通过。
- 该路线与现行章程文本冲突（见 §1.4），必须由负责人以 ADR 明确裁定，不能静默改变。

### 1.2 批准的资产例外（严格限定）

仅允许以下来源作为**离线模板编译输入**：

| 资产 | 来源 | 许可证依据 | 固定版本 |
|---|---|---|---|
| 基础网格 `makehuman/data/3dobjs/base.obj`（1,749,303 B，SHA-256 `8e761e6624b8f54536409135d1636da63b32486a90d4897f84e121d144f6fb4c`） | MakeHuman 官方仓库 | **强**：`LICENSE.md` §C 列举 + 文件头"explicitly released as CC0 in september 2020" | commit `a8bc2d54ff0ac92e78ff71431b1023eda42bf482` |
| 形态目标 `targets/**/*.target`（1,280 个，全量扫描 1,280/1,280 带 CC0 文件头；本轮实际验证 26 个） | 同上 | **强**：§C 列举 + 逐文件文件头 | 同上 |
| 归档整体 | `codeload.github.com/.../zip/<commit>`，45,251,153 B | SHA-256 `1604d6b35925c8b1c0a7a5e8be8d1f01a53ca8dd80e07dea95c1ba84b192e9b5` | 同上 |

条件：来源与哈希逐文件登记（R1 / `evidence/host-proof/provenance/`）；离线编译为 PAE 自有 `.paht`；**任何 `LICENSE_UNVERIFIED` 或 `PENDING_CONFIRMATION` 文件不得进入分发用模板**。

### 1.3 禁止事项（批准后仍然有效）

1. Android 运行时不加载 OBJ/FBX/glTF/DAE 等任何 DCC 或外部模型格式。
2. 不依赖 MakeHuman、MPFB2、Blender、Python 或在线服务的运行时。
3. 不复制 MakeHuman 的 AGPL/GPL **软件代码**（CC0 仅覆盖数据资产）。
4. 未逐文件核实许可证的第三方资产（含社区下载）不得进入模板。
5. `default.mhskel`、`default_weights.mhw` 在许可裁定前不得进入分发用模板（见 §2）。
6. LegacyRigV1（BoneId 0–21、UBO[22]、64 B 打包、JNI）保持冻结；mode-3 Canonical 在 VR-1B 验收通过前不替换。
7. MVP 非目标不变（写实人体、毛发/布料物理、逐指动作与表情捕捉等）。

### 1.4 必须同步修订的既有文本（拟修订稿，未应用）

历史决策不被静默覆盖：以下修订在 ADR-014 批准时**一并**落笔，原文保留可追溯。

| 文件 | 现行原文（本轮逐字核对） | 拟修订 |
|---|---|---|
| `docs/project-charter.md` 硬性约束 | "零外部 3D 美术资产：禁止 GLB/GLTF/FBX/OBJ/DAE、外部角色/场景贴图、HDRI、预制动画；全部几何/骨骼/权重/材质/场景由代码和参数生成。" | 句末追加例外："**例外（ADR-014）：许可证允许商业使用与再分发、来源与 SHA-256 逐文件可追溯、经离线编译为 PAE 自有二进制模板的拓扑模板与形态目标，属于正式资产来源；运行时仍不加载任何 DCC 格式或外部模型文件。**" |
| `docs/project-charter.md` 硬性约束 | "22 骨版本化骨架" | 改为"22 骨版本化骨架（LegacyRigV1，冻结）；Rig V2 骨骼数由动作需求决定（ADR-014）" |
| `docs/architecture-decisions/ADR-004-procedural-humanoid-geometry.md` 回滚/替代 | "不得改回外部模型。" | 追加："本条所指'外部模型'为运行时加载的成品模型；经 ADR-014 批准、离线编译的拓扑模板不属于此类。" |
| `docs/foundation/00-product-scope.md` 正式资产来源 | "C++ 几何生成函数、人体形态参数、拓扑模板、关节/骨骼规则……禁止 DCC 导出模型……作为运行时依赖。" | 在"拓扑模板"后加注："（来源限定见 ADR-014：逐文件许可证与哈希登记的 CC0 资产经离线编译）" |
| `docs/research/parametric-human-modeling/02-tech-and-license-matrix.md` §3 | MakeHuman 网格/target"不导入" | 研究记录**保留原文**；由 ADR-014 显式取代其中"不导入"一行，并在该文件头部加一行指向 ADR-014 的注记 |

### 1.5 对产品目标的影响

- 正面：人体生成从"拼接局部几何"变为"参数驱动一张完整闭合曲面"；面/手/足具备可扩展拓扑；形态参数有现成数据基础；R1/R2 的接缝问题类别消失。
- 负面/风险：引入对第三方数据许可证的长期依赖（CC0，风险低但须存档）；网格 26,756 三角形高于 R2；LBS 变形质量成为新的主要问题（握拳、肩外展）；需要新的骨骼调色板路径（WP4 之后）。
- 回退：任一条件不满足 → 保持 mode-3/R1/R2 现状，`tools/human-template-poc/` 保留为离线工具链，模板数据不进运行时。

### 1.6 负责人签署项（已决定，转录）

```text
[x] 批准 ADR-014（限定范围，见 ADR-014 文件 §批准范围/§明确不批准）
[x] §1.4 五处修订随批准一并落笔（03 轮已执行，原文保留可追溯）
[x] Gate：CONDITIONAL GO 的条件 1（治理）已解除；其余条件按 R7 §4 继续有效
批准依据：负责人 2026-11-27 会话指示"按照以上计划执行"（PAE-CONSOLIDATION-03 §二/§十一）
记录方式：AI 转录，未代签
```

## 2. Rig 与 Weights 决定（已决定：PAE 自建）

### 2.1 现状

- `default.mhskel`、`default_weights.mhw`：**PENDING_CONFIRMATION**（仅文件内声明 CC0，官方 `LICENSE.md` 未逐项列举）。
- Host 实测：源权重质量最好（Top-4 丢 0.575%）；PAE 距离法权重在肩外展下有一条边拉伸约 10 倍；PAE 热扩散法居中。**PAE 自生成权重尚不如源权重，不能视为生产通过。**
- 握拳约 714 翻面与权重来源无关（基础网格手指环线 + 纯 LBS 的限制）。

### 2.2 建议（与任务书 §9 一致）

**正式采用 PAE 自建 HumanRigV2 Core + PAE 自生成权重**，不把两份待确认文件作为 Android 生产依赖：

- 20 骨 core（肩/肘/髋/膝/腕/踝大关节）；逐指是 MVP 非目标。
- MakeHuman 关节辅助体（属于已核实 CC0 的 `base.obj` 网格数据）仅作 **Host-only 参考**，用于关节位置随形态重解析。
- PAE 自己定义 Bone Hierarchy、Rest Transform、Joint Positions、Bone Roll。
- PAE 自己生成权重 + Top-4 GPU 压缩 + 已实现的无权重回退规则（`kept-ancestor`，175 个体表顶点全部落到 `head`）。
- 独立 Host 测试先行；**肩外展拉伸缺陷必须先改进**（收紧候选骨、图平滑调参），改进结果以方向断言 + 边拉伸指标验收。
- 若负责人/法务后续确认两份文件许可，可重新评估源权重作为对照或过渡，但不改变"PAE 自有 Rig 定义"的方向。

### 2.3 负责人签署项（已决定，转录）

```text
[x] 采用 PAE 自建 RigV2 Core（20 骨）+ 自生成权重；排除 default.mhskel / default_weights.mhw
[ ] 确认两份文件许可后可用 —— 未选择（保持 PENDING_CONFIRMATION，不进分发资产）
批准依据：负责人 2026-11-27 会话指示（PAE-CONSOLIDATION-03 §十二）
记录方式：AI 转录，未代签
```

**执行边界**：当前 Host 自生成权重的肩外展拉伸缺陷未解决，不得标记为生产验收通过；WP1/WP2 静态加载阶段不启用 Rig/Weights；RigV2 动画与生产权重属后续单独 WP。

## 3. 人体视觉基线（ACCEPTED_FOR_VR1B）

- 验收包：`docs/research/human-template-reuse/visual-review/`（18 张精选 + 逐文件 SHA-256 + 逐图清单 + 签署栏）。
- 验收问题：**这个基础人体拓扑是否足以作为后续自然半写实成人的起点**（不是最终质量验收）。
- 四类验证严格区分：Host 软件渲染（已生成）/ 图像模型描述（不可靠，不作数）/ 负责人亲自确认（见下）/ Android 真机（VR-1B WP8，未做）。

**记录（转录，非代签）**：

```text
HUMAN_TEMPLATE_BASELINE = ACCEPTED_FOR_VR1B
依据：负责人既往反馈"这次感觉比较成功了，像个人了"；
     2026-11-27 会话指示按 PAE-CONSOLIDATION-03 §三/§六/§十三 正式记录。
含义：认可基础拓扑作为下一阶段人体生成的模板起点。
不代表：FINAL_HUMAN_VISUAL_GATE = PASS（最终自然半写实成人视觉质量尚未通过）；
       Android GLES3 渲染验证仍为 NOT RUN。
```

18 张图的逐图亲笔签署栏仍保留在验收包 `00-README.md`，供负责人后续补签细节意见；本记录不替代该签署栏。

## 4. VR-1B 第一阶段精确实施白名单（WP1 + WP2，已授权）

目标（一句话）：**把 Host 生成的 `.paht` 模板加载进现有 Android GLES3 引擎，在真机上显示完整静态人体**，验证法线、剔除、视角、内存与资源生命周期。不做动画。

### 4.1 WP1 — Human Template Compiler 产品化（host 工具，不进 APK）

复用现有 29 个 Host 工具文件，整理进 PAE 结构（规划名称，来自 R8 §5.1，授权后才创建）：

```text
native/procedural/human_template/PAEHumanTemplateFormat.h/.cpp
native/procedural/human_template/HumanTemplateCompiler.h/.cpp
native/procedural/human_template/HumanTemplateLoader.h/.cpp
native/procedural/human_template/AnatomicalRegionMap.h/.cpp
native/procedural/human_template/HumanTemplateLandmarks.h/.cpp
tests/unit/human_template_format_tests.cpp
tests/unit/human_template_target_tests.cpp
```

要求：模板读取、写出、校验、往返测试（原型 T12/T13 迁移）；**先解决 D1 时间戳问题**（`.paht` 尾部内嵌 UTC 生成时间导致哈希不可复现，进仓/进 APK 前必须移出内容或固定化）。

### 4.2 WP2 — Android 静态模板加载（只读、静态）

```text
native/procedural/human_template/BaseHumanCanonicalGenerator.h/.cpp
native/procedural/human_template/BaseHumanRenderMeshBuilder.h/.cpp
tests/unit/base_human_canonical_tests.cpp
```

- 只读加载 `.paht`；校验 magic / 版本 / 哈希；provenance 缺失或许可证状态非 `CC0-1.0` 时拒绝。
- 四边形三角化（剖分规则按 D6）→ 静态 Render Mesh → 现有材质/渲染路径显示。
- 多视角、法线、背面剔除、Surface recreation、进程生命周期、内存测试。
- `RenderThread.cpp` 如需接入点，**单独列明并获批准**；默认生产行为不得改变。

### 4.3 本阶段明确禁止

```text
不接入 Rig V2 动画          不修改 LegacyRigV1 / BoneId 0–21
不替换 mode-3 Canonical     不修改 UBO[22]
不修改 64 B GPU 顶点打包    不修改 JNI ABI
不接入未获许可的权重文件    不把 Host-only 工具引入 Android CMake
不做 DQS / corrective / LOD / 逐指动作
```

WP1/WP2 通过后，WP3–WP8（参数、Rig V2、权重、蒙皮、Render Mesh/LOD、完整验收）**另行单独批准**。

## 5. WP0 结果汇总与授权书草案

| # | WP0 项 | 状态 |
|---|---|---|
| 1 | ADR-014 正式状态 | **ACCEPTED**（2026-11-27，正式文本已入库） |
| 2 | Base Mesh 资产许可 | CC0-1.0，强依据（§C + 文件头），固定 commit + SHA-256 |
| 3 | Target 资产许可 | CC0-1.0，1,280/1,280 文件头；已验证 26 个，其余使用前须验证 |
| 4 | Rig/Weights 选择 | **PAE 自建**（§2，已决定）；两份 PENDING 文件排除 |
| 5 | 人体视觉基线 | **ACCEPTED_FOR_VR1B**（§3）；最终视觉 Gate 与 Android 验证未通过/未运行 |
| 6 | VR-1B 精确实施白名单 | §4（WP1/WP2），**已授权** |
| 7 | Git 主工作区 | 独立 Clone（staging 验证通过；正式切换执行记录见 05 报告） |
| 8 | 分支策略 | 保留 main + poc-sm + feature（只读历史）；4 条候选清理（执行记录见 05 报告） |
| 9 | Host proof 可重建证据 | 独立 Clone 重建：65/0、方向 8/8、101/101 图逐字节一致（收尾报告 §3.3） |

### P1-VR-1B 第一阶段授权书（已授权，转录）

```text
授权编号：PAE-CONSOLIDATION-03（负责人 2026-11-27 会话指示）
状态：AUTHORIZED — WP1 + WP2（生产实现尚未开始）
基线分支：poc-sm/shape-model-v2 @ 6a1c29daa009061d4adbaac1f63ef9516c8451b1 及其后继
         （AUTH-08 迁移提交 49d85a89211a77cdaf562dcf585e011466c0e2a4 已叠加）
允许阶段：仅 WP1 + WP2（§4 白名单）
前置条件：[x] ADR-014 签署并同步修订  [x] Rig/Weights 裁定  [x] 视觉基线确认
额外限制：§4.3 全部禁止项；D1 时间戳问题先解决
批准依据：负责人指示"按照以上计划执行"（PAE-CONSOLIDATION-03 §十四）；AI 转录，未代签
```

本授权只覆盖 WP1/WP2，不自动授权 WP3–WP8、替换 mode-3、面部/手部细节、DQS/corrective 或 LOD。