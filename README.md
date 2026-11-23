# Procedural Character Foundation v1.0

本仓库包含三份冻结的正式工程文档（v1.0 基线，位于仓库根目录）：

1. `01_Procedural_Character_Foundation_Specification_v1.0.md` — 技术设计规范：定义架构、几何主线、骨骼/坐标/蒙皮契约、数据结构、性能和验收标准。
2. `02_Procedural_Character_Development_Roadmap_v1.0.md` — 开发指导计划：定义 P0–P5 阶段、需求/设计/实施/验证/验收闭环。
3. `03_AI_Engineering_Execution_Prompt_v1.0.md` — AI 工程执行提示词：指导 Agent 按规范与计划审计和实施，避免一次性扩张范围。

## 当前设计文档索引（2026-11-23 更新）

| 状态 | 文档 | 说明 |
|---|---|---|
| 当前基线（冻结） | `01_Procedural_Character_Foundation_Specification_v1.0.md` | P0 冻结的架构/骨架/网格/蒙皮/视觉/线程契约 |
| 当前基线（冻结） | `02_Procedural_Character_Development_Roadmap_v1.0.md` | P0–P5 路线图 |
| 当前基线（冻结） | `03_AI_Engineering_Execution_Prompt_v1.0.md` | AI 工程执行提示词 |
| 新路线设计提案（PROPOSAL，未批准） | `ProceduralAvatarEngine-Anatomy-driven-Geometry-Redesign-v1.0.zip` | Anatomy-driven Geometry 设计包原始 zip |
| 新路线设计提案（PROPOSAL，未批准） | `docs/proposals/anatomy-driven-geometry-redesign/` | 上述 zip 解压后的 README + 00–09 共 11 份文档 |
| 新路线开发总提示词 | `ProceduralAvatarEngine-Anatomy-driven-Geometry-Development-Prompt-v1.0.md` | 新路线的开发闸门与流程约束 |
| 新路线实施规格（DRAFT，未批准） | `docs/phases/anatomy-driven-geometry-poc/00-implementation-spec.md` | 第一阶段仓库/源码审查、D-01…D-10 OPEN 决策、G0/A 规格草案 |
| 历史文档 | [`docs/archive/README.md`](docs/archive/README.md) | 已归档的 M 路线 v1.1 / 初版 zip 快照索引；仅作追溯，不作开发依据 |

> 重要：Anatomy-driven Geometry 相关文件状态为 PROPOSAL / DRAFT，**未获批准**，不取代冻结的 01/02/03 与
> `docs/foundation/*`、`docs/architecture-decisions/*`。P1-A 视觉 Gate 仍为 REJECTED。

## 使用顺序

1. 先由项目负责人审阅三份文档，确认技术决策。
2. 将三份 Markdown 一起提供给具备仓库读写和终端能力的 AI Agent。
3. 按执行提示词从 P0 开始，不要跳过仓库审计和契约冻结。
4. P1-A 几何 PoC 必须在 Android GLES Renderer 中给出真实多视角证据，先由项目负责人确认视觉方向，再扩展完整人体。
5. P1 通过后，再进入 P2 换装；P4 才接入 ML Kit。

## 仓库审查基线

文档编写参考 `Vin535LFJ/ProceduralAvatarEngine` 的 `main` 分支提交：

`c539afbec299ed4a066429ea09f9d3d36e67f74d`

AI 执行时必须重新获取最新仓库 SHA，并核对现有代码；不得假定该提交仍是最新。

## 重要原则

- 底层保留 Saltline-inspired procedural asset generation。
- 上层采用 Modular Avatar 组织角色组件。
- P1 主几何路线是解剖控制点 + 参数化 Surface Patch + Topology Template + 非圆形可变截面 Sweep。
- SDF 保留为局部辅助/研究工具，不作为 P1 最终动画人体网格的唯一生产方式。
- 不使用外部 3D 美术资产或完整游戏引擎。
- AI 自评、编译成功或 CTest 全绿不能替代视觉 Gate 和 Android 真机验证。
