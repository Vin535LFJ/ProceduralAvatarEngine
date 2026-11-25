# P1-VR 补充授权书（AUTH-P1-VR-01）

文件：`docs/phases/P1-visual-recovery/04-p1-vr-supplemental-authorization.md`
授权号：`AUTH-P1-VR-01`
日期：2026-11-25
状态：**`GRANTED`（负责人明确确认，2026-11-25）**
签发人：项目负责人
决策来源：负责人 2026-11-25 明确指令“确认授权签署 AUTH-P1-VR-01，继续实现 VR-1A-R1”
记录方式：AI 仅转录与归档；未代签，未添加负责人未表达的技术判断。

> 本授权解锁本文件白名单内的 VR-1A-R1 实现。它不放行完整 Canonical、头脸、手足、ABI/骨骼/UBO/packing/JNI 变更，也不产生任何 Gate PASS。

## 1. 当前核查事实

- 仓库：`https://github.com/Vin535LFJ/ProceduralAvatarEngine`
- 指定分支：`poc-sm/shape-model-v2`
- 远端 HEAD：`cc26390bf11f2a965cf75c9a1c8860b118737278`
- 本地 HEAD：`cc26390bf11f2a965cf75c9a1c8860b118737278`
- 已跟踪修改：无。
- 未跟踪：`.workbuddy-ai/`、`evidence/character-v6/anatomy-clay.png`、`evidence/character-v6/anatomy-shoulder.png`、`evidence/character-v7/`，以及本授权书与 `05-vr1a-r1-implementation-whitelist.md`。
- 最新已提交 `cc26390` 只包含 P1-VR 文档与专家决定，不包含 VR-1A 代码。
- 本授权由负责人明确确认，VR-1A-R1 生产写入解锁。

## 2. 既有授权与冲突

### 2.1 AUTH-08

`docs/phases/anatomy-driven-geometry-poc/10-decision-records/04-AUTH-08-granted.md` 已签发，但范围是：

- `native/procedural/shape/v2/` host-only PoC；
- 不允许修改 RenderThread 生产入口；
- 不允许替换 legacy 三路径；
- 不允许修改 Android 生产渲染路径；
- 不允许修改 legacy 生产 C++ 文件；
- 不允许宣称 Gate PASS。

### 2.2 P1 v0.2

`docs/phases/P1-continuous-gray-body/01-task-book-v0.2.md` 与 `03-p0-snapshot-and-whitelist.md` 记录 2026-11-24 负责人另行授权 P1 Android 生产生成链路，但未明确覆盖 AUTH-08 的 host-only 限制，也未覆盖本轮新增 `native/procedural/character/` 模块。

### 2.3 负责人设计决定

负责人已确认：

- OQ-03 暂缓硬阈值；
- 自然半写实成人目标替代历史自然卡通目标；
- 采用临时 20% 同设备基线性能 guardrail。

这些设计决定不等同于生产代码修改授权。

### 2.4 治理生效确认

负责人已明确确认：

- P0 Gate 已确认；
- ADR-010 的 P1 Surface Patch 主线约束生效；
- ADR-011 的 CharacterDefinition 五层契约与冻结边界继续适用；
- ADR-013 与 `AGENT.md` 的三产出、里程碑和验收纪律继续适用。

来源：2026-11-25 负责人选择，以及提交 `cc26390` 中的 P1-VR 决策记录。AI 仅转录，不修改 ADR/Gate 文件。

## 3. 本授权拟批准的阶段

仅批准：

`P1-VR-1A-R1 — Bilateral Torso-Shoulder Vertical Slice`

范围：

- Thorax；
- Abdomen；
- Upper Pelvis；
- Shoulder Girdle；
- Axilla；
- Left Upper-arm Root；
- Right Upper-arm Root；
- Android mode-2 局部预览；
- Host tests；
- 本轮真机证据。

不批准：

- mode-3 完整 Canonical 重建；
- 全身 SDF 重写；
- 头部/面部；
- 手部/足部；
- 新骨骼；
- Corrective 系统；
- PBR/SSS；
- 头发/服装；
- 新 JNI；
- 新运行时技术栈。

## 4. 对 AUTH-08 的有限 supersede 提案

以下覆盖仅在签署后、且仅对本授权白名单路径生效：

| AUTH-08 条款 | 拟覆盖范围 | 明确不覆盖 |
|---|---|---|
| RB-1 / AL-2 host-only | 允许 `shape::v2::CharacterDefinition` 与新增 character 模块编译进 Android `pae_core` | 不改变其他 shape PoC 的 host-only 历史定位 |
| FB-5 RenderThread 生产入口 | 仅允许 `RenderThread::ensureResources` 的 mode-2 块、`drawFrame` 的 mode-2 分支、`activatePreviewGpuResources` 的 mode-2 恢复和 P1-A cache 字段；跨模式质量/状态修复须负责人额外初始 | 不修改命令队列、生命周期、EGL、Surface、Shutdown；不改变 mode-0/1/3 几何 |
| FB-6 legacy 三路径 | 允许在 mode-2 增加新 generator 选择，旧 `BodyPatchGenerator` 保留为 baseline | 不删除、不静默替换 M11/M12/P1-A 旧实现 |
| FB-7 Android 生产渲染路径 | 仅允许 mode-2 新 patch 上传、Clay/Wireframe/Normal/Weight Heatmap | 不修改 GPU packing、UBO、JNI、mode-3 Canonical |
| FB-11 legacy C++ | 仅允许修改本白名单列出的 `CharacterDefinition`、`RenderThread` 和 CMake/test 文件 | 不允许修改 `SdfFusion`、`Humanoid`、`HeadAnatomy`、`ContinuousSkinGenerator`、`CanonicalSkinGenerator` |

### 4.1 需负责人额外初始的跨模式状态修复

现有代码在 mode-2 quality refresh 和 mode-3 cache 恢复时无法保证 `Mesh/Skeleton/BodyJoints/MotionRuntime` 一致。为完成本轮强制回归，拟申请以下最小边界：

- [x] 允许 `RenderThread::processCommands()` 的 `SetQuality` case 仅为 mode-2 增加 cache 失效/重建；不改变控制命令顺序。负责人初始：项目负责人（2026-11-25）
- [x] 允许 `RenderThread::activatePreviewGpuResources()` 在 mode-2/mode-3 激活时恢复匹配的 skeleton/joints，并重新初始化或校验 `MotionRuntime`；不改变几何。负责人初始：项目负责人（2026-11-25）
- [x] 允许 `RenderThread::ensureCanonicalGeometry()` 的 cached 分支在 Surface 重建后重新初始化 `MotionRuntime`；不改变 Canonical 网格、权重、UV 或验证逻辑。负责人初始：项目负责人（2026-11-25）

上述每一项必须单独初始；总签字或 §8 的汇总勾选不能替代单项初始。未初始项保持未授权。若负责人不初始上述任一项，VR-1A-R1 的 quality/cache/mode-3 无回归验收保持 `BLOCKED`，不得降低测试要求。

## 5. 冻结区域

始终禁止修改：

- `native/skeleton/Skeleton.h/.cpp`；
- BoneId 0–21 与骨骼层级；
- `UBO[22]`；
- 64-byte GPU Vertex Packing；
- JNI ABI；
- RenderThread command queue/lifecycle；
- `ContinuousSkinGenerator` 全身 Canonical 路径；
- `CanonicalSkinGenerator` 完整 Canonical 路径；
- Head/Face/Hand/Foot 生产实现；
- `AGENT.md`、ADR、DEC、CAS、Gate 文件；
- `.workbuddy-ai/`；
- `evidence/character-v6/**`、`evidence/character-v7/**`；
- 外部模型、morph、blendshape、训练数据、JS/Python/Torch/Blender 运行时。

## 6. Git 权限边界

- [x] 允许 VR-1A-R1 验证完成后创建一个本地 commit。
- [ ] 允许在验证完成后 push 到 `origin/poc-sm/shape-model-v2`。
- [x] 不允许 push；push 需另行授权。
- [ ] 其他：________________________________。

无论选择如何，都不允许 merge、rebase、force push、reset、clean 或覆盖既有未提交工作。

### 6.1 新旧 generator baseline 方式

- [x] 同意旧 P1-A 使用既有 APK/Host 结果作为 baseline；新 APK 默认启用 VR-1A，不要求同进程切换。
- [ ] 要求同一 APK 内通过内部 `GeometryBuildRequest.generatorVersion` 切换旧/新；不新增 JNI/UI extra。
- [ ] 其他：________________________________。

若选择第二项，白名单必须包含一个明确的内部选择入口；不得复用无关 debug extra 或改变现有命令语义。

## 7. 验收与停止

- 必须同时提供代码、Host/Android 原始日志和本轮真机 Clay/Wireframe/Normal/Weight 证据。
- OQ-03 两类角度只作研究诊断，5° 不作为本轮硬验收。
- 采用临时 20% 同设备基线 guardrail：时间/内存/P50/P95/P99 不得超过同工作量 baseline 的 120%，FPS 不得低于 baseline 的 80%；局部 Patch 只与旧 P1-A mode-2 同工作量比较。
- 出现 frame 翻转、Jacobian 退化、LOD 映射失败、权重串扰、cache/skeleton 不匹配、mode-3 回归或视觉仍为球管拼接时立即停止。
- AI 不宣布 Gate PASS 或 VISUALLY ACCEPTED。

## 8. 签署

授权状态：`GRANTED`（2026-11-25 负责人明确指令）

仓库：`Vin535LFJ/ProceduralAvatarEngine`
分支：`poc-sm/shape-model-v2`
基线 commit：`cc26390bf11f2a965cf75c9a1c8860b118737278`
适用阶段：`P1-VR-1A-R1`
白名单版本：`05-vr1a-r1-implementation-whitelist.md`

治理确认：

- [x] P0 Gate 已确认。
- [x] ADR-010 生效。
- [x] ADR-011 冻结契约与 ADR-013/AGENT.md 继续适用。
- [x] §4.1 三项跨模式状态修复均已逐项初始。
- [x] §6.1 新旧 generator baseline 方式已选择。

负责人确认：项目负责人（2026-11-25 会话明确指令）
日期：2026-11-25

AI 转录：QoderCN（仅转录，未代签）
日期：2026-11-25

> 本授权已按负责人明确指令转录为 GRANTED；仅放行白名单内 VR-1A-R1，不改变 Gate 结论。
