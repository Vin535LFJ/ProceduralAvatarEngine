# M3 — 03 任务拆解

| 任务 | 文件范围 | 测试命令 | DoD |
|---|---|---|---|
| T1 MeshData + Validator | `native/procedural/MeshData.h`, `MeshValidator.*` | `ctest` | 合法性检查通过 |
| T2 图元 | `native/procedural/primitives/*` | `ctest` | 确定性/边界测试通过 |
| T3 Loft/Sweep/PTF | `native/procedural/surfaces/*` | `ctest` | 无扭转跳变、法线正确 |
| T4 Renderer 集成 | `native/renderer/RenderThread.*` | APK 构建+真机截图 | 生成几何可见 |
| T5 阶段门 | `docs/phases/M3-geometry-core/*` | 对照 Exit Criteria | 结论写入 |

T1→T2/T3→T4→T5。
