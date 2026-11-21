# M8 — 00 范围与假设

- 范围：用户经文件选择（SAF / 应用内浏览器回退）选择本地视频 → MediaExtractor/MediaCodec 解码 → ML Kit 姿态推理 → 统一 PoseFrame（PTS 时间戳）→ 复用现有 ExternalPoseProvider→MotionRuntime 通道；播放状态机（播放/暂停/恢复/拖动/倍速/结束/重播）；seek/重播/倍速切换时重置姿态滤波状态；解码失败与不支持格式明确提示。
- 非目标：网络流、画中画视频预览 UI、多视频管理、视频内多人选择、音频轨处理。
- 假设（真机已证实的环境事实）：
  1. 目标盒子（Android 8.1 leanback）**无系统 DocumentsUI**（M7 已实测 `ActivityNotFoundException: OPEN_DOCUMENT`）→ 文件选择以 M7 已实现的应用内存储浏览器为主路径，SAF 为手机端主路径（ADR-007）。
  2. 解码能力由设备 MediaCodec 实际决定，不承诺所有容器/编码。
  3. 真人测试素材当前不可得（主机网络不可用）→ 解码/PTS/状态机用可验证方式验证，"真人姿态驱动"按素材可得性标记验证状态。
- 复用：`PoseFrame` 打包与 JNI 通道（M7）、`StoragePickerDialog`（M7）、ML Kit Accurate 检测器。
