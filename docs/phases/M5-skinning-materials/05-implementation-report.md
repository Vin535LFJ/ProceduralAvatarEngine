# M5 — 05 实施报告与阶段门

## 阶段结论
- 状态：**PASS WITH FOLLOW-UPS**
- 完成需求 ID：FUN-RND-002、FUN-AV-002、FUN-SCN-001、TEST-001（蒙皮部分）均 verified。
- 跟进项：GPU 回读逐点数值对比未做；以共享 palette + 同构公式 + 真机截图作为当前证据，后续可在调试构建中加 readback 测试。

## 需求与设计
按 ADR-005 实现 std140 Bone Palette UBO（22×mat4，1408 字节，binding 0）；GLSL ES 300 4-influence LBS 变换 Position/Normal；CPU 参考蒙皮用于单测；Skin/Hair/Cloth/Eye Uniform 颜色 + Toon/Lambert/Rim 光照；Solid/Gradient/ProceduralStudio 全屏背景；角色与场景 revision 分离。

## 代码变更
新增：`native/skeleton/Skinning.h/.cpp`、`native/renderer/gl/SkinnedGpuMesh.h/.cpp`、`BonePaletteUbo.h/.cpp`、`tests/unit/skinning_tests.cpp`；重写 `native/renderer/RenderThread.h/.cpp`；修改 `native/CMakeLists.txt`、`tests/CMakeLists.txt`、`app/src/main/cpp/jni_bridge.cpp`、`native/runtime/EngineRuntime.*`、`app/src/main/java/com/pae/host/{NativeBridge,MainActivity}.kt`、`native/procedural/humanoid/Humanoid.cpp`（color.w 携带 materialId）。

## 验证证据
- `ctest`：4/4 passed；真机 `BUILD SUCCESSFUL in 12s`。
- 截图：`frame-initial.png`、`pose-idle.png`、`pose-arm-raise.png`、`pose-arms-horizontal.png`、`colors-and-gradient.png`。
- logcat：`frames=3360 fps≈49.7 draw_calls=2 triangles=3053 buffer_bytes=156944`；点击 Stats 得到 `avatar_rev=2 scene_rev=4`。
- 关键修复：GLSL ES 3.00 不使用 `layout(binding=0)`，改为 `glUniformBlockBinding`；测试中的 UBO 索引按列主序修正。

## 风险与遗留
- 人形仍较小、卡通简化；M7/M9 再按性能与视觉数据优化。
- 关节体积为球形过渡，极端动作仍可能出现穿插；当前 6 类测试姿势未见明显裂缝。

## 下一阶段准备
进入条件满足；M6 首个任务：实现 PoseFrame / IPoseProvider / SyntheticPoseProvider、坐标转换、Retargeting、One Euro 滤波与关节限制，替换 M5 的硬编码测试姿态。
