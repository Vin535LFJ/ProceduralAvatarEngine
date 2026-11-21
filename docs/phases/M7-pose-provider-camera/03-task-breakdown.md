# M7 — 03 任务拆解

| 任务 | 文件范围 | 测试命令 | DoD |
|---|---|---|---|
| T1 依赖与权限 | `app/build.gradle.kts`, Manifest | Gradle 构建 | 依赖解析、权限声明 |
| T2 ExternalPoseProvider + JNI | `native/motion/providers/*`, `jni_bridge.cpp`, `EngineRuntime.*`, `RenderThread.*` | ctest + 构建 | 容量1、线程安全 |
| T3 CameraX + ML Kit Adapter | `app/.../PoseProviderAdapter.kt`, `MainActivity.kt` | 真机日志/截图 | PoseFrame 推送成功 |
| T4 统计与阶段门 | `docs/phases/M7-pose-provider-camera/*` | 对照 Exit Criteria | 结论写入 |

T1→T2→T3→T4。
