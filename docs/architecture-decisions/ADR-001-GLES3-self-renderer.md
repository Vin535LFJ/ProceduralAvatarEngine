# ADR-001：自研 EGL + OpenGL ES 3.0 Renderer

- 日期：2026-11-21
- 状态：accepted（架构基线，用户不可擅改）
- 背景：MVP 需在中低端 Android 手机和电视盒子上实时渲染程序化数字人。
- 候选：(A) 自研 GLES3 专用渲染器；(B) Filament/Unity/Unreal/Godot/Three.js；(C) GLES2 兼容管线。
- 决定：采用 (A)。C++ Native 管理 EGL Display/Context/Surface，Render Thread 独占 GL；GLES 3.0 为最低目标，运行时检测；不做 GLES2 降级。
- 理由：专用管线可控 Draw Call 与 Shader 成本，避免大型引擎包体与未知驱动行为；GLES3 提供 UBO/整数顶点属性，适合骨骼蒙皮。
- 影响：需自行实现 Shader/资源/生命周期管理；不支持设备必须明确报错。
- 回滚/替代：若产品后续要求 GLES2，需独立立项设计兼容管线，不得在本管线隐式降级。
