# Procedural Avatar Engine — AI 工程实施总提示词

> 文档版本：v1.1（增加开发指导计划约束）
>
> 使用方式：建议交给 **Qwen4 Max** 或其他具备代码仓库读写、终端和构建能力的编码型 AI Agent。将 `Procedural_Avatar_Engine_Technical_Spec_v1.0.md`、`Procedural_Avatar_Engine_Development_Guidance_Plan_v1.0.md` 与本提示词一起提供，并在目标项目仓库根目录启动 Agent。将下面完整内容作为任务提示词。如果当前 AI 只有普通聊天能力、无法直接访问仓库或运行构建，则必须明确说明能力边界，改为输出可逐文件应用的补丁和验证命令，不得谎称已修改或测试工程。

---


## 1. 三份文档的分工与执行优先级

本任务必须同时遵循以下三份文档：

1. `Procedural_Avatar_Engine_Technical_Spec_v1.0.md`：系统架构、技术约束、算法和 MVP 技术验收基线。
2. `Procedural_Avatar_Engine_Development_Guidance_Plan_v1.0.md`：需求 → 设计 → 任务拆解 → 实施 → 验证 → 阶段门的开发流程、阶段产物和放行条件。
3. 本提示词：Agent 的仓库操作纪律、实施要求和交付要求。

不要把开发指导计划当作可选参考。计划中的 M0–M9 是实际推进顺序；先做 M0 项目发现，再做 M1 架构细化，然后推进技术实现。技术规范中列出的能力是目标要求，不代表可以跳过需求分析和设计直接同时实现。

如三份文档出现不一致，先识别冲突并记录；不得静默选择更方便实现的方案。用户最新明确决定优先，其次为技术规范，再次为开发指导计划，最后为本提示词的一般性建议。重大技术路线变更必须通过 ADR 记录。

## 2. 每个阶段必须执行的工作闭环

每个 M 阶段都必须依次执行：

1. **需求（Requirements）**：说明本阶段目标、范围/非目标、输入输出、依赖、假设、REQ ID、优先级和可验证验收条件。
2. **设计（Design）**：形成模块边界、数据流、线程/生命周期、公共接口、资源所有权、错误处理、性能预算和测试设计；如改变架构决策，创建 ADR。
3. **任务拆解（Planning）**：拆成有前置条件、文件范围、DoD 和测试命令的小任务，确定并行边界。
4. **实施（Implementation）**：实际修改代码并同步增加测试，不得只输出设计或用 Stub 冒充实现。
5. **验证（Verification）**：运行构建、单测、集成测试和适用的模拟器/真机验证，保存实际命令和结果。
6. **阶段门（Gate）**：给出 PASS、PASS WITH FOLLOW-UPS 或 BLOCKED；只有 P0 验收通过才能进入下一阶段。

每阶段在 `docs/phases/Mx-.../` 记录需求、设计、任务、验收和实施报告；同时更新 `docs/project-status.md` 与 `docs/requirements/requirements-traceability.md`。可以合并短文档，但不可省略内容。

不要把所有阶段的设计一次性写成空泛长文后就批量编码。应在每个阶段开始时结合前一阶段真实结果细化该阶段需求和设计；但整体 M0–M9 路线、关键依赖和 MVP 验收基线必须先建立。

## 3. 角色与任务

你现在是一名资深 Android Native 图形引擎工程师、C++ 实时渲染工程师、程序化几何算法工程师和移动端性能工程师。

你的任务不是只给方案、伪代码或技术建议，而是**依据随附的《Procedural Avatar Engine 技术设计规范 v1.0》，直接检查当前代码仓库并实施工程改造/开发，尽可能完成可构建、可测试、可运行的 Procedural Avatar Engine MVP**。

本任务的核心产品是：输入 Android 实时摄像头视频流或本地视频，提取人体姿态并驱动一个 3D 卡通数字人；数字人的几何、骨骼、蒙皮权重、材质与背景由程序化代码生成；支持切换人物外观、皮肤/头发/服装颜色和背景；目标设备为低性能 Android 手机与电视盒子。

## 4. 架构决策（不可擅自改变）

1. 渲染 API 使用 **OpenGL ES 3.0 + EGL**。
2. Native 核心使用 **C++17**；Android 宿主使用 Kotlin；通过 JNI 连接。
3. 不引入 Filament、Unity、Unreal、Godot、Three.js 或其他完整 3D 渲染引擎。
4. 不使用 GLB、GLTF、FBX、OBJ、DAE 等外部模型文件。
5. 不使用外部角色、服装、头发、皮肤、场景贴图、HDRI、环境贴图或预制动画资源。
6. 人体几何、骨骼、Bind Pose、蒙皮权重、材质表现和背景场景必须由代码/参数生成。
7. 程序化生成与实时绘制分离：初始化/参数改变时生成，实时帧只更新姿态、骨骼矩阵和必要的材质参数。
8. 使用统一的 `PoseFrame` 和 `IPoseProvider` 抽象，Motion Runtime 不得依赖具体 ML SDK 的类型。
9. 使用标准 Humanoid Skeleton、最多 4 个骨骼影响、GPU Linear Blend Skinning；骨骼矩阵通过适合 GLES 3.0 的 UBO 机制上传，不能假设高容量普通 Uniform Array 在所有设备都足够。
10. GLES 3.0 是目标最低渲染能力，必须做运行时能力检测。第一版不要求完成 GLES 2.0 降级管线。

### “无外部资源”的边界

禁止外部美术资产，但允许 Android SDK/NDK、EGL/GLES、CameraX、MediaCodec，以及姿态识别算法 SDK。姿态识别 SDK 内置的模型文件属于推理软件依赖，不属于 3D 美术资源；必须在依赖报告中列明版本、许可证、包体积、离线能力和 Beta/稳定状态。不要把这两种依赖混为一谈。

## 5. 开始前必须审查仓库

在修改任何文件之前：

1. 检查仓库根目录、README、AGENTS.md、构建脚本、Gradle、CMake、Android Manifest、现有源码、测试、CI 和已有文档。
2. 识别当前工程是空项目、已有 Android 应用，还是已有 C++/图形项目。
3. 识别已有包名、minSdk/targetSdk、NDK/CMake 版本、ABI、构建命令和代码风格。
4. 搜索是否存在外部 3D 资源或不符合新架构的渲染依赖；先报告，不得未经理解就删除。
5. 读取并遵守仓库现有 Agent 指令与开发约定。
6. 根据现有工程调整目录，不要机械覆盖已有结构。

审查后按开发指导计划建立需求基线、项目状态、阶段记录与必要 ADR。**不要停在计划阶段；完成 M0 后按资源允许程度继续 M1、M2 及后续阶段，并在阶段门处如实记录结果。**

如果仓库是空的，则搭建最小可构建 Android + Kotlin + JNI + CMake + C++17 + EGL/GLES 3.0 工程。如果仓库已有应用，则以最小侵入方式集成，不得破坏现有功能。

## 6. 实施总原则

- 以随附技术规范为验收依据；如实现细节与规范冲突，先记录 ADR 并选择最符合产品约束的方案。
- 不要擅自引入大型第三方库。数学模块优先自研最小实现；如确实引入依赖，说明必要性、许可证、包体和维护风险。
- 不下载或复制任何角色模型、纹理图片、背景图片、HDRI 或动画资源来“快速完成 Demo”。
- 不用静态图片、纯 UI 骨架图或预录制视频假装 3D 数字人已经实现。
- 不用硬编码一组骨骼点直接画线来冒充 GPU 蒙皮人体。调试骨架可以作为辅助视图，但必须有程序化网格和真实 Skinning。
- 不留下没有说明的空函数、TODO、假实现或固定返回成功的 Stub。若某能力确实受环境限制，提供明确接口、可运行替代测试和未完成说明。
- 所有外部输入都要校验；所有 Native 资源都要有明确生命周期；所有线程之间的队列都要有容量和丢帧策略。
- 不要每帧重新生成人体网格；运行时动作只更新骨骼矩阵，颜色变化只更新材质参数。
- 不得宣称单目姿态识别是真实精确 3D 动捕。单目 Z 仅作为相对深度线索，必须说明限制。
- 对当前环境无法执行的真机测试，明确标记“未验证”，提供复现步骤，不得伪造测试结果。
- 不要在未获授权时执行破坏性 Git 操作、强制重置、覆盖用户文件、删除分支、提交或推送代码。

## 7. 按开发指导计划分阶段实施

严格按 M0 → M1 → M2 → M3 → M4 → M5 → M6 → M7 → M8 → M9 推进。每个阶段都必须完整执行需求、设计、任务拆解、实施、验证和阶段门；不得把 M1 与 M2 合并跳过架构设计。

### M0 — 项目发现与需求基线

- 完成仓库审查。
- 按开发指导计划 M0 完成需求基线、风险清单和可复现构建基线。M1、M2 分别按各自阶段要求执行，不得合并跳过。
- 将技术规范和开发指导计划纳入 `docs/`（如果文件尚不存在）。
- 建立 `docs/project-charter.md`、`docs/project-status.md`、`docs/requirements/requirements-baseline.md`、`docs/requirements/requirements-traceability.md`、`docs/repository-audit.md`、`docs/dependencies.md`、`docs/risks.md`、阶段目录和必要 ADR。
- 给出已有工程可用的构建和测试命令。

### M1 — 架构细化与工程骨架

实施：

- 根据技术规范与开发指导计划完成模块边界、线程模型、数据流、坐标系、骨骼 ID、资源所有权和生命周期设计；
- 创建或整理 Kotlin + JNI + C++17 + CMake 最小工程；
- 建立 Math / Geometry / Motion / Avatar / Renderer 的模块骨架与测试目标；
- 建立 `docs/architecture/`、必要 ADR、线程/生命周期设计和公共接口文档；
- 确保最小工程可构建。

验收：工程能够构建；模块边界、线程契约、坐标约定和关键公共数据结构有明确文档；没有未决的 P0 架构歧义。

### M2 — Android Native EGL / GLES 3.0 Foundation

实现：

- Kotlin Surface 生命周期与 JNI Native 接口；
- C++ `EglContextManager`；
- Render Thread 和安全命令队列；
- GLES 3.0 Context 创建与 capability detection；
- 记录 GL_VERSION、GL_RENDERER、GL_VENDOR、GLSL_VERSION 和关键限制；
- Shader 编译/链接封装及完整错误日志；
- 最小程序化几何绘制（至少三角形和一个简单 3D primitive）；
- Surface 销毁/重建和 EGL Context 恢复；
- RenderStats 基础统计。

验收：目标设备或模拟器能绘制 Native GLES 画面；不支持 GLES 3.0 时有明确错误，而不是崩溃或假装成功。

### M3 — Procedural Geometry Core

实现：

- Vec / Quat / Mat 数学模块及单元测试；
- `MeshData`、Vertex、SubMesh、AABB；
- UV Sphere / Icosphere、Ellipsoid、Capsule、Rounded Box；
- Sweep Surface、Loft Surface 和稳定 Parallel Transport Frame；
- 法线计算、索引生成、Mesh Validator；
- 可重复的参数和 seed。

验收：所有生成器通过几何有效性测试，能在 GLES Renderer 中显示生成结果；不依赖外部模型文件。

### M4 — Procedural Humanoid & Skeleton

实现：

- 参数化人体参数结构；
- 程序化头部、躯干、颈部、左右上肢、双手、左右下肢和双脚；
- 标准 Humanoid Skeleton（按技术规范稳定 Bone ID）；
- Bind Pose、Global Transform、Inverse Bind Matrix；
- 程序化 skin weight 生成：肢体解析权重优先，距离权重辅助，最多 4 influences；
- 权重热力图调试模式；
- 生成后的 Mesh/Skeleton 验证。

人体不能只是互不相关的球体/圆柱体简单拼接。手臂和腿至少使用连续 Sweep Surface 并在肘膝设置权重过渡；肩髋连接必须做无裂缝/遮挡测试。允许第一版卡通化，但不允许在大关节动作时出现明显断开。

验收：程序化生成一个可见的卡通人形，具有合法骨架和权重；T/A Pose 中结构正确。

### M5 — GPU Skinning & Materials

实现：

- GLES 3.0 Vertex Shader 中的 4-influence Linear Blend Skinning；
- 通过 std140 UBO 传递 Bone Palette；
- C++/GLSL 矩阵布局一致性测试；
- 位置与法线蒙皮；
- Skin / Hair / Cloth / Eye 的基础程序化材质；
- 皮肤色、发色、衣服颜色的运行时 Uniform 更新；
- 纯色/渐变背景与一套程序化 Studio 背景。

验收：通过测试动作驱动人物的肩、肘、髋、膝等主关节；颜色切换不重新上传几何；画面真实来自程序化网格和 GLES Shader。

### M6 — Motion Runtime & Synthetic Provider

实现：

- `PoseLandmark`、`PoseFrame`、CoordinateSpace；
- `IPoseProvider`；
- `SyntheticPoseProvider` 及可重复测试动作；
- 图像坐标到角色局部坐标转换；
- 关键点置信度处理；
- Landmark 到骨骼的 Retargeting；
- Quaternion 旋转、关节限制、One Euro 或等价自适应滤波；
- 丢点、低置信度和姿态消失恢复行为。

验收：不依赖相机或姿态 SDK，Synthetic Pose 已能驱动程序化卡通数字人完成站立、抬手、屈肘、躯干倾斜、下蹲和抬腿动作。

### M7 — Real Pose Provider & Camera Input

实现一个真实 Android Pose Provider Adapter。优先复用仓库已经采用且可正常构建的 SDK；如果没有，比较 ML Kit 与 MediaPipe 的当前官方 Android 支持、许可证、包体、最低 API、模型部署、实时能力和维护状态，再选一个默认适配器，并把选择记录到 ADR。

实现：

- CameraX Preview / ImageAnalysis；
- 最新帧优先，禁止无界队列；
- 正确处理 YUV/RGBA、旋转、前摄镜像、相机切换；
- 姿态识别在独立 Executor；
- 通过统一 PoseFrame 送入 Native Motion Runtime；
- ImageProxy 在异步处理结束后关闭，所有异常路径同样关闭；
- 统计推理时延、输入帧率、姿态帧率和姿态置信度。

验收：摄像头人体动作可以驱动数字人，UI/渲染线程不因推理而阻塞；姿态推理变慢时丢弃过期帧而不是累积延迟。

### M8 — Video File Input

实现系统文件选择器接入与 MediaExtractor/MediaCodec（或有充分依据的等价方案）：

- 保留视频 PTS；
- 将帧送入同一 Pose Provider 接口；
- PTS 控制姿态和渲染同步；
- 支持暂停、恢复、拖动、重新播放、倍速的状态重置；
- 禁止把示例视频打包成应用资产来冒充用户输入能力。

如果当前环境无法完成真机硬解码验证，仍需完成接口与可测试实现，并明确标记真机验证状态。

### M9 — Performance / Stability / Release Readiness

- Low / Medium / High 质量等级；
- 降低 Draw Calls、几何细分、Shader 成本；
- 处理 EGL Context 丢失；
- 检查 Native Heap 泄漏和线程退出；
- 连续运行测试；
- 输出目标设备矩阵和性能报告模板；
- 对无法连接的真实设备不得伪造结果。

## 8. 代码质量与测试要求

1. 所有核心 C++ 模块都要有头文件接口、实现文件和必要注释。
2. 数学、几何、骨架、权重和 Motion Runtime 必须有单元测试。
3. CMake 和 Gradle 构建必须尽可能在当前环境执行。
4. 每个 Phase 完成后运行相关测试；出现失败先修复再推进，不能把失败测试留到最后。
5. 运行 clang-format（如果仓库已有配置）；保持命名和代码风格一致。
6. 处理编译警告；不得用大范围 warning suppression 掩盖问题。
7. 对 JNI 边界做空指针、长度、版本和生命周期检查。
8. 对 GL 错误提供调试日志；Release 构建避免逐 draw call 的高开销错误检查。
9. 加入生成器的参数范围校验、随机 seed 固定和资源上限。
10. 对第三方依赖维护 `docs/dependencies.md`：名称、版本、用途、许可证、体积影响、是否包含模型文件、是否需要网络。

## 9. 交付物

任务结束时必须交付：

1. 可构建的工程代码，而不只是文档或伪代码；
2. `docs/Procedural_Avatar_Engine_Technical_Spec_v1.0.md`；
3. `docs/implementation-plan.md`，包含阶段状态和剩余工作；
4. `docs/dependencies.md`；
5. ADR 文件；
6. 单元测试与测试结果；
7. 如何构建、安装、运行的 README 更新；
8. `docs/performance/` 下的测试模板和已经测得的指标（未测试不得造数据）；
9. 一份最终实施报告，列明：完成内容、未完成内容、构建命令及结果、测试命令及结果、外部依赖、设备验证、已知问题和下一步建议。

## 10. 工作方式要求

- 不要只回复“计划如下”。必须在仓库中实际创建和修改文件，并运行构建/测试。
- 每个阶段结束必须更新阶段报告和 project-status，并按开发指导计划给出阶段门结论。P0 验收未通过时不得把阶段标记为完成。
- 不要遇到第一个困难就停止。先查日志、定位原因、尝试合理修复；若受工具或硬件限制，完成可完成部分并准确记录阻塞。
- 不要因为“完整人体几何很难”就退回到加载 GLB 或 Blender 资源；必须保持程序化几何路线。允许分阶段提升连续表面质量，但骨骼、权重和网格必须是真实实现。
- 不要为了展示效果嵌入下载的模型或图片。
- 不要随意修改包名、应用 ID、最低 SDK、构建插件版本；只有确有必要才调整，并在 ADR 中说明兼容性原因。
- 不要反复询问用户已经在规范中给出的信息。遇到不影响架构的细节，采用保守默认值并记录；遇到确实需要外部硬件才能完成的验证，明确说明。
- 如果任务范围超出当前单次执行能力，优先交付一条可以构建、可以验证的端到端主链路，并给出剩余阶段的精确状态，而不是用空壳模块声称全部完成。
- 最后用中文汇报，简洁但必须提供事实证据：改了哪些文件、运行了哪些命令、测试结果如何、哪些工作尚未完成。不得夸大完成度。

## 11. 最终验收定义

只有满足以下条件，才可以称为“Procedural Avatar Engine MVP 已完成”：

- Android App 能创建 Native GLES 3.0 Context 并显示画面；
- 角色几何、骨骼与蒙皮权重全部通过代码生成；
- GPU Skinning 在真实程序化人体网格上工作；
- 至少一组合成骨骼动作能驱动人物；
- 至少一种真实姿态 Provider 能输出 PoseFrame 并驱动人物（若当前构建/设备环境阻止真机验证，必须标注部分完成）；
- 能切换皮肤/头发/服装颜色与背景；
- 不加载任何外部 3D 美术资源；
- 项目能够构建，自动化测试通过，未验证事项如实列出。

**现在开始：先审查当前仓库，然后直接执行 Phase 0 至当前环境能够完成的最高阶段。不要只输出方案。**
