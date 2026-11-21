# M6 — 04 验证与验收

| 验收条件 | 结果 | 证据 |
|---|---|---|
| 六类合成动作可重复驱动角色 | PASS | `ctest` 5/5（含 `synthetic_provider_is_deterministic_and_varies_by_action`、`retarget_recovers_arm_raise_and_leg_raise`）；真机截图 `motion-pose-1..5.png` |
| 左右映射/镜像 | PASS | `normalized_coordinate_conversion_handles_mirror`：normal 与 mirrored 输入左右肩 X 符号正确翻转 |
| 低置信度/丢点/时间戳异常 | PASS | `dropout_and_low_confidence_are_controlled`：hasPerson=false 后 droppedFrames 增加且 3 秒内渐退到中性（矩阵误差 < 2e-2） |
| One Euro/四元数平滑 | PASS | `motion/filtering/OneEuroFilter.h` 与 `MotionRuntime::update` 中基于 dt/置信度的 Slerp；测试覆盖 |
| 关节限制 | PASS | `retargeting` 中肘/膝单轴投影与角度上限；单测通过 |
| 容量 1 最新值缓冲、无积压 | PASS | `MotionRuntime` 每帧只保留 latestFrame_；`droppedFrames` 计数；无队列积压路径 |
| Motion Runtime 无 SDK/Android 依赖 | PASS | `evidence/dependency-scan.txt`：`native/motion` 中无 MLKit/MediaPipe/android/JNIEnv/CameraX 命中 |
| 渲染与动作频率解耦 | PASS | 渲染每 16ms，MotionRuntime 按真实时间戳计算 dt；无同步推理阻塞 |
| 构建 | PASS | `evidence/gradle-assemble.txt`：BUILD SUCCESSFUL |

未验证：真实摄像头/ML Kit 输入（M7）、视频（M8）。
