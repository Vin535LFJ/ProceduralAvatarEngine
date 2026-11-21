# M8 — 04 测试与验收

- 日期：2026-11-21
- 设备：`MAX0019111000104`（nunai_max TV 盒子，Android 8.1 / API 27，arm64-v8a）
- 测试视频：`/sdcard/Download/pae_test_video.mp4`（设备端 `adb shell screenrecord` 自录 10s、1920×1080 H.264 MP4——设备编码器真实产物；**未打包进 APK**，属测试输入）

## 验收结果

| 验收项 | 结果 | 证据 |
|---|---|---|
| 本地视频→解码→推理→PoseFrame(PTS)→驱动链路 | **PASS** | `logcat-video-controls.txt`：`YUV->RGB conversion path: RenderScript 1920x1080`；`video stats: decoded=967 skipped=901 inferred=66 pts=9.22s` |
| PTS 同步（时间轴优先，慢推理跳帧不积压） | **PASS** | pts 单调 0→9.87s；推理慢时按时间轴跳帧（decoded 364 / skipped 331 / inferred 33 @第一轮），无队列积压 |
| 播放控制：Replay/Pause/Resume/Seek/Speed/Stop | **PASS** | `replay to 0.00s (motion filters reset)`、`playback paused at 10.81s`、`seek to 9.97s (motion filters reset)`、`speed changed to 0.5x` |
| seek/重播/倍速重置滤波状态 | **PASS** | 每次控制日志含 `motion filters reset`；宿主单测 `motion_reset_clears_external_pose_state` 通过（ctest 5/5） |
| 结束（EOS）行为 | **PASS**（修复后） | 初版 replay 后误进 EOS（flush 残留旧 EOS 输出缓冲）；修复为仅在本轮入队过 EOS 输入时采信；修复后 `replay to 0.00s` 后 PTS 重新从 0 递增，播完仅一次 EOS |
| 解码失败/不支持格式路径 | **PASS** | 初版格式不匹配异常被捕获：`video decode failed` + Toast + 资源释放退出（修复 YUV 格式后主路径正常） |
| 摄像头/Synthetic 路径回归 | **PASS** | 渲染全程 49–55 FPS（`PAE_STATS`），无回归；Pose 按钮仍可切回合成 |
| 生命周期（进程存活、资源释放） | **PASS** | 三次视频会话（含一次错误路径）后进程存活、渲染连续、无内存异常增长迹象 |

## 实施中发现并修复的真机问题

1. **盒子无系统 DocumentsUI**（M7 已发现）：视频选择走应用内存储浏览器回退（ADR-007）。
2. **解码器输出 YUV 而非 RGBA**：`producer format 0x15 != reader 0x1` 崩溃 → ImageReader 改用 `YUV_420_888` + RenderScript YuvToRGB（JPEG 兜底）。
3. **flush 后残留 EOS 输出缓冲**：seek/replay 后立即误判视频结束 → 仅在本轮真正入队 EOS 输入后采信 EOS 输出。

## 真人姿态驱动端到端验证（2026-11-21 补充，用户授权下载测试视频后完成）

- 测试素材：`angel_one_480p.mp4`（Shaka Player 官方演示短片 "Angel One"，600×480 H.264 MP4，60s，单人真人影片；来源 `https://storage.googleapis.com/shaka-demo-assets/angel-one/`，仅作本地测试输入，未打包进 APK）。备选真人素材站（Wikimedia/Pexels）在本环境网络不可达，Google 演示内容桶（shaka-demo-assets）可达且公开。
- **结果：PASS** —— 人物出镜段（pts 20–56s）ML Kit 置信度 0.42–**0.98**，数字人实时跟随人物动作：
  - `evidence/frame-person-pose-1.png`（pts≈20s，conf 0.77：站立）
  - `evidence/frame-person-pose-2.png`（pts≈27s，conf 0.88：**坐姿**——人物坐下动作完整映射）
  - `evidence/frame-person-pose-3.png`（pts≈32s，conf 0.98：**马步跨坐**）
  - `evidence/logcat-person-drive.txt`：confidence 时间轴与人物出镜吻合；推理 130–170ms/帧（480p 输入，显著优于 1080p 的 ~200ms）
- 过程中发现并修复第三个真机 bug：**从 ENDED 状态发起 replay/seek 时 `endOfStream` 标志未重置**（`applyCommand` 对非 STOP 命令返回 false 导致复位代码永不执行），表现为重播/拖动后不解码。修复为 `CommandResult` 三态返回（NONE/STOP/RESTARTED），并补充从 ENDED 状态的 replay/seek 验证路径。

## 已知行为与未验证事项（如实记录）

1. **ML Kit 对非人物画面的误报**：对屏幕录制（无真人）返回 confidence≈0.96 的关键点（模型对任意画面可能输出过拟合关键点）。MotionRuntime 的关节限位与平滑约束了表现，但"人物存在性判定"是已知缺口，列入 backlog（候选：最少可见关键点数 + 关键点几何合理性校验）。
2. **摄像头实时路径**：仍标未验证（基准盒子无摄像头，需带摄像头设备）。
3. **视频画中画预览**：MVP 不渲染视频画面本身（仅姿态提取），属设计范围而非缺陷。
4. 1080p 视频推理成本 ~200ms/帧；480p 素材推理 ~130–170ms/帧。降采样/分辨率协商列入 backlog。

## 阶段门结论

**PASS WITH FOLLOW-UPS**
- P0 验收（链路、PTS 同步、状态机、重置语义、失败路径、**真人姿态驱动**）全部通过。
- 遗留项（受控）：摄像头路径（需设备）、人物存在性判定（backlog）、性能优化候选。
