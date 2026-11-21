# M12 — 04 架构设计（统一隐式上半身）

## 模块结构

```text
native/procedural/humanoid/anatomy/
    AnatomyField.h/.cpp        # HumanAnatomyParameters + 各区域场函数 + smooth 组合
    HumanAnatomyGenerator.h/.cpp  # 场 → Surface Nets 网格 + 场区域权重 + 拓扑报告
native/procedural/MeshTopology.h/.cpp  # 连通组件/边界边/非流形边/退化/重复顶点分析
```

方案 A 对比样本复用现有样条工具（M11 路线），不新建文件。

## AnatomyField 场设计（核心工艺）

坐标系：米制，Y 上，+Z 面前。站姿 landmarks 复用 BodyJoints（hips 0.87 / chest 1.30 / neck 1.48 / head 1.68，肩 x ±0.21）。

区域场（全部为各向异性形变 SDF）：

| 区域 | 原语 | 关键参数 |
|---|---|---|
| RibCage | 椭球 (0, 1.20, +0.012) r=(0.185, 0.175, 0.125) | chestWidth/Depth/Volume |
| Abdomen | 椭球 (0, 1.00, +0.005) r=(0.150, 0.135, 0.100) | abdominalVolume/waistCurvature |
| Pelvis | 椭球 (0, 0.88, 0) r=(0.170, 0.115, 0.115) | pelvisWidth |
| 背部平坦化 | 大半径球 smooth-subtraction 自后方 | backDepth |
| Clavicle | 细胶囊 ×2：胸骨 (±0.02,1.39,0.055) → 肩 (±0.185,1.375,0.030) r=0.016 | clavicleWidth |
| Deltoid | 各向异性椭球 ×2 (±0.225,1.31,0) r=(0.062,0.085,0.070) 倾斜 | deltoidVolume |
| UpperArm | tapered capsule（sdRoundCone）肩→肘，r 0.050→0.044 | upperArmVolume/Length |
| Elbow | 小球 (±0.39,1.07) r=0.044 | — |
| Forearm | tapered capsule 肘→腕 r 0.047→0.036 + 纺锤中球 | forearmVolume/Length/wristWidth |
| Neck | capsule (0,1.42)→(0,1.60) r=0.048 | — |
| Head 占位 | 椭球 (0,1.70,0) r=(0.105,0.125,0.098) | 占位，不占精力 |

组合：
```text
torso = smoothUnion(ribCage, abdomen, k=0.055)
torso = smoothUnion(torso, pelvis, k=0.05)
torso = smoothSubtraction(torso, backCarveSphere, k=0.05)   # 背部平坦
shoulders = smoothUnion(torso, deltoids, k=0.10)             # 肩胸融合（k 最大）
body = smoothUnion(shoulders, clavicles, k=0.02)             # 锁骨弱脊
body = smoothUnion(body, upperArms, k=0.05)
body = smoothUnion(body, elbows + forearms, k=0.035)
body = smoothUnion(body, neck, k=0.05)
body = smoothUnion(body, headPlaceholder, k=0.04)
```
平滑次序与 k 值是形态工艺的核心：肩部 k 最大（融合感），肘/腕 k 最小（保持形体）。

## HumanAnatomyParameters（全部参与场函数）

按提示 §6.1 定义（torsoHeight…smoothness）；每个字段映射到场原语参数（如 deltoidVolume→deltoid 椭球 radii 缩放，waistCurvature→abdomen rx 调制，backDepth→backCarve 球心 z 偏移）。**禁止只定义不使用**——参数对比测试强制覆盖（deltoidVolume 0.6 vs 1.5 的肩部轮廓差 ≥ 1cm）。

## 提取与权重

- Surface Nets（复用 SdfFusion.extractSurfaceNets，扩展各向异性分辨率）：Low 64×88×40 / Med 88×129×49 / High 128×172×72（cell ≈ 6mm）
- 权重：提取后对每顶点评估区域场值 → 归属（ribcage→Chest/Spine，abdomen/pelvis→Hips/Spine，deltoid→Shoulder，arm→UpperArm/LowerArm…）→ WeightGenerator 距离权重细化（allowedBones 掩码 = 上半身骨骼集）
- 骨骼：复用 22 骨协议（Head 桩已延长），恒等=绑定不变

## Anatomy Preview Mode（RenderThread）

- 第二网格槽 anatomyMesh_（生成时上传），preview 命令切换绘制对象
- 相机预设表 8 个（VIEW_FRONT/BACK/LEFT/RIGHT/45L/45R/SHOULDER/TORSO），JNI `nativeSetCameraPreset`
- 调试视图：Shading（默认）/ Wireframe（线段索引缓冲 + flat color program）/ Normals（debug 片元 `n*0.5+0.5`）
- 预览时：跳过描边与阴影、中性材质、静态绑定姿势（poseMode -2 = 冻结 bind）
- JNI/UI：`nativeSetPreviewMode(on)`、`nativeSetCameraPreset(i)`、`nativeSetDebugView(i)`；MainActivity "Anatomy" 按钮（开关）+"View" 按钮（循环）

## 拓扑分析器（MeshTopology）

```cpp
struct TopologyReport {
    int components; int boundaryEdges; int nonManifoldEdges;
    int degenerateTriangles; int duplicateVertices; int invalidNormals;
    long triangles; size_t vertices;
};
TopologyReport analyzeTopology(const MeshData& mesh);
```
Surface Nets 输出应满足：components=1（封闭体）、boundaryEdges=0（水密）、nonManifoldEdges=0。解析测试锁定。

## Human Anatomy Report（§9）

generateAnatomyUpperBody 输出 PAE_GEOMETRY 日志：顶点/三角形/连通组件/边界边/非流形边/退化/重复顶点/法线异常/生成耗时/分辨率/关键比例（胸/腰/肩/骨盆宽度采样）。
