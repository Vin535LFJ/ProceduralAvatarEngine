# M5 — 00 范围与假设

- 范围：std140 Bone Palette UBO、GLSL ES 300 4-influence GPU LBS、CPU 参考蒙皮对照、程序化材质参数与三种背景、颜色/背景切换不重建几何、合成骨骼动作验证关节变形。
- 非目标：完整姿态识别与 Retargeting（M6），摄像头/视频（M7/M8）。
- 假设：骨骼数 22、UBO 容量由 M2 实测 MAX_VERTEX_UNIFORM_BLOCKS=12 足够；Bind Pose 由 M4 生成；关节动画暂用确定性测试姿态，不接 SDK。
