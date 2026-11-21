# M4 — 02 设计

- `BodyParameters` 定义身高、头/肩/胸/腰/髋尺寸、肢体长度与半径、手脚尺寸、风格化与细分/seed；`clampBodyParameters` 保证合法范围与相互约束。
- 躯干：Loft（骨盆→腰→胸→颈根）；头：椭球；颈/四肢：Sweep + PTF；手脚：椭球/圆角盒；肩髋与肘膝增加球形过渡几何避免大动作裂缝。
- `Skeleton`：22 骨定义 `{id,name,parent,headWorld,tailWorld}`；bindLocal=parent-relative translation；bindGlobal=累积平移；IBM=bindGlobal 逆；矩阵链 Parent*Local。
- 权重：对每个部件的顶点沿最近骨段投影求参数 t，在段端过渡带内平滑混合相邻骨骼；最多 4 影响并归一化；头部/手部/脚部使用单骨或双骨。
- 调试：MeshData.color 可输出权重热力图颜色。
