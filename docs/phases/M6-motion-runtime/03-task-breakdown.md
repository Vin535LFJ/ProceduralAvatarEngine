# M6 — 03 任务拆解

| 任务 | 文件范围 | 测试命令 | DoD |
|---|---|---|---|
| T1 PoseFrame/Provider | `native/motion/poseframe/*`, `providers/*` | `pae_motion_tests` | 数据结构与 Provider 测试通过 |
| T2 坐标转换/Retargeting | `native/motion/retargeting/*` | 同上 | 左右映射/镜像/单骨旋转通过 |
| T3 滤波/丢点 | `native/motion/filtering/*`, `MotionRuntime.*` | 同上 | 低置信度/超时渐退通过 |
| T4 Renderer 接入 | `RenderThread.*`, JNI/Kotlin Pose 按钮 | APK + 截图 | 六类动作可见 |
| T5 阶段门 | `docs/phases/M6-motion-runtime/*` | 对照 Exit Criteria | 结论写入 |

T1→T2→T3→T4→T5。
