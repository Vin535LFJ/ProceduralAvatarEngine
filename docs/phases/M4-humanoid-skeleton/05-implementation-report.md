# M4 — 05 实施报告与阶段门

## 阶段结论
- 状态：**PASS WITH FOLLOW-UPS**
- 完成需求 ID：FUN-RIG-001、FUN-RIG-002 均 verified；FUN-AV-001 的几何生成部分 verified（关键姿势变形留 M5）。
- 未完成/阻塞：无 P0 阻塞；跟进项：M5 必须用合成骨骼动作验证肩/肘/髋/膝变形与裂缝。

## 需求与设计
按 ADR-004 采用 A-Pose；躯干 Loft、头/发/眼椭球、颈与四肢连续 Sweep、手脚参数几何、肩髋肘膝球形过渡；22 骨层级与 IBM 由 `BodyJoints` 派生；权重沿最近骨段投影并在段端平滑混合。

## 代码变更
新增：`native/skeleton/Skeleton.h/.cpp`、`native/procedural/humanoid/Humanoid.h/.cpp`、`tests/unit/humanoid_tests.cpp`；修改 `native/CMakeLists.txt`、`tests/CMakeLists.txt`、`native/renderer/RenderThread.h/.cpp`（显示程序化人体并调整相机）。

## 验证证据
- `ctest`：3/3 passed（core、geometry、humanoid）。
- 真机：`frame-humanoid.png`；logcat：`frames=120 fps=51.5 frame_ms=19.41 draw_calls=1 triangles=3052 buffer_bytes=81216`。
- 单测覆盖参数约束、层级/IBM、矩阵链、权重/网格合法性、确定性与热力图。

## 风险与遗留
- 当前人形在画面中偏小、卡通简化，属于 M4 范围；视觉质量后续按 M7/M9 数据调整。
- 大动作裂缝风险 R-06 转入 M5 验证。

## 下一阶段准备
进入条件满足；M5 首个任务：实现 std140 Bone Palette UBO、GLSL 4-influence LBS、CPU 参考蒙皮对照、材质参数与三种背景。
