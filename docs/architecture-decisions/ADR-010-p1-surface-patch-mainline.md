# ADR-010 — P1 正式人体主线：Surface Patch / Topology Template，SDF 降级为研究对照

- 状态：**Accepted（P0，2026-11-23）**，需用户 P0 Gate 确认后生效为 P1 实施约束。
- 取代范围：ADR-009 在 **P1 正式产品人体主线** 中标记为 `Superseded`；ADR-008 中“SDF 肩部补丁作为正式方案”的部分标记为部分失效。ADR-008/009 的历史数据与 M12/M13 证据完整保留。

## 背景

M12/M13 采用统一 SDF 场 + Surface Nets 生成上半身，实测 components=1、boundary=0、nonmanifold=0，真机约 51 FPS，解决了 M11 “MeshData 合并≠几何融合”的连通性问题。但 v1.0 Specification §3.2/§4.1 与 Execution Prompt §2/§3.3 明确：P1 最终动画人体必须以 Rig-aware Anatomical Landmarks、Procedural Surface Patches、Topology Templates、Variable-profile Sweeps 为主线；SDF/Surface Nets 不得成为最终动画人体网格的唯一生产来源，也不得用椭球/胶囊拼接或 MeshData append 冒充连续融合。SDF 仍允许用于局部形态实验与辅助研究。

## 候选

1. 继续 M12/M13 统一 SDF 作为 P1 最终人体唯一主线：拓扑质量好，但作为唯一生产来源与 v1.0 规范冲突，且 Surface Nets 产生不规则拓扑，权重与 UV/seam 契约难标准化。
2. 回到 M11 spline sweep + 关节球拼接：参数化友好，但历史证据显示肩胸多分量、靠重叠遮挡，视觉与拓扑不达标。
3. Surface Patch + Topology Template + 非圆多截面 Sweep + 边界缝合：符合 v1.0 主线；patch 边界与权重区域可标准化；需要重新实现并做视觉验证。

## 决定

选择方案 3 作为 P1 正式几何主线：

- 新增 `native/procedural/character/` 生成器族：`CharacterDefinition`、`AnatomicalLandmarks`、`TopologyTemplate`、`SurfacePatch`、`VariableProfileSweep`、`PatchAssembler`、`BodyPatchGenerator`。
- SDF / Surface Nets：保留为研究与对照路径；允许用于局部形态实验、复杂轮廓辅助、局部融合 PoC、离线或初始化阶段的形态修正研究；**不得作为 P1 最终动画人体网格的唯一生产来源**。
- M12/M13 现有实现与证据全部保留，不删除、不静默替换；`native/procedural/humanoid/anatomy/*`、`native/procedural/fusion/*` 继续保持独立、可切换。
- P1-A PoC 范围：躯干、肩胸连接、单臂+肘部（随后镜像）；必须通过 patch boundary、PTF、法线、拓扑与真机 gray Lambert/wireframe/normals 验证。
- 用户 P1-A Gate 未确认前不扩展 P1-B。

## 理由与影响

- 理由：遵守 v1.0 正式基线；保留可复用的参数化曲面/PTF/材质/骨架/渲染资产；避免把历史 SDF 研究数据包装成规范验收。
- 影响：P1-A 需要新增代码和视觉证据；M12/M13 证据仍可作为对照，但不能替代 P1 Gate。
- 回滚：若用户后续明确批准改回 SDF 主线，需要新 ADR 并在 P1 Gate 前说明；当前实现不得静默混用两条主线。

## 验证

- P0：`docs/foundation/01-current-repository-audit.md` 记录冲突；`ctest` 8/8 与 `assembleDebug` 基线通过。
- P1-A 验证计划：patch boundary 契约测试、非圆截面参数测试、PTF 连续性、MeshValidator、MeshTopology 报告、真机固定相机截图、用户视觉确认。
- 未验证：P1-A 视觉方向（Gate 状态 NOT VERIFIED）。

## 回滚路径

- 代码层：`previewMode_` 保持 SDF anatomy 与 P1 新主线隔离；新生成器失败时不影响既有 M12/M13 预览。
- 契约层：若本 ADR 被否决，恢复 ADR-009 主线并重新走 P0 Gate；不得直接修改报告掩盖。
