# M4 — 04 验证与验收

| 验收条件 | 结果 | 证据 |
|---|---|---|
| 人体由程序生成，无外部模型/纹理 | PASS | `tests/unit/humanoid_tests.cpp`、`frame-humanoid.png` |
| 骨骼无环、ID 稳定、IBM 有效 | PASS | `pae_humanoid_tests` 中 `skeleton_hierarchy_and_ibm_are_valid`：parent<i、名称一致、bindGlobal*IBM≈I、Parent*Local==Global |
| 权重非负、和为 1、≤4 影响、骨骼索引有效 | PASS | `humanoid_mesh_and_weights_are_valid`；实现中最多写入 2 个影响 |
| 网格法线/索引/AABB 合法 | PASS | `validateMesh(humanoid.mesh)` 通过；3052 三角形、81216 buffer bytes |
| 确定性 | PASS | `humanoid_generation_is_deterministic` 逐字节比较顶点/索引与骨架矩阵 |
| 参数约束 | PASS | `body_parameters_are_clamped` |
| 权重热力图 | PASS | `heatmap_colors_are_generated` |
| 程序化人形可见 | PASS | `evidence/frame-humanoid.png`：A-Pose 人形，头/发/躯干/四肢/手可见；`draw_calls=1 triangles=3052` |
| 关键姿势下无明显断裂/翻面 | FOLLOW-UP → M5 | 当前仅 Bind Pose 静态显示；实际关节变形须待 M5 GPU Skinning 后验证 |

未验证：GPU Skinning 下的抬臂/屈肘/抬腿/屈膝（M5）；真实姿态驱动（M6/M7）。
