# M0 — 03 任务拆解

| 任务 | 前置 | 文件范围 | 测试命令 | DoD |
|---|---|---|---|---|
| M0-T1 仓库与工具链探测 | 无 | `evidence/*` | `adb devices -l`、版本命令 | 输出保存且路径记录 |
| M0-T2 治理文档 | T1 | `docs/*.md`, `docs/requirements/*` | 人工核对 ID 覆盖 | 每条 MVP 需求有 ID/验收 |
| M0-T3 Git 基线 | T1 | `.gitignore` | `git status --short` | 仓库已初始化、无提交 |
| M0-T4 阶段门 | T1–T3 | `04`, `05`, `project-status.md` | 对照 Exit Criteria | PASS/PASS WITH FOLLOW-UPS/BLOCKED |

并行：T2/T3 可并行；T4 依赖全部。
