# M2 — 02 设计

- `EglContextManager`：EGL_DEFAULT_DISPLAY；选择 `EGL_OPENGL_ES3_BIT | EGL_WINDOW_BIT` 配置（深度 24，失败降到 16）；`EGL_CONTEXT_CLIENT_VERSION=3`；RAII 释放 Surface/Context/Display。
- `GlCapabilities`：GL_MAJOR/MINOR、GL_VERSION、GL_RENDERER、GL_VENDOR、GLSL_VERSION、关键限制；`supportsGles3()` 基于主版本。
- `ShaderProgram` / `GpuMesh`：GLSL ES 300；编译失败输出完整源码与 info log；VAO/VBO/IBO RAII；UNSIGNED_INT 索引。
- `RenderThread`：单线程独占 GL；命令队列容量 64（满时丢弃最旧并释放 window 引用）；SurfaceCreated 消费 ANativeWindow 所有权；16ms 渲染节拍；不支持 GLES3 时不绘制。
- `EngineRuntime` / JNI：nativeCreate/nativeSurface*/nativeGetStats/nativeShutdown；Kotlin `NativeBridge` 与 `AvatarSurfaceView` 仅投递回调。

数据流：UI Surface 回调 → JNI 获取 ANativeWindow → 有界队列 → Render Thread → EGL → GLES draw → RenderStats。
