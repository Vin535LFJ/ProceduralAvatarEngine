# M2 — 03 任务拆解

| 任务 | 文件范围 | 测试命令 | DoD |
|---|---|---|---|
| T1 EGL/能力检测 | `native/renderer/egl/*`, `gl/GlCapabilities.*` | 真机 logcat | GL 字符串与版本记录 |
| T2 Shader/Mesh/Stats | `native/renderer/gl/{ShaderProgram,GpuMesh}.*`, `stats/*` | APK 构建日志 | 无编译/链接错误 |
| T3 RenderThread + JNI | `native/renderer/RenderThread.*`, `runtime/*`, `app/src/main/cpp/jni_bridge.cpp` | `./gradlew :app:assembleDebug` | 构建成功 |
| T4 Kotlin Surface | `app/src/main/java/com/pae/host/{NativeBridge,AvatarSurfaceView,MainActivity}.kt` | adb 安装/启动/logcat/截图 | 画面可见 |
| T5 阶段门 | `docs/phases/M2-gles-renderer/*` | 对照 Exit Criteria | 结论与证据 |

T1/T2 可并行，T3 依赖二者接口，T4 依赖 T3。
