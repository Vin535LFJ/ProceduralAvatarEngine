# Threading and Lifecycle — M1

## 线程时序
```
UI: Activity onCreate → SurfaceView callback → nativeSurfaceCreated
Render: 创建 EGL Display/Context/Surface → 能力检测 → 循环消费命令与绘制
Inference: CameraX ImageAnalysis (KEEP_ONLY_LATEST) → ML Kit → PoseFrame → nativePushPose
Render: 合并最新 Pose → Motion Runtime → BonePalette → UBO → draw
UI: onPause → 暂停相机与推理；SurfaceDestroyed → 停止绘制并销毁 EGLSurface（保留 Context 或按策略销毁）
UI: onResume / SurfaceCreated → 重建 Surface 与 GPU 资源（从 CPU 描述）
```

## 同步规则
- 不在 UI/推理线程调用 GLES；JNI 只投递命令或写 POD 缓冲。
- 实时 Pose 缓冲容量为 1：新帧覆盖旧帧；不使用无界队列。
- 文件视频以 PTS 为唯一时间基准；暂停/拖动/倍速/重播时清空滤波状态并重置时间戳基准。
- `PoseFrame`、`AvatarParameters`、`SceneParameters` 带时间戳或 revision；渲染使用最近一条，不无限外插。
- 所有 Native 资源有明确创建/销毁线程与释放路径；Context Lost 标记 GPU 资源失效。

## 错误处理
- GLES 3.0 创建失败：记录 GL 错误与设备信息，回调 Kotlin 显示“不支持的图形能力”，安全停止渲染，不静默成功。
- Shader 编译/链接失败：日志输出完整源码、编译日志、GL_RENDERER/GL_VERSION。
- 推理异常：`ImageProxy` 在 finally/异常路径关闭；PoseFrame 标记 hasPerson=false，角色渐退到 Idle。
- Surface 销毁后不继续循环渲染；队列中的 SurfaceCreated/Destroyed 按顺序执行。
