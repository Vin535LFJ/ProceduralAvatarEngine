# 00 — Product Scope（P0 基线）

- 版本：v1.0 基线对齐；更新 2026-11-23。
- 产品目标：在 Android GLES 3.0 上，用 C++ 数学规则与程序化曲面生成一个 Level-1 卡通数字人：完整人体、标准 Humanoid 骨骼、自动蒙皮、基础程序化动作、后续可换装、可材质切换、可由姿态 Provider 驱动。
- 正式资产来源：C++ 几何生成函数、人体形态参数、拓扑模板（来源限定见 ADR-014：逐文件许可证与哈希登记的 CC0 资产经离线编译为 PAE 自有模板）、关节/骨骼规则、Shader/材质参数、程序化场景规则。禁止 DCC 导出模型、GLB/GLTF/FBX/OBJ、外部人体/服装/头发/场景模型与外部美术贴图作为运行时依赖。
- 正式几何主线（P1 起）：Rig-aware Anatomical Landmarks + Procedural Surface Patches + Topology Templates + Variable-profile Sweeps；非圆截面、宽深独立、稳定 Parallel Transport Frame、patch 边界缝合、关节拓扑预留。SDF/Surface Nets 保留为研究与对照路径，允许局部形态实验、复杂轮廓辅助与离线/初始化研究，但不得作为 P1 最终动画人体网格的唯一生产来源。
- 固定技术栈：C++17 / Kotlin / JNI / Android NDK / EGL / OpenGL ES 3.0 / GLSL ES 3.00；保留自研 Renderer，不引入 Unity/Unreal/Godot/Filament 等完整引擎。
- 规则优先级：用户明确指令 > Specification > Roadmap > Execution Prompt > 当前有效 ADR > 现有代码与测试事实。冲突必须写 ADR，不得静默忽略。
- 阶段顺序：P0 → P1-A → P1-B → P1-C → P1-D → P2 → P3 → P4 → P5；P1-A、P1-B、P1-C、P1-D/P1 最终、P2、P3、P4、P5 均为用户 Gate。AI 不得自行宣布视觉验收通过。
- P0/P1 非目标：P0/P1 不接 ML Kit、不做多套服装、不做通用建模器/全局重拓扑、不做 PBR、不做布料物理、不做 DQS/corrective shapes。
- P4/P5 当前设备约束：nunai_max 电视盒子无摄像头；实时摄像头驱动标 BLOCKED / NOT VERIFIED，先用 `vedio_demo.mp4` 等已有视频验证 SDK→PoseFrame→JNI→Retargeting→渲染链路，真人摄像头验证等手机设备。
