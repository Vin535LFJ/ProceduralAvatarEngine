# PAE Project History

权威来源：`docs/architecture-decisions/ADR-014-offline-compiled-human-base-topology.md`、`docs/phases/P1-visual-recovery/` 历史报告。本文件只梳理演进脉络，不替代权威。

## 早期几何探索（R1/R2 及 PoC，历史对照，非当前主线）

探索过 Primitive / Ellipsoid / Capsule / Sweep / Loft / Surface Patch / SDF / Smooth Union / Surface Nets / Bridge / Seam / 局部融合。

证明的能力：局部生成、连接、连续拓扑的部分可行性。

暴露的问题（已确认）：
- 多部件放进同一 MeshData ≠ 几何融合。
- 单一连通组件 ≠ 自然人体。
- 全身 SDF 不自动产生正确面部/手足。
- Primitive 拼接易成人偶/机器人轮廓。
- 局部 Sweep/Bridge 不能独立承担完整自然人体。
- 拓扑正确 ≠ 关节变形自然。

纪律：R1/R2 保留为历史研究与对照证据，**不自动恢复为当前主线**，不把全身 Primitive/SDF 当最终人体主线。

## ADR-014 后的正式路线（当前）

许可证/来源确认的 CC0 Base Topology + Shape Targets → 离线编译 → PAE 自有 PAHT 模板 → 程序化形态/骨骼/权重/变形 → Android 运行时。

MakeHuman 官方 Base Mesh 与逐文件验证的 CC0 Targets 仅在 ADR-014 范围内离线编译。不得引入 MakeHuman 软件代码、MPFB2 代码或未审核第三方数据。`default.mhskel`/`default_weights.mhw` 仍 PENDING_CONFIRMATION。

## 已积累的关键里程碑（详见 current-project-state.md 与 STATUS）

M0 模板基础（PAHT v3 + Android 静态）→ M1 形态参数化（WP3 测量 + WP3-R2 逆求解）→ M2 骨骼蒙皮（Rig V2 + CPU LBS + 坐标审计）→ M6 手指细化负结果。

## 路线调整纪律

任何路线调整必须基于实际证据 + 正式决策（ADR 提案 → 负责人批准）。不得静默恢复旧主线，不得把研究建议自动升级为架构决定。