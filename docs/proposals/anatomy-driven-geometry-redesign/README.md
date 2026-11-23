# ProceduralAvatarEngine — Anatomy-driven Geometry Redesign

**研究设计包 v1.0 · 2026-11-23**

本目录是对 `Vin535LFJ/ProceduralAvatarEngine` 的远程仓库审计、人体形态建模研究与目标架构建议。它是**设计提案，不是已批准的实施规格**。

## 基线与适用范围

- 审查分支：`feature/procedural-character-foundation`
- 审查 HEAD：`486a00d8e9c5534d53480370be70015bc9c2f158`
- `main` / `m12-unified-human-geometry`：`c539afbec299ed4a066429ea09f9d3d36e67f74d`
- `m11-anatomy-visual-rebuild`：`67ccb3940950623b1a24d6b3e843750c5c4a99b3`
- 指定基线在远程 feature branch 仍是 HEAD。远程比较显示 feature 相对 main 有 6 个独有提交；本地工作区状态无法通过 GitHub 远程 API 验证。

## 阅读顺序

1. [00-current-state-audit.md](00-current-state-audit.md) — Git、代码、历史路线与证据真实性
2. [01-visual-shape-requirements.md](01-visual-shape-requirements.md) — 目标人体比例、区域形态及视觉评价
3. [02-anatomical-shape-model.md](02-anatomical-shape-model.md) — Anatomy/Shape 数据契约
4. [03-geometry-representation-study.md](03-geometry-representation-study.md) — 几何表示研究与评分矩阵
5. [04-shoulder-and-joint-study.md](04-shoulder-and-joint-study.md) — 肩部、腋下、肘部的专门设计
6. [05-topology-and-rigging-design.md](05-topology-and-rigging-design.md) — 动画拓扑、Rig-aware 与 Skinning
7. [06-target-architecture.md](06-target-architecture.md) — C++17 / Android NDK / GLES3 架构
8. [07-poc-roadmap.md](07-poc-roadmap.md) — PoC 阶段、视觉 Gate 和工程 Gate
9. [08-risk-and-decision-register.md](08-risk-and-decision-register.md) — 风险、假设与 OPEN 决策
10. [09-final-recommendation.md](09-final-recommendation.md) — 八个核心问题的答复与最终建议

## 状态语义

- **FACT**：由指定提交的代码、Git 元数据或历史证据文档直接支持。
- **REPORTED**：仓库报告记录了结果，但本次没有重新执行对应测试/设备采集。
- **PROPOSAL**：本提案建议的架构或验收方案，尚未实施。
- **OPEN**：需要项目负责人明确决定，不能视为批准。
- **NOT VERIFIED**：缺少可复核证据。

## 边界声明

本设计包不包含源代码变更、不包含 P1-B 实施、不提交、不推送、不合并。远程 GitHub 仓库未被写入；这些文件以可直接放入仓库的相对路径组织。仓库中的 PNG 文件名和文字报告已审计，但由于本次连接环境不能读取 GitHub 仓库二进制图片，本次**未完成像素级 PNG 视觉复核**。视觉缺陷结论只引用项目方 Gate 判定及已归档的缺陷报告，不冒充亲自看过截图。

