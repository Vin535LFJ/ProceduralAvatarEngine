# 历史设计文档归档索引（Design Document Archive Index）

本目录保存已被新路线取代的**历史设计文档**，仅作追溯与对照，**不是当前开发依据**。
当前有效文档见仓库根目录 [`README.md`](../../README.md) 的“当前设计文档索引”一节。

归档日期：2026-11-23。归档方式：`git mv`，保留 Git 历史。未修改任何文档内容。

## foundation-v1.0 — 初版交付包快照

| 原路径（仓库根） | 现路径 | 说明 |
|---|---|---|
| `Procedural_Character_Foundation_v1.0.zip` | `foundation-v1.0/Procedural_Character_Foundation_v1.0.zip` | v1.0 初版打包快照；解包后的三份正式文档仍保留在仓库根目录（`01_/02_/03_…v1.0.md`），不是历史 |

## v1.1 — M 路线设计包（已被 Anatomy-driven Geometry 提案取代）

| 原路径（仓库根 / docs） | 现路径 | 说明 |
|---|---|---|
| `AI_Implementation_Prompt_v1.1.md` | `v1.1/AI_Implementation_Prompt_v1.1.md` | M 路线 AI 执行提示词，对应 M0–M9 |
| `Procedural_Avatar_Engine_Technical_Spec_v1.0.md` | `v1.1/Procedural_Avatar_Engine_Technical_Spec_v1.0.md` | M 路线系统架构/技术规范 |
| `Procedural_Avatar_Engine_Development_Guidance_Plan_v1.0.md` | `v1.1/Procedural_Avatar_Engine_Development_Guidance_Plan_v1.0.md` | M 路线开发指导计划 |
| `Procedural_Avatar_Engine_Design_Package_v1.1.zip` | `v1.1/Procedural_Avatar_Engine_Design_Package_v1.1.zip` | M 路线设计包原始 zip |
| `README_交付说明.md` | `v1.1/README_交付说明.md` | M 路线交付说明 |
| `docs/AI_Implementation_Prompt_v1.1.md` | `v1.1/docs-copy-AI_Implementation_Prompt_v1.1.md` | 上述文件的 `docs/` 重复副本（内容逐字节相同） |
| `docs/Procedural_Avatar_Engine_Technical_Spec_v1.0.md` | `v1.1/docs-copy-Procedural_Avatar_Engine_Technical_Spec_v1.0.md` | 重复副本 |
| `docs/Procedural_Avatar_Engine_Development_Guidance_Plan_v1.0.md` | `v1.1/docs-copy-Procedural_Avatar_Engine_Development_Guidance_Plan_v1.0.md` | 重复副本 |

> 注：被归档文档内部的相对路径引用仍指向其原位置，属历史原文，未改写。需要引用时以本索引的“现路径”为准。

## 未归档内容（明确保留）

- 仓库根目录 `01_Procedural_Character_Foundation_Specification_v1.0.md`、
  `02_Procedural_Character_Development_Roadmap_v1.0.md`、
  `03_AI_Engineering_Execution_Prompt_v1.0.md`：`docs/project-status.md` 指定的权威基线，保留原位。
- `docs/phases/*`、`docs/foundation/*`、`docs/architecture-decisions/*`、`docs/project-status.md`：各阶段记录与冻结合同，保留原位。
- `docs/proposals/anatomy-driven-geometry-redesign/*`：本次新路线设计包（PROPOSAL，未批准），保留原位。
- `docs/phases/anatomy-driven-geometry-poc/00-implementation-spec.md`：新路线实施 SPEC 草案（DRAFT），保留原位。
- `vedio_demo.mp4`：历史演示视频，非设计文档，保留原位。
