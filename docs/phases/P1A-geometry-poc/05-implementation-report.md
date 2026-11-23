# P1-A — 05 Implementation Report

## 阶段结论

- 状态：**NOT VERIFIED（证据已提交，等待用户视觉 Gate）**
- 前一提交：`3e573dd`；本阶段提交：见 `git log -1 --format=%H -- docs/phases/P1A-geometry-poc`。
- 完成：视觉目标补充契约、Rig-aware Landmark 驱动、Topology Template / boundary 契约、非圆 Variable-profile Sweep、PTF、面积加权法线、Patch Assembler、真机多视角灰模证据、同机位新旧对照、动作与性能数据。
- 未完成：用户视觉确认；P1-B 全部内容；最终商业外观（P3）。

## 需求与设计

按用户补充的 12 条视觉要求执行：P1-A 只做不穿衣服的基础人体局部，证明新几何路线能生成连续、自然、可骨骼变形的胸腔/腹/腰/肩胸/上臂/肘/前臂；禁止球体/椭球/圆柱拼接，禁止用材质、Shader 或三角形数量冒充几何质量。实施中以 3 个文件承载全部职责，避免为模块数量而增加类。

## 代码变更

新增：
- `native/procedural/character/CharacterDefinition.h`
- `native/procedural/character/BodyPatchGenerator.h/.cpp`
- `tests/unit/character_patch_tests.cpp`
- `tools/capture_p1a_evidence.sh`
- `docs/foundation/07-visual-target-alignment.md`
- 本阶段 00–05 文档与 evidence

修改：
- `native/CMakeLists.txt`、`tests/CMakeLists.txt`
- `native/renderer/RenderThread.h/.cpp`（previewMode=2、P1-A mesh/lines、真机报告）
- `native/runtime/EngineRuntime.h/.cpp`、`app/src/main/cpp/jni_bridge.cpp`
- `app/src/main/java/com/pae/host/{NativeBridge,MainActivity}.kt`（debug_generator/debug_pose intent extras）
- `docs/project-status.md`（P1-A 待 Gate）

## 构建与测试

- `cmake --build build/host-cygwin` + `ctest --test-dir build/host-cygwin --output-on-failure` → 9/9 通过。
- `./gradlew :app:assembleDebug --no-daemon` → BUILD SUCCESSFUL。
- 真机报告：`P1A-REPORT verts=870 tris=1664 components=1 boundary=80 nonmanifold=0 degenerate=0 badnorm=0 gen_ms=2.419 boundaries=4`。
- 真机性能：约 46–55 FPS，P50≈20.5–20.8ms，P95≈23–24ms，P99≈30ms，draw_calls=2。

## 视觉证据

- `clay_preset0..7.png`：正面/背面/左右侧面/45°/肩部与躯干特写，连续 gray Lambert。
- `wire_front.png`、`wire_shoulder.png`：线框。
- `normals_front.png`、`normals_shoulder.png`：法线。
- `action_elbow_front.png`、`action_elbow_shoulder.png`：屈肘动作。
- 同机位对照：M12 `dv3_presetN.png`；M11 历史人形截图另见 M11 evidence。

## 视觉判断与遗留

工程上已证明该路线能生成单一连通、非退化、法线合法的程序化局部人体，并能在 Android GLES3 真机运行。视觉上仍有肩部细暗线、臂部一定圆管感与肘部轻微折痕；这些没有被隐藏或包装成通过。是否达到项目负责人要求的视觉方向，必须由用户裁决。

按规则：**P1-A Gate 未确认，不进入 P1-B**。
