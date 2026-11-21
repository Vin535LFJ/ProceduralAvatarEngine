# M6 — 02 设计

- `PoseFrame` 固定 33 landmark 数组，含 timestampNs/CoordinateSpace/图像尺寸/旋转/镜像/置信度。
- `SyntheticPoseProvider` 由 BodyJoints + 测试关节旋转正向运动学生成 landmark；动作 0–5：站立、抬手、屈肘、双臂水平、下蹲、抬腿。
- `Retargeting`：Landmark 对构造目标骨方向，与 Bind 方向求 `fromToRotation`，按父级世界旋转转成本地旋转；肘膝限制单轴与角度上限。
- `OneEuroFilter` 用于 landmark 位置；旋转使用基于置信度与 dt 的 Slerp；低置信度降低观测权重；丢点超过阈值渐退到中性。
- `MotionRuntime` 维护最新帧与滤波状态，输出 Bone Palette；不依赖任何 SDK 或 Android 类型。
