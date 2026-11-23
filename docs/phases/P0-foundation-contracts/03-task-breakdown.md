# P0 — 03 Task Breakdown

| 任务 | 文件 | 命令 | DoD |
|---|---|---|---|
| T1 Git 基线 | feature branch, 399967f | git status/log/push | 8 项基线提交并推送，未合并 main |
| T2 契约与 ADR | docs/foundation/*, docs/project-status.md, ADR-010 | 文档审阅 | 六份契约 + 唯一状态文档 + ADR |
| T3 基线复现 | tests/unit/* | cmake/ctest/gradlew | 8/8 ctest + assembleDebug 成功 |
| T4 Shoulder 修复 | Skeleton.cpp + 相关调用点 + tests | ctest + adb 截图 | 红色测试→修复→全绿→真机可见 |
| T5 阶段门 | 本目录 + 索引文件 | 用户确认 | P0 Gate 提交用户 |

依赖：T1 → T2/T3 → T4 → T5。
