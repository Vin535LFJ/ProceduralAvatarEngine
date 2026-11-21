# M4 — 00 范围与假设

- 范围：BodyParameters 与约束；程序化头/颈/躯干/肩/上肢/手/下肢/脚；固定 22 骨 Humanoid Skeleton、Bind Pose/IBM；解析+距离辅助自动权重（≤4）；网格/骨架/权重验证；调试着色。
- 非目标：GPU Skinning、材质系统、姿态输入（M5/M6）。
- 假设：canonical bind pose 采用 A-Pose（ADR-004）；所有部件使用 M3 的 Loft/Sweep/图元生成器；骨架旋转在 Bind 中为单位四元数，关节位置决定层级平移。
