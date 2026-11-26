# P1-VR-1A-R2 补充授权书

文件：`docs/phases/P1-visual-recovery/09-vr1a-r2-supplemental-authorization-draft.md`
任务：`P1-VR-1A-R2 — Anatomical Silhouette & Shoulder Form Reconstruction`
日期：2026-11-25
状态：**`APPROVED FOR P1-VR-1A-R2 ONLY`**

决定来源：负责人 2026-11-25 明确授权文本。
记录方式：AI 仅转录负责人决定，不代签、不扩大范围、不宣布 Gate PASS。

## 1. 授权任务与基线

本人批准启动 `P1-VR-1A-R2` 生产代码实施。

基线：

- R1：`299ae3bf3fcb6d76be563f815aa52380e018d50b`。
- R2 设计基线：`66a7e5905dde407b46df1e76074cd8b9ea94ab79`。
- 分支：`poc-sm/shape-model-v2`。
- 绑定白名单：`10-vr1a-r2-implementation-whitelist-draft.md`。

本授权只覆盖 R2，不自动授权 VR-1B、完整 Canonical 重建或其他后续阶段。

## 2. 目标形态决定

- [x] 批准以 `Natural Semi-Realistic Adult Asian Human` 作为项目长期视觉方向。
- [x] 批准 R2 文档中的候选人体比例和解剖控制数据，作为本轮几何实验的初始目标参数。
- [x] 这些候选数据属于项目形态设计参数，不代表亚洲成年人的普遍人体测量标准。
- [x] 允许在 R2 实施过程中根据多视角视觉结果进行有记录的调整；调整必须记录原因、参数变化和视觉结果，不得未经记录地修改基准。

## 3. Neck Base 决定

- [x] 批准方案 A：保留一个明确的 `neck-base-interface` 外部 crop loop。
- [x] 本阶段不生成完整头部、颈部或头颈动画。
- [x] 肩胸、腋窝和上臂根的内部连接必须保持真实连续，不得将内部断口登记为 crop boundary。

## 4. R1/R2 对照决定

- [x] 批准方案 A：采用 build-time generator version 分别构建 R1 baseline APK 和 R2 APK。
- [x] 两个版本必须使用相同 `CharacterDefinition`、相机参数、灯光、姿态、设备和分辨率。
- [x] 不授权新增 JNI、UI extra 或运行时切换入口。

## 5. 性能决定

- [x] 沿用此前已批准的临时 20% 同设备性能 Guardrail。
- [x] 只对相同设备、相同工作量和相同测量口径的数据进行直接比较。
- [x] 无法形成等价工作量的指标只记录实际数据和归一化结果，不得据此宣称 Guardrail PASS。
- [x] 必须分别记录生成、Patch 组装、权重、GPU 上传、帧时间和峰值内存。

## 6. 动作验证决定

- [x] 本轮接受右臂单侧抬起测试保持 `NOT VERIFIED`。
- [x] 不授权修改 `SyntheticPoseProvider` 增加新的右臂动作。
- [x] 使用已批准的双臂外展、左臂抬起及左右权重隔离测试进行本轮验证。

## 7. 技术前置条件

R2 必须明确区分：

- `Acromion Surface Landmark`：表面解剖标志点。
- `Shoulder Joint Center`：骨骼关节中心。
- `Upper-arm Root`：几何连接位置。
- `Deltoid Surface`：软组织外轮廓。

不得将表面标志点直接当作骨骼关节中心。必须验证肩部几何、现有 `Shoulder Bone`、Top-4 权重和基础动作之间的兼容性。任何需要修改冻结 `Skeleton` 或 `BoneId` 的情况必须停止并重新申请授权。

## 8. 实施范围

仅批准 `10-vr1a-r2-implementation-whitelist-draft.md` 中明确列出的文件和功能。

禁止修改：

- BoneId 0–21。
- Skeleton 层级。
- `UBO[22]`。
- 64-byte GPU Vertex Packing。
- JNI ABI。
- `native/procedural/humanoid/**`。
- `native/renderer/gl/**`。
- mode-3 完整 Canonical 路径。
- Head/Face/Hand/Foot/Full Legs 生产实现。

不得导入外部人体模型、morph、blendshape、训练数据或运行时依赖。

## 9. Git 权限

- [x] 本授权只允许在指定分支开展实施。
- [x] 本授权不自动授予 commit、push、merge、rebase 或 force push 权限。
- [x] 所有修改保留在工作区；完成代码、测试和真机证据审查后，由负责人单独决定提交与推送。
- [x] 禁止 reset、clean 或覆盖未提交工作。

## 10. 验收与停止

本轮必须提供：

1. 实际代码实现。
2. Host 测试。
3. Android 真机证据。
4. R1/R2 同条件对比。
5. Mesh、Topology、Weight 和 LOD 报告。
6. 性能与内存报告。
7. 完整 Git Diff。
8. 未解决问题和失败项。

视觉验收由负责人独立确认。AI 不得自行宣布视觉 Gate PASS 或 P1-VR 完成。

停止条件包括：授权范围外修改、Skeleton/BoneId 变化、肩部几何与权重/动作不兼容、内部边界伪装为 crop、性能超预算、视觉仍为桶状躯干/平板肩盖/圆管。

## 11. 授权记录

授权状态：**`APPROVED FOR P1-VR-1A-R2 ONLY`**。
负责人决定来源：2026-11-25 明确授权文本。
AI 转录：QoderCN，仅转录，不代签。

负责人签署（如项目流程需要线下签署）：________________________________
签署日期：________________________________

本授权不修改、不替代、不扩大既有 `AUTH-P1-VR-01`，也不授权 VR-1B。
