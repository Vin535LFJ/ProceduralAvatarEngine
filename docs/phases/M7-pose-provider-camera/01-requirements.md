# M7 — 01 需求

- FUN-POSE-002：真实 Provider 输出统一 PoseFrame 并驱动数字人。
- FUN-IN-001：CameraX 实时输入，KEEP_ONLY_LATEST，异常路径关闭 ImageProxy。
- COMP-001：运行时权限；相机方向/镜像；GLES3 能力不回归。
- OBS-001：推理耗时、Pose FPS、置信度统计。
- 验收：摄像头动作驱动角色；推理变慢不积压；UI/渲染线程不阻塞；失败路径有日志。
