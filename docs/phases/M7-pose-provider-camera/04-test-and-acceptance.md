# M7 — 04 测试与验收

- 日期：2026-11-21
- 设备：`MAX0019111000104`（nunai_max TV 盒子，Android 8.1 / API 27，arm64-v8a，GLES 3.2）
- 构建：`./gradlew :app:assembleDebug` → BUILD SUCCESSFUL；APK `app/build/outputs/apk/debug/app-debug.apk`

## 验收结果

| 验收项 | 结果 | 证据 |
|---|---|---|
| ML Kit SDK→PoseFrame→JNI→MotionRuntime 链路 | **PASS**（静态图替代验证） | `logcat-m7-final.txt`：`pose stats: inference_ms=238.79 confidence=0.00 landmarks=0`；`static-image done: 241.74 ms` |
| 摄像头不可用时的容错 | **PASS** | `logcat-bind-fallback.txt`：FRONT/BACK/ANY 选择器逐一尝试，最终 `no camera resolved on this device; synthetic provider stays active`，无崩溃 |
| 无人/低置信度降级 | **PASS** | 程序化测试图（无人）→ landmarks=0 → 角色平滑回中性站姿（`frame-after-static-image.png`） |
| 渲染连续性（推理不阻塞渲染线程） | **PASS** | 外部帧推送后 PAE_STATS 持续 `fps≈48.5-49.6`（`logcat-bind-fallback.txt`） |
| 合成模式恢复（最后来源生效） | **PASS** | `frame-synthetic-restored.png`、`pose-t1.png`、`pose-t2.png`：Pose 按钮后角色保持抬臂合成动作 |
| CameraX KEEP_ONLY_LATEST / ImageProxy 全路径关闭 | **实现核对**（代码审查 + 构建） | `PoseProviderAdapter.kt`：`STRATEGY_KEEP_ONLY_LATEST`；`addOnCompleteListener { proxy.close() }`。真机实时帧路径因设备无摄像头**未验证** |
| 推理统计（耗时/在途帧/Pose FPS/置信度） | **PASS**（静态图路径） | `logcat-static-image.txt`：inference_ms、in_flight、pose_fps、confidence 均有输出 |
| 运行时权限流程 | **PASS** | 存储权限 `pm grant` + 运行时请求代码路径；相机权限沿用 M7 原实现（真机弹窗路径未再触发——无摄像头设备 CameraX 初始化即失败） |

## 未验证事项（如实记录）

1. **真实摄像头实时推理（FUN-IN-001 真机项）**：目标盒子无可用摄像头（`No available camera can be found`，三选择器均失败）。替代验证（静态图经同一检测→打包→JNI 链路）已通过；带真机的摄像头端到端验证**未验证**，复现步骤：接入带摄像头的 Android 设备 → 安装 APK → 授予相机权限 → 点击 Camera → 抬手/屈肘观察角色跟随。
2. **真人姿态驱动端到端**：当前环境无法获取含真人的图片/视频素材（主机网络不可用），测试输入为程序化图案（无人）。ML Kit 对真人关键点的输出质量、镜像/旋转在真机上的表现**未验证**。
3. ML Kit STREAM_MODE 连续帧推理（区别于单帧）未在真机验证。

## 修复的问题

- 点击 TestImg 时 `ActivityNotFoundException: OPEN_DOCUMENT`——目标盒子无系统文件选择器。修复：SAF 失败后回退到应用内存储浏览器（`StoragePickerDialog`），需 `READ_EXTERNAL_STORAGE` 运行时权限。该问题同样影响 M8，已按可复用组件实现。
- 一次性外部姿态帧后合成模式永久失效。修复：`RenderThread` 的 `SetPoseMode` 命令显式 `setExternalEnabled(false)`（最后来源生效语义）。

## 阶段门结论

**PASS WITH FOLLOW-UPS**
- P0 验收（Provider 链路、容错、降级、无积压设计、线程约束）通过。
- 遗留项（受控）：真机摄像头实时验证与真人素材端到端验证，因硬件/素材缺失按 M7 假设条款标记"未验证"；遗留项不阻塞 M8（M8 的视频链路复用同一 PoseFrame 通道）。
