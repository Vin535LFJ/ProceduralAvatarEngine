# M2 — 00 范围与假设

- 范围：Kotlin Surface ↔ JNI ↔ C++ EGL/GLES3 Context；Render Thread 与有界命令队列；能力检测、Shader/VBO/VAO 封装、RenderStats；程序化三角形+立方体；Surface 重建与 Context 恢复路径。
- 非目标：人体几何、骨骼蒙皮、姿态输入（M3+）。
- 假设：设备声明 GLES 3.2，但以 Native EGL 创建结果与 GL_VERSION 字符串为准；SurfaceView 回调发生在 UI 线程，仅投递命令。
