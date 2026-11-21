# M5 — 03 任务拆解

| 任务 | 文件范围 | 测试命令 | DoD |
|---|---|---|---|
| T1 CPU 参考蒙皮 + UBO 布局 | `native/skeleton/Skinning.*`, `tests/unit/skinning_tests.cpp` | ctest | 中性姿势/单骨旋转通过 |
| T2 SkinnedGpuMesh + Shader + UBO | `native/renderer/gl/SkinnedGpuMesh.*`, shaders in RenderThread | APK 构建+logcat | 无 shader 错误 |
| T3 材质/背景/切换命令 | `RenderThread.*`, JNI, Kotlin 控件 | 真机截图 | 颜色/背景可切换 |
| T4 合成动作关节验证 | RenderThread 测试姿态 | 截图序列 | 抬臂/屈肘/抬腿/屈膝 |
| T5 阶段门 | `docs/phases/M5-skinning-materials/*` | 对照 Exit Criteria | 结论写入 |

T1→T2→T3→T4→T5。
