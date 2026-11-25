# P1-VR-1A-R2 补充授权书草稿

文件：`docs/phases/P1-visual-recovery/09-vr1a-r2-supplemental-authorization-draft.md`
任务：`P1-VR-1A-R2 — Anatomical Silhouette & Shoulder Form Reconstruction`
日期：2026-11-25
状态：**`DRAFT — NOT EFFECTIVE`**

本文件仅为授权草稿，不代表负责人已签署。`AUTH-P1-VR-01` 只覆盖 VR-1A-R1，不自动覆盖 R2。草稿签发前不得修改生产代码。

## 1. 基线事实

- 仓库：`https://github.com/Vin535LFJ/ProceduralAvatarEngine`。
- 分支：`poc-sm/shape-model-v2`。
- R1 提交：`299ae3bf3fcb6d76be563f815aa52380e018d50b`。
- R1 状态：`IMPLEMENTED / TECHNICAL CHECK PARTIAL / VISUAL BLOCKED`。
- 当前未跟踪保护文件：`.workbuddy-ai/`、`evidence/character-v6/**`、`evidence/character-v7/**`。
- R2 目标定义：`07-vr1a-r2-shape-target.md`。
- R2 实施计划：`08-vr1a-r2-anatomical-shape-revision-plan.md`。

## 2. 拟批准范围

仅批准：

- Thorax。
- Abdomen。
- Upper Pelvis。
- Shoulder Girdle。
- Axilla。
- Left/Right Upper-arm Root。
- Android mode-2 局部预览。
- Host tests。
- R2 真机证据。

明确不批准：

- Head/Face/Hand/Foot/Full Legs。
- mode-3 full Canonical 重建。
- 新骨骼或骨骼层级变化。
- JNI、UBO[22]、64B vertex packing。
- PBR/SSS、外部模型、morph、blendshape、训练数据。
- VR-1B 生产实现。

## 3. 对 R1 授权的有限扩展

| 区域 | 拟允许 | 明确不允许 |
|---|---|---|
| `CharacterDefinition` | 新增 typed torso control stations、shoulder form、landmark 曲线；schemaVersion=3；landmark confidence/provenance | 第二个人体定义、双数值权威、Assumed 伪标 Measured |
| `ResolvedCharacterState` | 解析并输出 control stations/landmarks 供生成器消费 | 回写可编辑定义 |
| `GeometryBuildRequest` | 完整 definition hash、detail/LOD 策略 | 修改 JNI/UI extra |
| `LocalShapeBasis` | 方向性、station/angular support、continuity 元数据 | 球形/椭球堆叠肌肉 |
| `VariableProfileSweep` | 多控制截面、曲率/特征驱动采样 | 新运行时技术栈 |
| `SurfacePatch/PatchAssembler/TopologyTemplate` | 实际接入肩带/腋窝 patch 和共享 BoundaryId | 新建同职责第二套模块 |
| `TorsoShoulderPatchGenerator` | R1/R2 version dispatch、R2 解剖曲面 | 修改 mode-3 Canonical |
| `LodCorrespondence` | parent/edge/vertex/barycentric/refinement lineage | 破坏 Stable ID 稳定性 |
| `RenderThread` | generator version 选择、mode-2 cache/诊断日志 | camera/lighting/mode-3/JNI/lifecycle 修改 |
| Tests/CMake/Evidence | R2 测试、构建和真机证据 | 删除或降低 R1 测试 |

## 4. 负责人必须单独确认的决策

### 4.1 目标形态

- [ ] 批准 `07-vr1a-r2-shape-target.md` 的候选比例和形态方向。
- [ ] 修改后再批准：________________________________。

未确认前不得把目标值写入生产默认值。

### 4.2 Neck Base / Trapezius 接口

- [ ] 方案 A（推荐）：只生成显式 `neck-base-interface` crop loop，不生成头颈动画；内部肩胸边界仍必须连续。
- [ ] 方案 B：允许最小 neck/trapezius transition ring，以保持 neck interface 封闭；不包含 Head/Neck 骨骼动画。
- [ ] 其他：________________________________。

### 4.3 R1/R2 对照方式

- [ ] 方案 A（推荐）：同一源码、同一 `CharacterDefinition`、同一 camera/lighting，以 build-time generator version 生成 R1/R2 两个证据 APK；`generatorVersion` 与 schema version 解耦，R1 显式 pin 基线版本。
- [ ] 方案 B：同一 APK 内运行时切换；需额外明确选择入口和 JNI/UI 授权。
- [ ] 其他：________________________________。

### 4.4 性能预算

R2 实验密度可能从 524/1,028 增至 2,000–4,000 vertices，不能继续把 R1 绝对时间当同工作量 20% 基线。

- [ ] 批准按相同 detail/workload 比较，并记录 `ms/1k vertices`、`ms/1k triangles`、FPS、P50/P95/P99、PSS/peak RSS。
- [ ] 批准 R2 各 LOD 的临时预算：________________________________。
- [ ] 暂不设硬预算，只记录研究数据：________________________________。

### 4.5 动作验证范围

当前 synthetic actions 只有左臂抬起、左臂弯曲和双臂外展，没有右臂单侧抬起。

- [ ] 接受右臂单侧动作 `NOT VERIFIED`，使用双臂外展和左右权重测试覆盖右侧。
- [ ] 额外授权最小 deterministic 右臂单侧动作驱动：________________________________。

### 4.6 Git 权限

- [ ] R2 验证完成后允许一个本地 commit。
- [ ] R2 验证完成后允许 push 到 `origin/poc-sm/shape-model-v2`。
- [ ] 不允许 commit/push，仅交付工作区文档/代码。

任何 merge、rebase、force push、reset、clean 或覆盖未提交工作均不在本授权内。

## 5. 冻结区域

始终禁止修改：

- `native/skeleton/**`。
- BoneId 0–21 与骨骼层级。
- `UBO[22]`。
- 64-byte GPU vertex packing。
- JNI ABI 与 `app/src/main/cpp/jni_bridge.cpp`。
- `native/procedural/humanoid/**`。
- `native/renderer/gl/**`。
- `AGENT.md`、ADR、DEC、CAS、Gate。
- `.workbuddy-ai/`。
- `evidence/character-v6/**`、`evidence/character-v7/**`。

## 6. 验收要求

- R2 必须相对 R1 在胸廓、腰腹、肩带、腋窝、上臂根和曲面质量上有可测量改善。
- 至少八视角、Shoulder/Chest/Axilla 近景、Clay/Wireframe/Normal/Weight。
- Bind、左臂抬起、双臂外展；右臂单侧动作按 §4.5 决策，未授权时保持 `NOT VERIFIED`。
- components/boundary/non-manifold/winding/normal/self-intersection/Top-4/Stable ID/LOD。
- 全局二面角、BoundaryId seam、曲率、sliver、triangle quality 分开报告。
- detail 0/1/2 必须产生不同且有效的网格。
- cache hash 必须覆盖所有 shape/landmark 字段。
- mode-3 无回归。
- 性能按 §4.4 决策记录，不自动宣称 guardrail PASS。
- AI 不宣布 Gate PASS 或视觉验收通过。

## 7. 停止条件

- 本授权未正式签发。
- 目标形态未确认。
- 需要修改冻结区域。
- 参数/landmark 仍不控制 mesh。
- 肩带/腋窝 patch 折叠、自交或 Jacobian 退化。
- LOD/Stable ID 失配。
- 权重串扰或腋窝动作塌陷。
- cache/skeleton 不一致。
- 视觉仍为桶状躯干、平板肩盖或圆管。
- 性能超出负责人确认的预算。

## 8. 签署

授权状态：**`DRAFT — NOT EFFECTIVE`**

负责人确认：________________________________
日期：________________________________

AI 转录/草拟：QoderCN
日期：2026-11-25

本授权草稿不修改、不替代、不扩大既有 `AUTH-P1-VR-01`。
