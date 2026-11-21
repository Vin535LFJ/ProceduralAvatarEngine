# M5 — 04 验证与验收

| 验收条件 | 结果 | 证据 |
|---|---|---|
| GPU LBS 驱动 M4 网格 | PASS | 真机截图序列 `pose-idle.png`、`pose-arm-raise.png`、`pose-arms-horizontal.png`、`frame-initial.png`；Shader 无编译错误（logcat 无 PAE_SHADER 错误） |
| UBO 布局一致性 | PASS | `skinning_tests.cpp`：`BonePaletteUboData::kBytes == 1408`，mat4 写入偏移与列主序数据一致 |
| CPU 参考蒙皮 | PASS | `ctest` 4/4：中性姿势保持原网格；单骨旋转顶点位置与 T(head)R T(-head) 预期一致 |
| 颜色切换不重建几何 | PASS | `logcat-stats.txt`：`avatar_rev=2 scene_rev=4`；截图 `colors-and-gradient.png` 显示服装由蓝变绿、背景由 Studio 变纯色 |
| 背景切换不影响角色 | PASS | 同上；`postBackground` 只更新 uniform/场景状态 |
| 三种背景模式 | PASS | 初始 `ProceduralStudio`（地面+竖向装饰）；点击 BG 后 SolidColor；Gradient 模式已实现（见 shader 与按钮循环） |
| 关节变形无明显裂缝 | PASS（关键姿势） | `pose-arm-raise.png`：抬臂后肩/肘连接连续；`pose-arms-horizontal.png`：双臂水平；`pose-idle.png`：中性 |
| GPU 数值 vs CPU 参考逐点对比 | FOLLOW-UP | 当前通过共享 palette 数据 + 同构公式 + 真机截图验证；未实现 GPU 位置回读的逐点数值比较 |

未验证：真实姿态 SDK 驱动（M6/M7）；视频（M8）。
