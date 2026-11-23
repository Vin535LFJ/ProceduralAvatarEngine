# G0 基线复现与证据收集报告

> 日期时间：2026-11-23 18:38–18:45 GMT+8
> Worktree：`E:/AI-Tools/ProceduralAvatarEngine-PoC`（detached HEAD）
> Git SHA：`99941a94ec58fb37ab0f1a36b90590d3fdce5198`
> 授权：AUTH-01（Worktree）、AUTH-02（原工作区只读）、AUTH-03（Host Tests）、AUTH-06（证据文件）
> 状态：**静态与 Host 基线已建立；设备 / 性能 / Parity 证据仍缺失，不宣布 G0 全部通过**

---

## 1. 基线与可复现性

| 项 | 结果 |
|---|---|
| Worktree 创建 | `git worktree add --detach E:/AI-Tools/ProceduralAvatarEngine-PoC 99941a94...` |
| HEAD | `99941a94ec58fb37ab0f1a36b90590d3fdce5198` |
| 分支状态 | detached HEAD |
| Worktree Git 状态 | 创建后干净；本报告与日志为 AUTH-06 允许的证据文件 |
| 原工作区 | 仍为 `feature/procedural-character-foundation`，HEAD 相同；3 删 + README 修改 + 11 untracked 未触碰 |
| 远端核验 | 18:21 `ls-remote` 成功：远端 HEAD = `99941a9`；18:36 重试遇 GitHub `502 CONNECT tunnel failed`，未 fetch、未 push |

## 2. Host Tests 当前状态

- 9/9 CTest 用例 PASS，0 FAIL，0 SKIP，详见 `host-tests-report.md`。
- 重要限制：CTest PASS 仅代表用例内断言通过，**不能**证明水密、自交、外法线、动画变形或视觉质量。
- `pae_character_patch_tests` 中的边界 C0/G1 指标为硬编码值，见第 8 节。

## 3. 历史 PNG 与 manifest

- 历史截图位置：
  - `docs/phases/P1A-geometry-poc/evidence/`（clay_preset0–7、wire_front/shoulder、normals_*、action_elbow_*）
  - `docs/phases/M11-anatomy-visual-rebuild/evidence/`
  - `docs/phases/M12-unified-human-geometry/evidence/`（dv0/dv1/dv2 preset0–7、_clay.png）
  - `docs/phases/M*/evidence/`
- 未发现独立 PNG manifest 文件；唯一匹配 `manifest` 的文件是 `app/src/main/AndroidManifest.xml`。
- 结论：历史截图可作为对照证据，但缺少统一的采集 manifest（设备、APK SHA、相机预设、分辨率、时间点）。

## 4. Mesh / Topology / Parameter 基线

| 项 | 观察 |
|---|---|
| R1 路线 | `native/procedural/fusion/SdfFusion.cpp` 中 Surface Nets 与 `sdSmoothUnion` 存在 |
| smooth union 差异 | `SdfFusion.cpp:51-54` 使用 `+ k*h*(1-h)`；`AnatomyField.cpp:107-110` 使用 `- k*h*(1-h)`，符号相反 |
| Muscle Features | `AnatomyField.cpp` 定义 `sdMuscleFan` 与 MuscleShape，但 `makeUpperBodyField()` (386-456) 最终只用 ribCage/abdomen/pelvis/backCarve/deltoid/clavicle/arms/neck/head，muscle 原语未进入最终表面 |
| AnatomyFeatures.cpp | 不在 `native/CMakeLists.txt` 的 `PAE_CORE_SOURCES` 中 |
| ValidationReport | `MeshData.h:33-41` 仅含 `valid` 与 `errors`，不覆盖水密性、自交、外法线、动画质量 |
| MeshTopology | 已有 triangles / vertices / components / boundaryEdges / nonManifoldEdges / degenerateTriangles / duplicateVertices / invalidNormals 类检查；未见稳定 ID、joint loop、edge flow 通用检查 |
| 参数语义 | `HumanAnatomyParameters` 有声明，但缺少统一单位、范围、默认值、作用区域的可验证记录；当前不应标记为 VERIFIED |

## 5. 性能数据来源

- 现有来源：`docs/performance/performance-test-plan.md`、P1A 的 `gradle-assemble*.txt` / `adb-install*.txt` / `capture-log*.txt` / `logcat-p1a.txt`。
- 这些是历史文件或计划，不是本次 Worktree 构建与设备的实测结果。
- 本次 G0 未执行 Android Build 与设备采集，性能数据状态：`UNVERIFIED`。

## 6. CPU/GPU Skinning Parity

- 未执行真机数值 parity 对比。
- GPU parity 状态：`UNVERIFIED`。
- 不能把 C++ 核心可复用或 Host Tests 通过等同于 Android GPU parity 通过。

## 7. 历史失败记录

- `docs/phases/P1A-geometry-poc/06-visual-defect-report.md`
- `docs/phases/P1A-geometry-poc/04-test-and-acceptance.md`
- 提交 `486a00d8...` 记录了 P1-A visual rejection and defect archive。
- P1-A 保持 REJECTED，仅作对照，不作为新 PoC-A 的验证结果。

## 8. 硬编码 PASS / 0 指标

`native/procedural/character/BodyPatchGenerator.cpp:334-337`：

```cpp
torsoSeam.maxPositionDeviation = 0.0f;   // shared indices
torsoSeam.maxNormalDeviationDeg = 0.0f;
torsoSeam.smoothSeam = true;
torsoSeam.valid = true;
```

这些值不是实测结果；历史 C0/G1 PASS 不能作为新路线验收证据。

## 9. 生成入口与实际调用链

`native/renderer/RenderThread.cpp`：

- `:568` `generateHumanoid(BodyParameters{}, false)`
- `:592` `anatomy::generateAnatomyUpperBody(...)`
- `:613` `BodyPatchGenerator::generate(...)`

三条几何路径在渲染线程同步生成。未见独立 Geometry Worker / GenerationVersion 机制。

## 10. 同步生成、GPU 资源生命周期与线程边界风险

- `RenderThread.cpp:636-643` `releaseGpuResources()` 释放 `avatarMesh_`、`shadowMesh_`、`p1aMesh_`、`p1aLines_`、`boneUbo_`，**未释放** anatomy mesh / anatomy lines。
- `EngineRuntime.cpp:53-63` 提供 `setPreviewMode` / `setCameraPreset` / `setDebugView` 异步 post 接口。
- `RenderThread.cpp:726-744` 存在 8 个固定相机 preset 与 gray-clay debug view，G0 可复用，无需源码修改。
- JNI/Kotlin 已暴露 `nativeSetCameraPreset` / `nativeSetDebugView`（`jni_bridge.cpp:151,156`；`NativeBridge.kt:29-30`）。

## 11. G0 缺失证据与补证方案

| 缺失项 | 补证方式 | 授权 | 状态 |
|---|---|---|---|
| Android Build 产物与 APK SHA256 | 在 Worktree 执行 `./gradlew assembleDebug` | AUTH-04 | **已完成**（SDK `E:\AndroidSdk`，APK SHA256 `2c47990b…`） |
| TV Box 设备身份、GLES 信息、截图、logcat | 设备确认后 ADB 采集 | AUTH-05 | **已完成**（nunai_max，Mali-T860，GLES 3.2，9 张截图，见 tvbox 报告） |
| 设备端性能数据 | 运行期 STATS 采样 | AUTH-05 | **已完成**（54.6 fps / frame_ms 18.30 / p95 21.21 ms） |
| CPU/GPU parity 数值 | G4 或专门 parity 测试；当前保持 UNVERIFIED | 后续授权 | 未完成 |
| 统一 PNG manifest | 在 Worktree 内生成采集记录 | AUTH-06 | 未完成（本次截图已带文件名与时间戳，尚无统一 manifest 文件） |
| 参数 Mesh Delta / 自动差分 | G0/G2 参数实验 | AUTH-06/AUTH-07 | 未完成 |
| EGL 恢复路径黑帧缺陷 | 查明首次上传时序 / `releaseGpuResources()` 未释放 anatomy mesh/lines | 后续授权 | **已定位根因（DEF-01）**：渲染线程在 surface 重建时同步重算几何 4.1 s，期间不提交任何帧 → 黑屏 4.2 s。修复需批准，见 `defect-01-egl-resume-black-frame.md` |

## 12. 阶段结论（18:55 更新）

- 已建立：Git 基线、Host Tests 基线（9/9 PASS）、静态源码事实、历史证据位置、Android Build 产物与 SHA256、TV Box 设备身份与渲染证据、设备端性能实测（54.6 fps）。
- 仍未建立：CPU/GPU skinning parity 数值、设备端 skinning 权重合法性、统一截图 manifest、参数 Mesh Delta / 自动差分。
- 新发现缺陷：EGL 恢复路径黑帧 **根因已确认（DEF-01）**，根因是恢复时渲染线程同步重建几何 4.1 s 且期间不出帧；修复方案需负责人批准（生产渲染路径）。
- G0 静态 + Host + Android + 设备基线已齐备；**不宣布 G0 全部通过**，parity 与参数差分两项按补证方案在 G1/G2 处理。
- 未修改任何源码、CMake、Gradle、原工作区文件；未 Commit / Push。
- 详细设备证据见 `evidence/reports/tvbox-verification-report.md`，构建证据见 `evidence/reports/android-build-report.md`。
