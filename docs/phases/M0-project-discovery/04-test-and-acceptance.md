# M0 — 04 验证与验收

| 验收条件 | 结果 | 证据 |
|---|---|---|
| 仓库结构/可构建性有事实记录 | 通过（初始无工程；构建基线已决策） | `evidence/toolchain.txt`、`repository-audit.md` |
| 平台/工具链确认，未知项标注 | 通过 | `evidence/toolchain.txt`、`device.txt`、risks R-02/R-03/R-07 |
| MVP 范围、硬约束、非目标清晰 | 通过 | `project-charter.md`、`requirements-baseline.md` |
| 每项 MVP 需求有 ID 与验收条件 | 通过 | `requirements-baseline.md`（23 项） |
| Git 初始化且无提交 | 通过 | `git status --short` 显示未跟踪文件；无 commit |

未验证（非 M0 门槛）：Native GLES Context、摄像头取帧、性能数据。详见 `project-status.md`。
