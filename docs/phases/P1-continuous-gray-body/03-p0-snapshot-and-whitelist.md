# PAE-P1-001 — P0 快照、保护清单与修改白名单 v0.2

日期：2026-11-24

## 1. 授权状态

项目负责人在 2026-11-24 会话中明确确认：

- PoC-SM 并发写入已经结束，可以冻结。
- 授权 P1 Android 生产生成链路修改。
- 允许 P1 验收前创建一个新 Git commit。
- 不包含 push、merge、rebase。
- 不修改或签署 AUTH、ADR、DEC、CAS、Gate 文件。

本文件记录执行授权，不替代正式治理文件。

## 2. 冻结基线

| 项目 | 值 |
|---|---|
| 工作区 | `E:/AI-Tools/ProceduralAvatarEngine-poc-sm` |
| 分支 | `poc-sm/shape-model-v2` |
| HEAD | `f3e3bc2970e9c4844929d632077c765890c6b2b7` |
| 提交标题 | `docs: record the character route change and round-3 evidence` |
| 快照目录 | `docs/phases/P1-continuous-gray-body/evidence/p0/` |

快照文件：

- `head.txt`、`branch.txt`、`commit.txt`
- `status.txt`
- `working-tree.patch`
- `staged.patch`
- `diff-stat.txt`
- `untracked.txt`
- `sha256.txt`
- `ctest-baseline.log`
- `android-build-baseline.log`
- `apk-sha256.txt`
- `device-model.txt`

## 3. 基线工作树

P0 捕获到的既有修改：

- `native/CMakeLists.txt`
- `native/procedural/humanoid/FaceGenerator.cpp`
- `native/procedural/humanoid/FaceGenerator.h`
- `native/procedural/humanoid/Humanoid.cpp`
- `native/procedural/humanoid/anatomy/HumanAnatomyGenerator.cpp`
- `native/procedural/humanoid/anatomy/HumanAnatomyGenerator.h`
- `native/renderer/RenderThread.cpp`
- `tests/unit/anatomy_tests.cpp`
- `tests/unit/face_hair_tests.cpp`

既有未跟踪内容：

- `native/procedural/humanoid/HeadAnatomy.cpp`
- `native/procedural/humanoid/HeadAnatomy.h`
- `evidence/character-v6/anatomy-clay.png`
- `evidence/character-v6/anatomy-shoulder.png`
- `evidence/character-v7/face.png`
- `evidence/character-v7/front.png`
- `.workbuddy-ai/`

上述内容的原始 diff 和状态已保存。P1 不得删除、覆盖或清理这些成果；如需修改既有文件，必须在 P1 白名单内并以快照为回退依据。

## 4. 构建与测试基线

### Host

- 17 个 CTest 测试中 16 个通过。
- 唯一失败：`pae_anatomy_tests`。
- 失败原因：既有 M12 解剖网格 `nonManifoldEdges = 2`。
- 原始日志：`ctest-baseline.log`。

分类：既有基线拓扑失败，不是 P1 新代码引入。P1 最终报告必须继续列出，不得隐藏。如果 P1 复用或修改该拓扑路径，应在白名单内修复并保留回归测试。

### Android

- 构建命令：`:app:assembleDebug`。
- 结果：`BUILD SUCCESSFUL`。
- JDK：`C:/Users/Giant/.jdks/jbr-17.0.12`。
- SDK：`C:/Users/Giant/AppData/Local/Android/Sdk`。
- 工作区没有 `local.properties`；构建通过环境变量完成。
- APK SHA-256：`843bb2b1c5dd961a9366c075cc8c838987745f86eee9cfcf1b44e54e4f0ad873`。
- 原始日志：`android-build-baseline.log`。

### 真机

- 设备：`nunai_max`。
- 状态：ADB 在线。
- 该设备无摄像头；P1 只使用程序化人物和固定姿态，不依赖摄像头。
- 当前只有一台真机，性能数据不得推断为全平台结论。

## 5. P1 修改白名单 v0.2

### 5.1 允许修改

**程序化几何与 Canonical 数据**

- `native/procedural/humanoid/canonical/**`（新增）
- `native/procedural/humanoid/Humanoid.cpp`
- `native/procedural/humanoid/Humanoid.h`
- `native/procedural/humanoid/HeadAnatomy.cpp`
- `native/procedural/humanoid/HeadAnatomy.h`
- `native/procedural/humanoid/anatomy/**`（仅在与 P1 拓扑复用相关时）
- `native/procedural/fusion/SdfFusion.cpp`
- `native/procedural/fusion/SdfFusion.h`

**权重与蒙皮**

- `native/skeleton/Skinning.cpp`
- `native/skeleton/Skinning.h`
- `native/skeleton/weights/**`

**渲染与相机**

- `native/renderer/gl/SkinnedGpuMesh.cpp`
- `native/renderer/gl/SkinnedGpuMesh.h`
- `native/renderer/RenderThread.cpp`
- `native/renderer/RenderThread.h`
- `native/renderer/egl/EglContextManager.cpp`
- `native/renderer/egl/EglContextManager.h`
- `app/src/main/java/com/pae/host/MainActivity.kt`
- `app/src/main/java/com/pae/host/NativeBridge.kt`（如存在且不需要改变 JNI 签名）

**构建与测试**

- `native/CMakeLists.txt`
- `tests/CMakeLists.txt`
- `tests/unit/p1_continuous_gray_body_tests.cpp`（新增）
- `tests/unit/p1_render_data_tests.cpp`（新增）
- `tests/harness/**`（仅在测试需要时）

**文档与证据**

- `docs/phases/P1-continuous-gray-body/**`
- `evidence/p1-continuous-gray-body/**`（新增）

### 5.2 保护但不得清理

- 所有 P0 已修改文件。
- `HeadAnatomy.cpp/.h`。
- `evidence/character-v6/**`。
- `evidence/character-v7/**`。
- `.workbuddy-ai/`。

这些内容可以被 P1 必要修改引用，但不得删除、移动、覆盖性重写或纳入未经审查的提交。

### 5.3 禁止修改

- `AGENT.md`。
- `docs/architecture-decisions/**`。
- AUTH、ADR、DEC、CAS、Gate 文件。
- `docs/phases/anatomy-driven-geometry-poc/06-owner-signoff-checklist.md`。
- `docs/phases/anatomy-driven-geometry-poc/09-blocker-closure-auth08/**`。
- JNI ABI、22 骨骼编号、UBO[22]、旧 64B Vertex Packing。
- `.workbuddy-ai/`。
- 其他会话或实验目录。

### 5.4 Git 操作

- 允许在 P1 验证完成后创建一个新 commit。
- 提交前必须检查 staged diff 和文件清单。
- 不 push、不 merge、不 rebase。
- 不自动提交 `.workbuddy-ai/`、治理文件或未批准证据。

## 6. P0 结论

- 并发写入已结束。
- 基线已冻结并保存补丁。
- Android 构建成功，真机在线。
- Host 基线存在 1 个既有 M12 非流形边失败，已记录。
- 修改白名单和提交授权已明确。
- 可以进入 P1-A1 实施；最终验收仍要求所有失败和未验证项如实报告。
