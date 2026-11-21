# M8 — 03 任务拆解

| 任务 | 文件范围 | 测试命令 | DoD |
|---|---|---|---|
| T1 Native reset 通道 + 单测 | `native/renderer/RenderThread.{h,cpp}`, `native/runtime/EngineRuntime.{h,cpp}`, `app/src/main/cpp/jni_bridge.cpp`, `app/src/main/java/com/pae/host/NativeBridge.kt`, `tests/unit/motion_tests.cpp` | `ctest`（宿主）+ Gradle 构建 | reset 语义单测通过；JNI 通道编译通过 |
| T2 VideoPoseSource 解码管线 | 新建 `app/src/main/java/com/pae/host/VideoPoseSource.kt` | Gradle 构建 + 真机日志 | 解码→推理→PoseFrame(PTS)→JNI 闭环；异常路径资源释放 |
| T3 播放状态机 + UI | `VideoPoseSource.kt`, `MainActivity.kt` | 真机操作 | 播放/暂停/拖动/倍速/重播/停止全部可用且触发 resetMotion |
| T4 真机验证 + 文档 | `docs/phases/M8-video-input/*` | 对照 Exit Criteria | 04/05 报告 + evidence + RTM/状态更新 |

T1→T2→T3→T4；T2 与 T3 内聚在同文件不并行。

**测试素材策略**：真机无可用人像素材时，验证解码/PTS/状态机/降级行为（合成测试视频或设备端生成）；"真人姿态驱动"状态如实标注。测试视频经 adb 推送到 /sdcard/Download，由用户经 UI 选择，不打包进 APK。
