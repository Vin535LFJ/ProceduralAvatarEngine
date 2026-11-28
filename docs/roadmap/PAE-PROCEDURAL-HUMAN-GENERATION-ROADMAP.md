# PAE Procedural Human Generation — Master Roadmap (SUPERSEDED / 历史快照)

> **已取代（2026-11-28）。** 本文件不再是权威。M1–M8 唯一权威总计划是
> [`PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md`](PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md)
> （PAE-HUMAN-GENERATION-MASTER-PLAN-01）。本文件保留为 WP4 轮次的历史快照，
> 仅供追溯；任何冲突以 MASTER-PLAN 为准。下方原始内容未删改。

Status: `PAE_MASTER_ROADMAP = SUPERSEDED_BY_MASTER_PLAN`
Date: 2026-11-28（原始）
Branch: `poc-sm/shape-model-v2` @ `bfcada75583a690b12f1bf0edd0dad246da104ad`（原始快照基线；现 HEAD 已前进）
Authority: ~~本文件是唯一权威路线图~~ → 权威已移交 MASTER-PLAN。历史提案 `docs/proposals/anatomy-driven-geometry-redesign/07-poc-roadmap.md` 属 R1/R2 时期，仅存档。

最终目标：**自然半写实成人程序化人体生成与实时控制**（Android GLES3，运行时不依赖 Blender / MakeHuman / 在线 AI）。

治理前提（已签署，见 `docs/architecture-decisions/ADR-014-offline-compiled-human-base-topology.md`）：
- 基础拓扑来自固定 commit `a8bc2d54` 的 MakeHuman CC0 资产，离线编译为 PAE 模板；运行时全部几何由代码与参数生成。
- `default.mhskel` / `default_weights.mhw` 许可证仍为 `PENDING_CONFIRMATION`：**只能作 Host 研究参照，不得进入 APK 或成为分发依赖**。生产 Rig 与权重必须 PAE 自有（WP4 契约）。
- LegacyRigV1（BoneId 0–21 / UBO[22] / 64B packing / JNI ABI / mode-3 / mode-4 静态路径）冻结，新人体走独立运行时路径。

---

## 1. 阶段总览与 Gate

| 阶段 | 内容 | 状态 | Gate（独立验收，不得互相顶替） |
|---|---|---|---|
| M0 | Human Template Foundation | **DONE** | 真机静态全身渲染 + 损坏拒绝 + 生命周期 + Legacy 回归（已通过，16/17 号报告） |
| M1 | Human Shape Parameterization（WP3-R2 求解器） | Host 测量完成，求解器 NOT_RUN | 目标尺寸→形态逆求解误差 ≤ 约定阈值；组合非加性被显式处理 |
| M2 | Rig V2 & Skinning Host（WP4） | **本轮启动** | Bind Pose 精确恢复；逐关节区域变形质量报告；Core/Full 对照结论 |
| M3 | Integrated Character Instance | BLOCKED on M1+M2 | 形态+关节+权重+姿态一体化实例；缓存键绑定版本/哈希 |
| M4 | Android Dynamic Human Runtime | NOT_AUTHORIZED | 目标设备能力测试 + CPU/GPU parity + 真机动态渲染 |
| M5 | Procedural Animation & IK | NOT_RUN | 双骨 IK、足部放置、关节约束、程序化 idle/walk |
| M6 | Anatomical Visual Refinement | NOT_RUN | 肌肉/面部/手足细节人眼验收 |
| M7 | Materials / Hair / Clothing | NOT_RUN | 程序化皮肤、头发、服装视觉验收 |
| M8 | Production Quality & Final Visual Gate | PENDING | 多体型×多动作×多设备性能/内存/稳定性 + 负责人最终视觉签署 |

规则：
- Host 测试通过 ≠ Android 生产验收；静态显示 ≠ 动画完成。
- 每阶段 Gate 由负责人签署；AI 只转录，不代签。
- M1 与 M2 并行，M3 汇合；不得互相等待全部完成。

## 2. 并行契约（WP3 ↔ WP4）

共享数据契约（M3 前必须统一）：
- `Stable Canonical Vertex IDs`：OBJ `v` 行序（0 基），永不重排（R1 审计契约）。
- `ShapeParameterVector`：PAE 参数名 → Shape Target 组合（含 macro 与均匀缩放）。
- `ResolvedJointSet`：形态应用后解析的关节 XYZ + 每骨 head/tail + 对称诊断。
- `RigDefinition`：PAE 语义骨骼表（见 `docs/architecture/human-rig-v2/01-rig-v2-architecture.md`）。
- `WeightSet`：每顶点 Top-4 影响 + 归一化 + 来源/版本。
- `PoseDefinition`：Bone Local Transform 输入（语义骨 ID + 局部轴 + 角度）。
- `DeformedMesh`：位置 + 法线 + 稳定 ID 映射。

同步规则：**形态改变 → 关节必须重解析；权重是否重算由缓存误差规则决定；任何缓存必须绑定 BaseMesh / ShapeTarget / RigDefinition / WeightSet 的版本或哈希。** 不得把基础人体权重无条件复用到所有体型。

## 3. 子系统清单（13 项）

每项字段：当前能力 / 已有代码与证据 / 缺失 / 输入→输出 / 上下游 / 阶段 / 测试标准 / Gate / 风险 / 明确不做。

### 3.1 Human Template
- 当前：PAHT v3（payload SHA-256、无时间戳、三次编译逐字节一致），13,380 顶点 / 13,378 quad，Android 真机静态加载渲染 53–56 fps。
- 代码/证据：`native/procedural/human_template/`、`tools/human-template-poc/host/src/wp1_compiler_main.cpp`、`app/src/main/assets/human_template/*.paht`（sha256 `d3bcbbe…417b`）、16/17 号报告。
- 缺失：joint→顶点分组随模板分发（WP4 在 Host 侧先行）；模板内 Rig/权重版本字段与 WP4 契约对齐。
- 输入→输出：CC0 OBJ+targets → `.paht`；`.paht` → Canonical Mesh。
- 上下游：上游 CC0 资产审计（R1）；下游 WP3/WP4/Android。
- 阶段：M0 完成；M3 扩展字段。
- 测试：格式拒绝用例（9 类）、往返逐字节、真机加载哈希一致。
- Gate：M0 已过；字段扩展需新 Gate。
- 风险：PAHT 格式变更影响已冻结 Android 路径 → 只加字段不改布局，或新版本号。
- 不做：运行时读 OBJ/mhskel/mhw。

### 3.2 Human Shape Parameterization（WP3）
- 当前：18 个模板 target 与源资产逐值一致；±1 响应曲线、联动矩阵（affected/meanAll 语义）、截面围度（环分离）、均匀缩放身高 1.55–1.85 m 精确；9 项锚点检查全过。
- 代码/证据：`wp3_measure/quality/experiment/tests`、18/18a/19 号报告、`evidence/vr1b-wp3-host-01/`（含 r1）。
- 缺失：**逆求解器**（目标尺寸→target 组合，组合非加性）；±1 authored 范围钳制策略；更多 CC0 target 引入评估。
- 输入→输出：`ShapeParameterVector`/目标尺寸 → 变形后 Full Geometry。
- 上下游：上游模板；下游 WP4 关节解析、M3。
- 阶段：M1（WP3-R2）。
- 测试：求解误差、0 翻面/0 自交、确定性、锚点不变性。
- Gate：M1。
- 风险：macro height 单独使用 NOT VERIFIED（比例漂移）→ 生产用均匀缩放；退化判定绝对阈值（native，白名单外）需尺度相对化。
- 不做：Android 形态运行时（WP3_ANDROID = NOT_AUTHORIZED）。

### 3.3 Anthropometry Measurement
- 当前：围度/宽度/深度截面协议（环分离、躯干环判定 63/63 确认）、身高/肢段测量。
- 代码/证据：`wp3_measure.*`、`ht_measure.*`、19 号报告。
- 缺失：与 ANSUR 验收口径对齐（参考人群差异 1.6659 vs 1.7549 已记录）；测量不确定度。
- 输入→输出：DeformedMesh → 尺寸表。
- 上下游：WP3 求解器反馈、M8 验收。
- 阶段：M1 并行完善。
- 测试：单位盒解析解、截面闭合、左右对称。
- Gate：随 M1。
- 风险：A-pose 手臂环污染（已修）；口径漂移。
- 不做：医学级人体测量认证。

### 3.4 Human Rig Definition（WP4-A）
- 当前：Host 研究参照 102 deform / 20 core（源自 PENDING mhskel，仅研究用）。
- 代码/证据：`ht_rig.*`、`ht_proof.*`、04/05 号研究文档。
- 缺失：**PAE 自有语义 RigDefinition**（本轮交付）：稳定语义 ID、PAE 层级、rest head/tail、局部坐标架/roll、变形分组、profile/version。
- 输入→输出：RigDefinition 表 + ResolvedJointSet → 可蒙皮骨架。
- 上下游：上游 CC0 joint 辅助组；下游权重/蒙皮/Android palette 映射。
- 阶段：M2（本轮）。
- 测试：语义 ID 唯一、无环、无零长骨、关节随形态、L/R 对称。
- Gate：M2。
- 风险：PENDING 文件不得入分发链；语义 ID 与 GPU palette 索引解耦。
- 不做：改 LegacyRigV1 / BoneId 0–21。

### 3.5 Joint / Landmark Resolution（WP4-A）
- 当前：`resolveJoints` 在变形后表面重解析（依赖 mhskel 顶点引用，研究用）。
- 缺失：纯 CC0 解析器（joint 组质心 + PAE 锚点规则），Root Transform 数学关系显式化。
- 输入→输出：Full Geometry（形态后）+ Landmark 规则 → ResolvedJointSet + 诊断。
- 上下游：WP3 形态 → 本模块 → 权重。
- 阶段：M2。
- 测试：身高缩放/各 target 下关节跟随、骨长一致、无未解析关节。
- Gate：M2。
- 风险：几何缩放锚点 ≠ Root Joint（R1 已修锚点，WP4 显式区分）。
- 不做：运行时依赖 PENDING 骨架文件。

### 3.6 Skin Weight Generation（WP4-B）
- 当前：PAE Distance / Heat 生成器 + Top-4 压缩 + 回退规则（kept-ancestor/nearest-bone）。
- 代码/证据：`ht_rig.cpp`、65 项 host 测试、05 号报告。
- 缺失：Region 约束权重、跨侧污染/空间有效性/平滑度诊断、每形态重算策略。
- 输入→输出：当前形态 Rig + Canonical Mesh → Full/Top-4 WeightSet + 热力图 + 异常报告。
- 上下游：上游关节；下游蒙皮。
- 阶段：M2。
- 测试：Top-4 合法、归一化、无未绑定、左右不互污、区域连续。
- Gate：M2（`SKIN_WEIGHT_QUALITY`）。
- 风险：“全部有权重”≠绑定正确（胸腹绑 head 的反例）；禁止 PENDING mhw 作生产权重。
- 不做：Biharmonic/神经网络权重（本轮）。

### 3.7 Skeletal Deformation（WP4-C/D/E）
- 当前：LBS 位置蒙皮 + 世界轴姿态原型；已知缺陷：握拳 ~700 翻面、肩外展边拉伸 ~3×。
- 缺失：Bone Local Transform 姿态、法线蒙皮、Bind Pose 精确恢复证明、逐关节区域质量、姿态套件。
- 输入→输出：Rest Mesh + WeightSet + PoseDefinition → DeformedMesh + 质量报告。
- 上下游：上游权重；下游 M3/M4。
- 阶段：M2。
- 测试：Bind 恢复误差 0；每姿态 7 类图 + 逐区域指标；方向断言。
- Gate：M2（`POSE_QUALITY`）。
- 风险：手指环线少 + 纯 LBS 的握拳翻面是基础网格限制 → 触发停止条件时如实报告，不扩权。
- 不做：DQS / Corrective / 肌肉模拟（第一轮）。

### 3.8 Animation / IK
- 当前：无（静态姿态验证属 WP4）。
- 缺失：双骨 IK、关节约束、足部放置、平衡、程序化 idle/walk/run。
- 阶段：M5。Gate：M5。风险：在变形质量未达标前做动画会掩盖几何问题。
- 不做：本轮任何动画控制器。

### 3.9 Canonical / Render Mesh
- 当前：quad→tri（0-2 对角）26,756 三角形，面积加权法线，Android 64B 静态路径。
- 缺失：动态路径顶点布局（bone index/weight 属性）、LOD、细分。
- 阶段：M4/M6。Gate：随 M4。
- 不做：本轮改 GPU 布局。

### 3.10 Android Runtime
- 当前：mode-4 静态模板 + AAssetManager 加载（WP2.1），Legacy 冻结。
- 缺失：Rig V2 独立动态路径、palette 容量设备测试、CPU/GPU parity、形态/权重缓存。
- 阶段：M4（NOT_AUTHORIZED）。Gate：目标设备能力 + 真机动态验收。
- 风险：102 骨 palette 不得假定全设备可用；UBO 限制需实测。
- 不做：本轮构建 APK / 部署设备。

### 3.11 Procedural Materials
- 当前：Clay/法线/线框证据渲染；无生产皮肤材质。
- 缺失：Albedo/Roughness/Normal/SSS 近似/Triplanar。
- 阶段：M7。不做：用材质掩盖几何/变形缺陷。

### 3.12 Hair / Clothing
- 当前：无。阶段：M7。不做：本轮。

### 3.13 Visual Quality & Performance Validation
- 当前：Host 质量检查器（拓扑/自交/翻面/边拉伸/体积）、真机 fps 静态。
- 缺失：逐关节区域阈值体系、多设备性能/内存、最终人眼 Gate。
- 阶段：贯穿 M2–M8。规则：阈值由基准姿态与实测分布制定，不得为 PASS 临时放宽。
- 不做：以图像描述模型替代人眼验收。

## 4. 本轮（PAE-HUMAN-GENERATION-PLAN-01 + WP4-HOST-RIG-01）范围

交付：本路线图、Rig V2 架构、WP4 Host 计划与原型代码、证据、综合报告。
白名单：`tools/human-template-poc/host/**`、`docs/roadmap/**`、`docs/architecture/human-rig-v2/**`、`docs/phases/P1-visual-recovery/wp4-host-rig-01/**`。
禁止：app/**、native/renderer|runtime|procedural/human_template/**、PAHT v3、JNI、LegacyRigV1、UBO[22]、64B packing、mode-3/4；不构建 APK、不部署、不自动 commit/push。
停止条件与最终状态字段见 WP4 计划（`wp4-host-rig-01/01-wp4-host-rig-plan.md`）。

## 5. 未解决风险（全局）

1. 握拳/手指变形受基础网格环线密度限制（已知 ~700 翻面）——可能需要 M6 几何细化或局部 corrective，而非权重调参。
2. 肩外展 LBS 体积损失/边拉伸——DQS 评估在 LBS 基准建立之后。
3. PENDING 许可证文件：若最终无法确认，PAE 自有 Rig/权重路线（本轮）是唯一分发路径。
4. 组合形态非加性 → M1 求解器必须迭代/查表，不能线性叠加。
5. 退化判定绝对阈值（native）尺度相关 → 需白名单授权后修改。
6. 归档 `_PAE-Repository-Archive` 清单过期（不含 WP1–WP3 成果），需单独刷新。