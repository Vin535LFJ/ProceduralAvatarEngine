# ADR Index

权威 ADR 位于 `docs/architecture-decisions/`。

| ADR | 主题 | 状态 |
|---|---|---|
| ADR-001 | GLES3 自研 Renderer | Accepted |
| ADR-002 | 坐标系统与 Skeleton | Accepted（P0 复核中） |
| ADR-003 | Pose Provider / ML Kit | Accepted（P4 复核坐标/镜像语义） |
| ADR-004 | 程序化 Humanoid 几何 | Accepted（历史 M 路线） |
| ADR-005 | GPU Skinning UBO | Accepted |
| ADR-006 | 视频时间同步 | Accepted（P5 复核 PlaybackEpoch） |
| ADR-007 | 文件选择器回退 | Accepted |
| ADR-008 | 人体几何选型 | 部分被 ADR-010 取代 |
| ADR-009 | 统一 SDF 上半身 | 在 P1 最终人体唯一主线范围内被 ADR-010 取代；SDF 仍可做局部辅助研究 |
| ADR-010 | P1 正式人体主线 | Accepted（待用户 P0 Gate 确认） |
| ADR-011 | CharacterDefinition 分层 Shape Model 契约 | **Accepted**（2026-11-24；仅批准契约结构 + 冻结边界 F-1..F-6，不构成任何 Gate 的 PASS） |
| ADR-012 | Saltline 程序化资源生成方法独立模块研究 | **Accepted — Research Only**（2026-11-24；不引入 Three.js / 网页运行时；非 PoC-SM 强制前置） |
| ADR-013 | AI 开发轮次的验收契约 | **Accepted**（2026-11-24；每轮必须交代码 + 视觉证据 + 验收结果，缺一不计工程里程碑；权威正文 `AGENT.md`） |

详见 `docs/architecture-decisions/ADR-010-p1-surface-patch-mainline.md`。
ADR-011 / ADR-012 见 `docs/architecture-decisions/ADR-011-shape-model-contract.md`、
`docs/architecture-decisions/ADR-012-saltline-module-study.md`。

> **决策记录归档**：`docs/phases/anatomy-driven-geometry-poc/10-decision-records/`
> 收录 2026-11-24 签署的 ADR-011 / ADR-012、OQ-03（≤5° 硬条件与测量口径）、
> OQ-05（G3 DEFER）、AUTH-08（授权书）五份决策记录。
> 2026-11-23 的 `.draft.md` 版本已由 Git 历史保留（提交 `1ce1a21`）。
