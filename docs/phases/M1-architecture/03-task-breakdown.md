# M1 — 03 任务拆解

| 任务 | 文件范围 | 测试命令 | DoD |
|---|---|---|---|
| T1 架构与 ADR | `docs/architecture/*`, `docs/architecture-decisions/*` | 文档审阅 | 接口/线程/坐标明确 |
| T2 构建骨架 | `settings.gradle.kts`, `build.gradle.kts`, `gradle.properties`, `app/*`, `gradle/wrapper/*` | `./gradlew :app:assembleDebug --no-daemon` | BUILD SUCCESSFUL |
| T3 宿主核心与测试 | `native/CMakeLists.txt`, `native/core/*`, `tests/*` | `ctest --test-dir build/host-cygwin --output-on-failure` | 100% 通过 |
| T4 阶段门 | `docs/phases/M1-architecture/*`, `docs/project-status.md` | 对照 Exit Criteria | 结论写入 |

接口冻结后 T2/T3 可并行；T4 依赖全部。
