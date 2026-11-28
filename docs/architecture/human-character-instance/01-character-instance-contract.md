# Character Instance — Unified Data Contract (M1 ↔ M2 ↔ M3)

Status: `READY_FOR_REVIEW` · Date: 2026-11-28
Authority: [`MASTER-PLAN`](../../roadmap/PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md) §4。本文件是 M1/M2/M3 唯一接口权威；代码可并行开发，接口以本文件为准。
关联：[`human-rig-v2/01-rig-v2-architecture.md`](../human-rig-v2/01-rig-v2-architecture.md)。

## 1. 数据类型（10 项）

### 1.1 HumanTemplateIdentity
- `templateVersion`（PAHT v3）、`baseMeshSha256`、`shapeTargetSetHash`、`sourceCommit`（CC0 `a8bc2d54`）、`vertexCount`（13,380）、`stableVertexIdRule`（OBJ `v` 行序 0 基，永不重排）。
- 现有载体：`native/procedural/human_template/PAEHumanTemplateFormat`。

### 1.2 ShapeParameterVector
- `params: map<semanticKey, alpha>`（shoulderWidth/ribcageWidth/chestDepth/waistWidth/hipWidth/armShape…），`statureMode`（uniformScale 锚点缩放，**非**单 macro height），`targetStatureM`。
- alpha 范围：各 Target 实测有效范围（当前 [-1,+1]），**未经验证不得扩展**。保留 incr/decr 非对称。

### 1.3 ResolvedShapeState（M1 输出）
- `deformedFullPositions`（含 helper/joint）、`deformedBodyPositions`、`actualMeasurements`（见 1.4）、`alphaEffective`（钳制后）、`geometryQuality`（flips/selfInt/edge/volume）、`profileHash`。
- 所有求解从固定 Base Mesh 开始，**不叠加上一次输出**。

### 1.4 AnthropometricMeasurementReport
- Height / ShoulderBreadth / RibcageBreadth / ChestDepth / WaistBreadth / WaistCircumference / HipBreadth / HipCircumference。
- 每项标注 `caliber`（PAE 自定义截面协议 vs ANSUR 参考）；围度用环分离躯干环（WP3-R1 已修）。区分 PAE 测量与 ANSUR 参考人群（1.6659 vs 1.7549 口径差）。

### 1.5 ResolvedJointSet（M2 输出）
- 每关节 XYZ（形态后解析）、每骨 head/tail、rest 朝向、骨长、L/R 对称诊断、未解析清单。
- **区分** SurfaceLandmark / BoneJointCenter / RigJoint / GeometryAnchor。**肩峰表面 landmark ≠ ShoulderJointCenter**（需解剖偏移模型，M2-B 待完善）。
- 现有载体：`wp4::JointSet`（CC0 joint 组质心 + root 锚点）。

### 1.6 HumanRigDefinition
- stable semantic bone id / parent id / role / head-tail / rest transform / orientation-roll / deformation role / rig profile version。
- **禁止 GPU palette index 充当 semantic id。** profiles：Core20 / Full92（数量不写死）。
- 现有载体：`wp4::RigV2` + authored 表。详见 rig-v2 架构。

### 1.7 SkinWeightSet
- Full + Top-4；每顶点影响骨列表；coverage / discardedMass / regionLeakage / normalization / joint-region continuity。
- `generator`（pae-distance/heat/region）、`weightVersion`。**不得仅以归一化宣布质量通过。**
- 现有载体：`ht::SkinWeights` + `wp4::diagnoseWeights`。

### 1.8 PoseState
- 每骨 Bone Local Transform（semantic id + 局部轴 + 角度）；root motion；blend 权重（M5）。
- 现有载体：`wp4::Pose`。

### 1.9 DeformedCanonicalMesh
- 位置 + 法线 + stable vertex id 映射；每次从 Rest 出发，不叠加。
- 现有载体：`wp4::skinPositions/skinNormals`。

### 1.10 RenderMesh
- 三角化（0-2 对角）+ 法线 + UV + 骨骼/权重属性（M4 动态布局）；LOD（后续）。
- 现有载体：`BaseHumanRenderMeshBuilder`（静态）；动态布局 M4 定义。

## 2. 版本与哈希（所有数据必带）

`templateVersion` · `baseMeshSha256` · `shapeTargetHash` · `rigVersion` · `weightVersion` · `parameterProfileHash`。

## 3. 变化与缓存失效规则（强制）

1. **Shape 变化 → 关节必须重解析**（JointResolver 在变形后表面运行）。
2. **权重是否复用由已验证规则决定**，不得因模板拓扑相同默认复用。当前规则：形态改变→权重重生成（WP4 即如此）；复用需误差阈值验证（M3 定义）。
3. **不同 Rig/Weight 版本不得共享缓存。** 缓存键 = `hash(baseMesh)+hash(shapeVector)+rigVersion+weightVersion+profileHash`。
4. 几何缩放锚点（pivot=(rootX,soleY,rootZ)）≠ RootJoint；世界根 ≠ 骨架局部原点（rig-v2 架构 §4.3）。

## 4. M3 整合输出

`CharacterInstanceBuilder` → `CharacterInstance`（含上述全部状态 + 缓存）→ `CharacterInstanceQualityReport`。须证明换形态后关节/权重不停留旧人体；≥3 体型同动作。

## 5. 现有代码映射（Host）

| 契约类型 | 现有 Host 实现 |
|---|---|
| ResolvedShapeState | `wp3_experiment` applyVariant + `wp3_measure` |
| ResolvedJointSet / HumanRigDefinition | `wp4_rig` |
| SkinWeightSet | `wp4_weights` + `ht_rig` 生成器 |
| PoseState / DeformedCanonicalMesh | `wp4_poses` / `wp4_skin` |
| RenderMesh | `BaseHumanRenderMeshBuilder`（静态） |
| CharacterInstanceBuilder | **M3 待实现** |