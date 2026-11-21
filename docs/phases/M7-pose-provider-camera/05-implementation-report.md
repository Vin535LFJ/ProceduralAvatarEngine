# M7 阶段报告：实时姿态 Provider 与摄像头

## 1. 阶段结论
- 状态：**PASS WITH FOLLOW-UPS**
- 完成的需求 ID：FUN-POSE-002（verified：静态图链路验证）、FUN-IN-001（implemented；真机摄像头项未验证——设备无摄像头）
- 未完成/阻塞需求 ID：无 BLOCKED 项；未验证项见 04 文档

## 2. 需求与设计
- 本阶段确认的需求：见 `01-requirements.md`（FUN-POSE-002、FUN-IN-001、COMP-001、OBS-001）
- 关键设计决定：
  - ML Kit Accurate + STREAM_MODE，Kotlin 侧运行，33 点像素坐标打包 float[132] 经 JNI 写入 `ExternalPoseProvider`（容量 1，最新帧优先）
  - 相机选择器 FRONT→BACK→ANY 顺序回退（电视盒子兼容性）
  - SAF 文件选择器不可用（盒子无 DocumentsUI）时回退应用内存储浏览器
  - 最后来源生效：显式选择合成动作时切回 Synthetic
- 新增/更新 ADR：ADR-003（ML Kit 选型）沿用；文件选择器降级策略将随 M8 记录为 ADR-007

## 3. 代码变更
- 新增文件：`app/src/main/java/com/pae/host/StoragePickerDialog.kt`
- 修改文件：
  - `app/src/main/java/com/pae/host/PoseProviderAdapter.kt`（相机回退、共享打包 `pushPoseResult`、in-flight/pose_fps 统计、`processBitmap` 单帧入口）
  - `app/src/main/java/com/pae/host/MainActivity.kt`（TestImg 按钮 + SAF→应用内浏览器回退 + 存储权限流）
  - `app/src/main/AndroidManifest.xml`（READ_EXTERNAL_STORAGE）
  - `native/renderer/RenderThread.cpp`（SetPoseMode 显式切回合成）
- 删除文件：无

## 4. 验证证据
- 构建命令与结果：`./gradlew :app:assembleDebug` → BUILD SUCCESSFUL
- 设备配置：nunai_max TV 盒子，Android 8.1（API 27），arm64-v8a；adb 安装成功
- 关键测量：ML Kit 单帧推理 **238.79–240.38 ms**（640×480 静态图，盒子 CPU）；渲染持续 48.5–49.6 FPS，draw_calls=2，triangles=3053
- 证据文件：`evidence/logcat-bind-fallback.txt`、`evidence/logcat-static-image.txt`、`evidence/logcat-m7-final.txt`、`evidence/frame-m7-boot.png`、`evidence/frame-picker-*.png`、`evidence/frame-after-static-image.png`、`evidence/pose-t1.png`、`evidence/pose-t2.png`
- 未执行验证及原因：摄像头实时帧路径（设备无摄像头）；真人素材端到端（主机网络不可用，无法获取合规素材）

## 5. 风险与遗留项
- 已知问题：盒子无摄像头（硬件事实）；ML Kit 在连续帧下的实际帧率待有摄像头设备测得（单帧 240ms → 理论 ~4 FPS，低于渲染帧率属预期，Spec 允许）
- 风险：R-03 由"待实测"更新为"确认：目标盒子无摄像头，需第二台设备验证摄像头路径"
- 需要用户决定的事项：如需完成摄像头路径与真人驱动验证，请提供带摄像头的 Android 设备或含真人的测试图片/视频

## 6. 下一阶段准备
- 进入条件是否满足：是（M6 PASS 基线 + M7 Provider 链路可用）
- 下一阶段首个任务：M8-T1 `MotionRuntime` 显式 reset（seek/重播/倍速切换时清滤波状态）+ JNI + 宿主单测
