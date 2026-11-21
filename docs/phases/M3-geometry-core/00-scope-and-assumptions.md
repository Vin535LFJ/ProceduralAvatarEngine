# M3 — 00 范围与假设

- 范围：数学 API 完整化、MeshData、基础图元、Loft/Sweep 与 Parallel Transport Frame、MeshValidator、确定性、Renderer 集成显示生成几何。
- 非目标：人体部件与骨架（M4）、GPU 蒙皮（M5）。
- 假设：生成器只输出 CPU 数据，不调用 GL；所有长度单位为米；索引三角形 winding 为逆时针正面。
