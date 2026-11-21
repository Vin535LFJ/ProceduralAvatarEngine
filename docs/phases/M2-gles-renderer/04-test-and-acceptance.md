# M2 — 04 验证与验收

| 验收条件 | 结果 | 证据 |
|---|---|---|
| 真机绘制 Native GLES 画面 | PASS | `evidence/frame.png`、`frame-recreated.png`：深色背景 + 彩色三角形 + 旋转立方体 |
| GL 字符串与限制记录 | PASS | `evidence/logcat-recreate.txt`：`GL_VERSION=OpenGL ES 3.2 ...`、`GL_RENDERER=Mali-T860`、`GL_VENDOR=ARM`、`GLSL ES 3.20`、MAX_VERTEX_UNIFORM_BLOCKS=12 / MAX_TEXTURE_SIZE=8192 / MAX_VERTEX_ATTRIBS=16 |
| GLES 3.0 Context 真实创建 | PASS | `EGL initialized version 1.4` → `GLES 3.0 EGL context created` → shader 编译成功 → surface ready |
| Shader 错误完整日志 | PASS（错误路径已触发并验证） | 首次因 `#version` 前换行失败，日志输出完整源码与编译日志；修复后成功 |
| Surface 重建不崩溃 | PASS | HOME → 再次启动：`surfaceDestroyed` → `EGLSurface destroyed` → `surfaceCreated` → 重新查询能力 → `surface ready` → 继续绘制 |
| GL 仅 Render Thread | PASS | JNI 只投递命令；`ANativeWindow_fromSurface` 在 UI 线程获取引用，EGL/GL 调用位于 PID 21299/TID 21315 |
| RenderStats | PASS | `frames=240 fps=56.2 frame_ms=17.81 draw_calls=2 triangles=13`；重建后继续累计到 360 帧 |
| 构建 | PASS | `evidence/gradle-assemble.txt`：BUILD SUCCESSFUL；`gradle-exit.txt` = 0 |

未验证：真机摄像头与姿态输入（M7）；视频输入（M8）；长时间稳定性（M9）。
