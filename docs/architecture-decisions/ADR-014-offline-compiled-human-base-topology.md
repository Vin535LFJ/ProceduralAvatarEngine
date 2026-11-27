# ADR-014 — 离线编译的人体基础拓扑模板作为 PAE 人体几何主要表示

> **状态：ACCEPTED**（2026-11-27）。
> 本 ADR 是资产政策与几何表示的正式决策；实施范围与授权见
> `docs/phases/P1-visual-recovery/15-vr1b-wp0-decision-package.md`。
> 草案沿革：P1-VR-ARCH-1 R7 附录（2026-11-26，PROPOSED）→ 本正式版本。

## 决策记录（DECISION RECORD）

| 字段 | 值 |
|---|---|
| 决策项 | 是否允许许可证明确的 MakeHuman CC0 基础网格与形态目标，经离线编译成为 PAE 人体模板的来源 |
| 决策结果 | **ACCEPTED** —— 仅限下文 §批准范围 的严格限定例外 |
| 决策人 | 项目负责人 |
| 决策日期 | **2026-11-27** |
| 决策来源 | 负责人在会话中提交 PAE-CONSOLIDATION-03 计划并指示："按照以上计划执行"。该计划 §二 明确"ADR-014：我建议批准"并列出批准/不批准范围，§十一 要求"按负责人已确认的方向准备正式版本……必须作为正式决策修订，不要静默覆盖旧文本" |
| 记录方式 | AI **仅作转录与归档**；未代签，未添加负责人未表达的判断。批准范围以计划 §二/§十一 原文为限 |

同轮关联决定（同一指示批准，分别记录于 §决定 第 7 条与 15 号决策包）：

- **Rig/Weights**：不使用 `PENDING_CONFIRMATION` 的 `default.mhskel`、`default_weights.mhw`；PAE 自建 HumanRigV2（第一版 20-bone core）与自生成权重（计划 §十二）。
- **人体基础模板视觉基线**：`HUMAN_TEMPLATE_BASELINE = ACCEPTED_FOR_VR1B`（计划 §三/§六/§十三；依据负责人既往反馈"这次感觉比较成功了，像个人了"与本轮指示）。**不是** `FINAL_HUMAN_VISUAL_GATE = PASS`。

## 背景

PAE 现行人体几何由 SDF/Surface Nets、椭球/胶囊、变截面扫掠、面片与桥接从零拼装（ADR-004、ADR-008～011）。P1、VR-1A-R1/R2 三轮证明这些方法能生成局部几何，但在成人肩部、腋窝、手、足、面部持续遇到形态与接缝问题（R1 接缝角 114.5°；R2 边界 32、sliver 56）。

P1-VR-ARCH-1 用两条互相独立的 PAE C++17 实现在 host 上验证了另一条路线（R6）：读取 CC0 成熟人体四边形拓扑（13,380 顶点 / 13,378 全四边形、闭合 χ=2、绕向一致、带 UV）→ Shape Target 改形态 → Rig + Top-4 权重 → 方向正确的基础动作（每个动作带方向断言）→ 导出 PAE 自有 `.paht` 模板（写读往返逐字节一致）。65 项测试通过；101 张证据图在独立 Clone 重建后逐字节一致。

该路线与章程"零外部 3D 美术资产"等现行文本冲突，必须由负责人以 ADR 明确裁定。本 ADR 即该裁定。

## 批准范围（严格限定）

| # | 批准的是 |
|---|---|
| A-1 | MakeHuman 官方 `makehuman/data/3dobjs/base.obj`（1,749,303 B，SHA-256 `8e761e6624b8f54536409135d1636da63b32486a90d4897f84e121d144f6fb4c`）作为拓扑来源。许可依据：官方 `LICENSE.md` §C 列举 + 文件头"explicitly released as CC0 in september 2020" |
| A-2 | 官方 `targets/**/*.target` 中**逐文件确认带 CC0 文件头**的形态目标（固定 commit 全量扫描 1,280/1,280 命中；本轮已验证 26 个，其余使用前须按 R3 口径验证） |
| A-3 | 来源锁定：MakeHuman commit `a8bc2d54ff0ac92e78ff71431b1023eda42bf482`；归档 zip 45,251,153 B，SHA-256 `1604d6b35925c8b1c0a7a5e8be8d1f01a53ca8dd80e07dea95c1ba84b192e9b5`；逐文件 Git blob / SHA-256 登记于 R1 与 `evidence/host-proof/provenance/` |
| A-4 | 上述资产**只允许离线编译**为 PAE 自有 `.paht` 二进制模板；模板解析、形态应用、Rig、Weights、Canonical/Render Mesh 与 Android 运行时全部由 PAE 自己的代码实现 |
| A-5 | 人体皮肤的主要几何表示改为 `PAEHumanTemplate`（BaseTopology + Region + Landmark + ShapeTarget + Rig V2 + Weights + Provenance），格式见 R5；四边形内部拓扑保留，Render Mesh 阶段三角化 |

## 明确不批准的事项

| # | 不批准 | 理由 |
|---|---|---|
| N-1 | MakeHuman / MPFB2 的 AGPL/GPL **软件代码**进入 PAE | CC0 仅覆盖数据资产 |
| N-2 | 未逐文件核实许可证的社区模型或第三方资产 | 来源不可追溯 |
| N-3 | Android 运行时加载 OBJ/FBX/GLB/DAE 等任何 DCC 或外部模型文件 | 运行时资产政策不变 |
| N-4 | 依赖 MakeHuman、Blender、MPFB2、Python 或在线服务的运行时 | 离线编译边界 |
| N-5 | 未经批准的训练数据、网络模型及其他第三方形态资源 | 超出本次批准范围 |
| N-6 | `LICENSE_UNVERIFIED` 或 `PENDING_CONFIRMATION` 文件进入分发用模板 | 含 `default.mhskel`、`default_weights.mhw`（见 §决定 第 7 条） |
| N-7 | 以本 ADR 为由跳过任何 Gate 判定或宣布视觉 PASS | Gate 判定权在负责人 |

## 决定

1. 人体皮肤主要几何表示改为 `PAEHumanTemplate`（A-5）。
2. 模板只能由离线编译器从逐文件登记过许可证与 SHA-256 的来源生成（A-1～A-4）。
3. 运行时不依赖任何外部 DCC 软件或模型格式（N-3/N-4）。
4. LegacyRigV1（22 BoneId、UBO[22]、64 B 打包、JNI）保持冻结；Rig V2 走独立路径。
5. R1/R2 代码与证据保留为历史基线；Sweep/Loft/Patch 降级为服装、附件与局部几何的辅助算法。
6. mode-3 Canonical 在 VR-1B 验收通过前不被替换。
7. **Rig/Weights 来源**：排除 `default.mhskel`、`default_weights.mhw`（`PENDING_CONFIRMATION`）；采用 PAE 自定义 HumanRigV2（第一版 20-bone core，PAE 自定义 Rest Transform / Bone Hierarchy / 关节位置）与 PAE 自生成权重（Top-4 为首个兼容 GPU 输出目标）。MakeHuman 关节辅助体（属已核实 CC0 的 `base.obj` 数据）仅作 Host-only 参考。**当前 Host 自生成权重的肩外展拉伸缺陷未解决，不得标记为生产验收通过**；RigV2 动画与生产权重属后续单独 WP。
8. **视觉基线**：`HUMAN_TEMPLATE_BASELINE = ACCEPTED_FOR_VR1B` —— 基础拓扑可作为下一阶段模板起点；最终自然半写实成人视觉 Gate 与 Android GLES3 渲染验证仍为未通过/未运行。

## 同步修订的既有文本（保留历史，不静默覆盖）

| 文件 | 修订方式 |
|---|---|
| `docs/project-charter.md` | "零外部 3D 美术资产"句追加限定例外（指向本 ADR）；"22 骨版本化骨架"注明 LegacyRigV1 冻结 + Rig V2 |
| `docs/architecture-decisions/ADR-004-procedural-humanoid-geometry.md` | "不得改回外部模型"追加解释：指运行时加载的成品模型；离线编译拓扑模板不属此类 |
| `docs/foundation/00-product-scope.md` | "拓扑模板"加注来源限定（指向本 ADR） |
| `docs/research/parametric-human-modeling/02-tech-and-license-matrix.md` | 原文保留；头部加注：§3"不导入"一行由本 ADR 取代 |
| `docs/ADR_INDEX.md` | 登记本 ADR |

## 与既有治理的关系

- MVP 非目标（写实人体、毛发/布料物理、逐指动作与表情捕捉等）不变。
- 五个 Gate 判定条件、AUTH-08 FB-12、OQ-03、`AGENT.md` 轮次验收契约不变。
- 本 ADR 不构成任何 Gate 的 PASS；WP1/WP2 只是静态全身加载验证，不等于完整 VR-1B 完成。

## 后果

- 正面：人体生成从"拼接局部几何"变为"参数驱动一张完整闭合曲面"；面/手/足具备可扩展拓扑；形态参数有现成数据基础。
- 负面/风险：对第三方数据许可证的长期依赖（CC0，风险低但须存档）；网格 26,756 三角形高于 R2；LBS 变形质量成为新的主要问题（握拳约 714 翻面、肩外展约 3 倍边拉伸）；需要新的骨骼调色板路径（后续 WP）。
- 回退：任一条件不满足 → 保持 mode-3/R1/R2 现状，`tools/human-template-poc/` 保留为离线工具链，模板数据不进运行时。

## 验证

1. Host 链路已由两条独立实现验证（R6）；独立 Clone 重建：65/0 测试、方向 8/8、101/101 图逐字节一致（收敛收尾报告 §3.3）。
2. 资产哈希与许可证逐文件登记可复核（R1、`evidence/host-proof/provenance/`、`SHA256SUMS.txt`）。
3. Android 侧全部 NOT RUN；第一个设备证据将在 WP2 产生。

## 落地文件清单

| 文件 | 角色 |
|---|---|
| 本文件 | 正式决策记录 |
| `docs/project-charter.md`、ADR-004、`docs/foundation/00-product-scope.md`、`02-tech-and-license-matrix.md` | 同步修订（见上表） |
| `docs/ADR_INDEX.md` | 登记 |
| `docs/phases/P1-visual-recovery/15-vr1b-wp0-decision-package.md` | WP0 决策包与 WP1/WP2 授权范围 |
| `docs/research/human-template-reuse/visual-review/` | 视觉基线验收对象（18 张） |