# M1 — 04 验证与验收

| 验收条件 | 结果 | 证据 |
|---|---|---|
| 工程在当前环境构建 | PASS | `evidence/gradle-assemble-debug-local.txt`：`BUILD SUCCESSFUL in 49s`，33 tasks；`evidence/gradle-exit-local.txt` = 0 |
| Gradle wrapper 生成 | PASS（本地发行版 workaround） | `evidence/wrapper-generation.txt`；官方 URL 下载 10s 超时见 `gradle-assemble-debug-utf8.txt`；将缓存 7.6.1 发行版放入 wrapper 期望目录后成功 |
| 宿主单测 | PASS | `evidence/ctest-cygwin.txt`：5 cases, 100% passed |
| 公共接口/坐标/线程明确 | PASS | `docs/architecture/*` 与 ADR-001…006 |
| 模块依赖无环 | PASS | 依赖单向：宿主→适配器→Runtime→Renderer/Core；Core 不依赖 GL/SDK |

未验证：GLES3 Context 创建与绘制（M2）、真机运行（M2 起）。
