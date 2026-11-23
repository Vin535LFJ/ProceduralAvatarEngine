# 01 — Current Repository Audit（P0 基线）

- 审计日期：2026-11-23。
- 基线分支：`m13-procedural-retopology`；基线 HEAD：`bf5415fc9f51af02e6f80f5adef255df87bc97e3`。
- v1.0 feature 分支：`feature/procedural-character-foundation`；基线提交 `399967f`（`chore: baseline M13 WIP and v1.0 foundation docs on feature branch`），已推送；工作区随后仅做 P0 编译稳定化与契约文档。
- 用户引用路径差异：用户提供 `Procedural_Character_Foundation_v1.0/01_....md` 等路径；仓库实际只有同名 zip，三份规范与 README 位于仓库根目录，已按根目录文件完整阅读并作为权威基线。
- 未提交基线内容：v1.0 四份文档、zip、M13 `AnatomyField.cpp` 修改、`AnatomyFeatures.cpp/.h`、README 修改，共 8 项，已在 `399967f` 全部提交，未覆盖用户工作。

## 现有资产（保留/复用）

- Android 工程：`app/src/main/java/com/pae/host/`、`app/src/main/cpp/jni_bridge.cpp`、`app/build.gradle.kts`；Gradle 7.6.1 / AGP 7.4.2 / NDK 25.1.8937393 / CMake 3.22.1 / arm64-v8a / minSdk 23 / targetSdk 33。
- Native：`native/core/math/Math.h`、`native/renderer/*`（EGL/GLES3、UBO skinning、stats、debug views）、`native/skeleton/*`（22 bone、Skinning、WeightGenerator）、`native/procedural/*`（MeshData、Topology、Validator、surface/humanoid/face/hair/clothing/anatomy）、`native/motion/*`（MotionRuntime、PoseFrame、retargeting、OneEuro、providers）、`native/runtime/EngineRuntime.*`。
- 测试：`tests/CMakeLists.txt` 8 个 CTest 可执行文件；`tests/unit/` 下 core、geometry、fusion、face_hair、anatomy、humanoid、skinning、motion 等测试。
- 证据：M0–M13 各 `docs/phases/Mx-*/evidence/`；M12 有 8 相机 × 3 debug 视图共 24 张真机截图与 clay 视图；M11/M12/M13 有性能与视觉报告。

## 基线构建与测试（实测）

- 命令：`cmake -S native -B build/host-cygwin -G "Unix Makefiles" -DCMAKE_CXX_COMPILER=C:/cygwin64/bin/g++.exe -DPAE_HOST=ON`；`cmake --build build/host-cygwin`；`ctest --test-dir build/host-cygwin --output-on-failure`。
- 结果：**8/8 测试套件通过**（core、geometry、fusion、face_hair、anatomy、humanoid、skinning、motion），总耗时 7.60s。证据：`docs/phases/P0-foundation-contracts/evidence/ctest-baseline.txt`。
- 命令：`./gradlew :app:assembleDebug --no-daemon` → **BUILD SUCCESSFUL in 12s**（35 tasks）。证据：`docs/phases/P0-foundation-contracts/evidence/gradle-assemble-baseline.txt`。
- 基线稳定化修复：当前 M13 WIP 中 `AnatomyField.cpp` 写入 `RegionPrimitives::trapRbBelly` 但结构体缺少该字段，导致 host 编译失败；已在 P0 补上字段（未改变 M12 已验证几何语义），重跑后全绿。证据：`evidence/cmake-build.txt` 初版失败记录与后续通过记录。

## 审计事项：状态机与复核清单

所有下列事项统一按 **PRELIMINARY FINDING → CODE AUDIT → REPRODUCTION TEST → CONFIRMED ISSUE → FIX / ADR** 推进。P0 只做确认与最小安全修改，不顺带重构 Skeleton、MeshData 或 Renderer。

| 事项 | 当前状态 | 审计/复现要求 |
|---|---|---|
| `Skeleton.cpp:41-42,46-47` Shoulder 与 UpperArm head/tail 相同 | CONFIRMED ISSUE（代码事实 + 回归测试失败已复现） | 修 Shoulder=chest→shoulder、UpperArm=shoulder→elbow；补 Bind/IBM/权重/Retargeting/GPU 回归；BoneId 不变；真机前后对照见 P0 报告 |
| `Skeleton.cpp:64` bindLocal 仅平移 | PRELIMINARY FINDING | 审计 BoneDefinition 方向、Bind Global 构造、IBM 生成、SkinPalette 与 GLSL 一致性、现有 GPU Skinning 是否已表达目标 A-Pose；仅当审计证明缺少必要旋转信息时才写 ADR 修复 |
| 64 字节 skinned packing 未上传 UV | PRELIMINARY FINDING | 完整追踪 `MeshData::Vertex → CPU packing → GPU VBO → VAO → vertex shader → fragment shader`；若当前材质不需要 UV，不为了结构完整而改布局；P3 需 UV 时再 ADR + 全链路同步修改 |
| ADR-008/009 与 v1.0 主线冲突 | CONFIRMED CONFLICT | ADR-010 修正定位：M12/M13 SDF 不是 P1 最终动画人体唯一生产路线；SDF 允许局部形态实验、复杂轮廓辅助、局部融合 PoC、离线/初始化研究，不被全面禁止 |
| `PoseProviderAdapter.kt:130` mirror 硬编码 / `Retargeting.cpp` rotationDegrees 未应用 / 单目 Z 语义 | PRELIMINARY FINDING | P4 做代码追踪 + 0/90/180/270° 输入测试 + 前后摄像头镜像一次验证后确认；不提前断言实现缺陷 |
| 摄像头不可用 | CONFIRMED DEVICE FACT | nunai_max CameraX FRONT/BACK/ANY 均无法解析；实时摄像头 BLOCKED，视频链路与摄像头链路分开验收 |

- Git 基线提交范围：已核对 8 项与批准清单一致后才提交；后续提交显式列文件路径，push 前核对远程同名分支，不允许覆盖他人提交。

## 风险

- R-01（SDF 主线定位）由 ADR-010 缓解，SDF 仍可做局部辅助研究；R-02（Shoulder/UpperArm 语义）由 P0 回归与真机对照缓解；R-03（摄像头缺失）保持 BLOCKED；R-04（UV）待全链路审计后再判定是否成为问题。
- 详细风险登记见 `docs/risks.md`；本 P0 审计不修改既有 M0–M13 证据。
