# M3 — 04 验证与验收

| 验收条件 | 结果 | 证据 |
|---|---|---|
| 相同参数生成一致结果 | PASS | `geometry_tests.cpp` 中 `uv_sphere_is_valid_and_deterministic` 逐字节比较顶点与索引 |
| 无 NaN/Inf、越界索引、非法 AABB | PASS | 几何测试全部通过；Validator 负例测试覆盖越界索引、NaN、权重和不为 1 |
| 法线长度/winding/退化面 | PASS | Validator 检查法线单位长度与三角形面积；测试覆盖 Sphere/Ellipsoid/Capsule/RoundedBox/Loft/Sweep |
| Parallel Transport Frame 无翻转 | PASS | `parallel_transport_frames_avoid_flipping`：相邻法线点积 > 0.5，正交基成立 |
| 生成几何在 GLES Renderer 显示 | PASS | `evidence/frame-generated.png`：程序化生成的青色球体与三角形、立方体同屏显示 |
| 宿主机单测 | PASS | `ctest`：2/2 tests passed（core + geometry），`evidence/ctest-utf8.txt` |
| 构建 | PASS | `evidence/gradle-assemble.txt`：BUILD SUCCESSFUL；logcat 统计 `draw_calls=3 triangles=445` |

未验证：人体部件、骨骼与权重（M4）；GPU Skinning（M5）。
