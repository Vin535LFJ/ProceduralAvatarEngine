# M7 — 00 范围与假设

- 范围：CameraX ImageAnalysis + ML Kit Pose Detection Adapter；权限、旋转/镜像、最新帧优先、ImageProxy 全路径关闭；33 点→PoseFrame→JNI→MotionRuntime；推理时延与置信度统计。
- 非目标：视频文件（M8）、性能优化报告（M9）。
- 假设：设备声明摄像头特性；ML Kit 18.0.0-beta5（Beta、专有权条款、精确模型、Z 实验性）已按 ADR-003 披露；真实摄像头若不可用则用静态图/视频帧验证 SDK→PoseFrame 链路并标摄像头未验证。
