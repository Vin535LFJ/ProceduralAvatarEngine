# M4 — 03 任务拆解

| 任务 | 文件范围 | 测试命令 | DoD |
|---|---|---|---|
| T1 骨架与 IBM | `native/skeleton/Skeleton.h/.cpp` | `pae_humanoid_tests` | 层级/IBM/矩阵顺序通过 |
| T2 参数化人体 | `native/procedural/humanoid/Humanoid.h/.cpp` | `pae_humanoid_tests` | 网格合法、确定性 |
| T3 自动权重与热力图 | 同上 | 单测 | 权重和/影响数/有效性 |
| T4 Renderer 显示 | `native/renderer/RenderThread.*` | APK + 截图 | 人形可见 |
| T5 阶段门 | `docs/phases/M4-humanoid-skeleton/*` | 对照 Exit Criteria | 结论写入 |

T1→T2→T3→T4→T5。
