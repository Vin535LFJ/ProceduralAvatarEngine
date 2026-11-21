# M7 — 02 设计

- Kotlin `PoseProviderAdapter`：CameraX `ImageAnalysis` `STRATEGY_KEEP_ONLY_LATEST`，独立 Executor；`InputImage.fromMediaImage`，ML Kit `PoseDetector` STREAM_MODE + accurate model。
- 33 点以像素坐标（x/y）与置信度打包为 float 数组，附 timestampNs / 图像宽高 / rotationDegrees / mirrored 经 JNI 写入 `ExternalPoseProvider`（容量 1）。
- `MotionRuntime` 在 external provider 启用时改用它，否则保持 Synthetic；不引入 ML Kit 类型到 Native。
- 每次推理记录在途帧数、耗时、Pose FPS；`ImageProxy.close()` 在 addOnCompleteListener 中保证执行。
