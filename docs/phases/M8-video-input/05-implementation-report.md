# M8 阶段报告：本地视频输入

## 1. 阶段结论
- 状态：**PASS WITH FOLLOW-UPS**
- 完成的需求 ID：FUN-IN-002（verified：解码/PTS/状态机/**真人姿态驱动**真机验证——Shaka "Angel One" 短片，置信度最高 0.98，角色完整跟随人物站立/坐姿/马步动作）
- 未完成/阻塞需求 ID：无 BLOCKED；摄像头路径未验证（设备无摄像头，属 M7 项）

## 2. 需求与设计
- 本阶段确认的需求：见 `01-requirements.md`（FUN-IN-002 及 a/b/c/d 子项、COMP-002、OBS-001）
- 关键设计决定：
  - 解码线程 = 推理线程（单线程顺序执行），视频时钟（PTS 锚点 × speed）驱动帧推进，慢推理按时间轴跳帧，天然无积压
  - `PoseFrame.timestampNs` = 视频 PTS（µs→ns），与渲染帧率解耦
  - seek/重播/倍速 → `NativeBridge.resetMotion()` → `MotionRuntime.resetFilters()`（含清空 ExternalPoseProvider 缓冲，防旧帧重放）
  - YUV_420_888 ImageReader + RenderScript YuvToRGB 主路径 / YuvImage JPEG 兜底
  - SAF 主路径 + 应用内存储浏览器回退（ADR-007）
- 新增/更新 ADR：**ADR-007**（无 DocumentsUI 设备的文件选择降级策略）

## 3. 代码变更
- 新增文件：
  - `app/src/main/java/com/pae/host/VideoPoseSource.kt`（解码管线 + 播放状态机 + 视频时钟）
  - `app/src/main/java/com/pae/host/PoseFrameCodec.kt`（共享 33 点打包，M7/M8 复用）
- 修改文件：
  - `native/motion/MotionRuntime.cpp`（resetFilters 清空外部帧缓冲）
  - `native/motion/providers/ExternalPoseProvider.{h,cpp}`（新增 clear()）
  - `native/renderer/RenderThread.{h,cpp}`（MotionReset 命令）
  - `native/runtime/EngineRuntime.{h,cpp}`（resetMotion()）
  - `app/src/main/cpp/jni_bridge.cpp`（nativeResetMotion）
  - `app/src/main/java/com/pae/host/NativeBridge.kt`（resetMotion()）
  - `app/src/main/java/com/pae/host/MainActivity.kt`（Video 按钮 + 播放控制行 + 生命周期）
  - `app/src/main/java/com/pae/host/PoseProviderAdapter.kt`（改用 PoseFrameCodec）
  - `tests/unit/motion_tests.cpp`（新增 reset 语义测试）
- 删除文件：无

## 4. 验证证据
- 构建命令与结果：`./gradlew :app:assembleDebug` BUILD SUCCESSFUL；`ctest` 5/5 passed（含新测试）
- 设备配置：nunai_max TV 盒子（Android 8.1/API 27/arm64-v8a）；测试视频为设备端 screenrecord 自录（1920×1080 H.264 MP4，10s）
- 关键测量：RenderScript 转换路径 1920×1080 生效；推理 208–240ms/帧；时间轴跳帧 decoded/skipped/inferred 统计；PTS 单调 0→9.87s；渲染全程 49–55 FPS
- 证据文件：`evidence/logcat-video-controls.txt`、`evidence/logcat-person-drive.txt`、`evidence/frame-picker-download.png`、`evidence/frame-video-ended-controls.png`、`evidence/frame-video-playing.png`、`evidence/frame-person-pose-{1,2,3}.png`
- 未执行验证及原因：摄像头路径（M7 项，设备无摄像头）；视频预览渲染（设计非目标）

## 5. 风险与遗留项
- 已知问题：ML Kit 无人画面误报（confidence 0.96）；1080p 推理成本高（backlog 降采样）；播放状态按钮文字在 ENDED 时未同步（外观）
- 风险：R-11 无人画面误报保持 open；R-12 **closed**（真人素材端到端已验证）
- 需要用户决定的事项：无

## 6. 下一阶段准备
- 进入条件是否满足：是（M7 PASS、M8 PASS；同一 MotionRuntime 已验证实时流[合成/静态图]与视频两条路径）
- 下一阶段首个任务：M9-T1 EngineConfig 质量等级接线（Low/Medium/High → 几何细分 + 渲染分辨率）
