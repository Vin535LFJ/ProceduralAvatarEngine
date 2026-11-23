# 04 — Skinning Contract（P0 冻结）

## 权重规则

- 每个顶点最多 4 个骨骼影响；非负、和为 1（容差 1e-3），骨骼索引 ∈ [0,21]。
- P1-C 主线：Semantic Region Mask + Bone Segment Distance；区域由 `AnatomicalLandmarks`/Topology Template 的 region id 给出，骨段距离用于区域内平滑过渡。
- 禁止仅用“最近骨段路径”而不区分语义区域躯干/肩/臂交叉处的权重串扰。
- 关节过渡：肩/肘/髋/膝必须有过渡带；不允许单一骨权重硬切导致塌陷；不允许关节球代替权重过渡。

## Region → Bone 映射（初始表，P1-C 可细化）

| Region | 主骨 | 过渡骨 |
|---|---|---|
| Pelvis | Hips | Spine, UpperLeg L/R |
| Abdomen | Spine | Hips, Chest |
| Chest | Chest | Spine, Neck, Shoulder L/R |
| Neck | Neck | Chest, Head |
| Head | Head | Neck |
| Shoulder L/R | Shoulder | Chest, UpperArm |
| UpperArm L/R | UpperArm | Shoulder, LowerArm |
| Forearm L/R | LowerArm | UpperArm, Hand |
| Hand L/R | Hand | LowerArm |
| Thigh L/R | UpperLeg | Hips, LowerLeg |
| Calf L/R | LowerLeg | UpperLeg, Foot |
| Foot/Toe L/R | Foot, Toe | LowerLeg |

## LBS 与矩阵链

- `SkinPalette[i] = GlobalPose[i] * InverseBind[i]`。
- 顶点位置：`Σ w_j * (Palette[j] * position)`；法线用 `mat3(Palette[j])` 同权重变换后归一化。
- std140 UBO：`mat4 uBones[22]`，1408 字节，binding 0；列主序；C++ `BonePaletteUboData` 写入偏移 = bone*16 floats。
- CPU 参考实现 `native/skeleton/Skinning.cpp` 必须与 GLSL 公式同构；P1-C 需补 CPU↔GPU 数值对照（当前 `skinning_tests.cpp` 仅验证 CPU 自洽与 UBO 布局，状态为 NOT VERIFIED）。

## 调试视图

- Weight Heatmap：按主导骨或权重混合着色，需可切换。
- Bone Debug：显示骨段/节点；与 wireframe/normal/gray Lambert 同为 P1-C 证据。
- 不允许仅用 toon/描边结果作为蒙皮或几何验收依据。

## 验收

- 权重和、索引、有限值由 `MeshValidator` 检查。
- 中性姿势无 pop；9+ 确定性动作（idle、arm raise、abduction、elbow 45/90、torso flex/side/rotation、hip flex、knee bend、squat、leg raise）下肩/肘/髋/膝无塌陷或翻面。
- CPU/GPU 容差在 P1-C 定义并实测；未实测前标 NOT VERIFIED。
