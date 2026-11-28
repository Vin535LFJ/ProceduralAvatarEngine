# PAE Architecture Map

权威来源（本文件仅导航 + 摘要，不复制）：
- `docs/architecture/target-architecture.md`、`docs/architecture/module-contracts.md`
- `docs/architecture/human-character-instance/01-character-instance-contract.md`
- `docs/architecture/human-rig-v2/01-rig-v2-architecture.md`
- `docs/architecture-decisions/ADR-014-offline-compiled-human-base-topology.md`

## 数据流（当前主路线）

```
CC0 Base Topology + Shape Targets (离线编译, ADR-014)
  → PAEHumanTemplate (PAHT v3)
  → ShapeParameterization (M1: Forward Model + Inverse Solver)
  → ResolvedShapeState
  → JointResolver (形态后重解析关节)
  → HumanRigV2 (Full92 / Core20, PAE 语义骨骼)
  → WeightGenerator (Region-constrained, Top-4)
  → Skinning (CPU LBS; Bone Local Transform)
  → DeformedCanonicalMesh → RenderMesh
  → Android Runtime (M4, 独立动态路径)
  → Animation / IK (M5)
  → Anatomy / Appearance (M6/M7)
  → Final Acceptance (M8)
```

## 核心契约（详见 human-character-instance 契约）

数据类型：HumanTemplateIdentity / ShapeParameterVector / ResolvedShapeState / AnthropometricMeasurementReport / ResolvedJointSet / HumanRigDefinition / SkinWeightSet / PoseState / DeformedCanonicalMesh / RenderMesh。

所有数据携带：Template Version / Base Mesh SHA / Shape Target Hash / Rig Version / Weight Version / Parameter Profile Hash。

缓存失效：Shape 变化→关节必重解析；权重是否复用由已验证规则决定（不因拓扑相同默认复用）；不同 Rig/Weight 版本不共享缓存。

## 关键坐标/数学约定（详见 rig-v2 架构 + M2 坐标审计报告）

列向量；Affine.apply(p)=M·p+t；(A*B) 先作用 B。restGlobal=T(head)·R（R 列=ax/ay/az 为局部→世界）。restLocal=restGlobal[parent]⁻¹·restGlobal。

**三种轴不可混用**：Bone Local Axis (1,0,0)/(0,1,0)/(0,0,1)；Bone Rest World Axis rig.ax/ay/az=R·局部轴；Bone Posed World Axis=父运动后的世界方向。

**已修复缺陷（M2-POSE-FRAME-AUDIT-01）**：poseGlobals 右乘的局部旋转 rl 必须用局部单位轴，不能用世界轴 rig.ax/ay/az（否则 skin 绕 R·axis 而非 axis 旋转，非轴对齐骨骼出错）。

## 冻结边界（Legacy）

LegacyRigV1 / BoneId 0–21 / UBO[22] / 64B Vertex Packing / 旧 JNI ABI / mode-3 Canonical / mode-4 静态默认 / PAHT v3 字段语义。新动态路径独立，不改这些。

## 模块代码位置（Host 研究）

`tools/human-template-poc/host/src/`：ht_*（基础库）、wp1_compiler（PAHT 编译）、wp3_*（形态）、wp4_*（骨骼/蒙皮/坐标审计）、wp6_finger（手指细化）。生产模板格式在 `native/procedural/human_template/`。Android 在 `app/`、`native/renderer|runtime/`。