# M12 — 01 代码审计（基于 m12-unified-human-geometry @ 67ccb39 实际代码验证）

- 日期：2026-11-22（M12-A 开始）
- 基线：main = `67ccb39`（含 M11-A..I 全部提交）；工作区干净
- 审计方法：全部结论来自 grep/读码验证，不沿用 M11 结论

## 1.1 SDF 未进入生产人体生成链路 —— **已证实**

```
grep -c "extractSurfaceNets|sdSmoothUnion|SdfFusion" native/procedural/humanoid/Humanoid.cpp → 0
```

`native/procedural/fusion/SdfFusion.{h,cpp}`（Surface Nets 提取 + smooth-union/subtraction）存在且被 `tests/unit/fusion_tests.cpp` 引用（5 处），但 `Humanoid.cpp` **零调用**。M11-B 的 PoC 证明了算法可行，M11-C 的人体重写没有采用它——SDF 是独立算法库，不是生产路径。

## 1.2 MeshData 合并 ≠ 几何融合 —— **已证实**

`Humanoid.cpp:447-448`：
```cpp
combined.vertices.insert(combined.vertices.end(), part.mesh.vertices...);
for (uint32_t index : part.mesh.indices) combined.indices.push_back(firstVertex + index);
```
人体 = ~28 个独立 Part 的顶点/索引**内存拼接**。各 Part 顶点独立、边界边裸露、部件重叠靠遮挡——无焊接、无连通性、无流形保证。顶点计数：验证器只检查权重/索引/退化，**不检查**连通组件/边界边/非流形边。

## 1.3 人体形态仍由截面曲面 + 扫掠管构成 —— **已证实**

- 躯干 = 7 个椭圆站位放掠（`sections.push_back` ×7）；前后对称椭圆 + 中心前移近似不对称——无背部平坦化、无锁骨/胸肌形体；
- 四肢 = 5-6 环样条扫掠（M11-C 已改进锥度/椭圆截面），但肩部三角肌体积 = 扫掠首环放大，无真实三角肌形体；
- 关节球已在 M11-C 移除（`addJoint` = 0）——但替代的"扫掠嵌入"在肩部仍是两段曲面的重叠遮挡，非融合。

## 1.4 M11 验收报告不可继承的问题 —— **已证实**

- M11-B PoC 三法对比只在 host 单测中比较（三角形/耗时/等参误差），**没有**在 Android Renderer 中渲染三法样本；
- M11 05 报告的"Shoulder Integration = 4"引用的是 `frame-m10-checkpoint1.png`（M10 截图）而非 M11 真机证据；
- M11-G 材质报告（roughness/SSS/AO/三档）未实际实现——片元着色器只有 toon/rim/材质分支；
- M11-I 未完成 §13 要求的完整多视角证据（只有正面固定相机）。

## 1.5 现有可复用资产（M11 保留项）

- `CatmullRom.h`（样条重采样）✓ 可复用于方案 A/C
- `generateSplineSweep/generateSplineLoft`（椭圆截面/roll/锥度法线/端盖）✓ 方案 A/C 复用
- `SdfFusion`（Surface Nets + smooth-union/subtraction + 梯度法线 + 朝向校正）✓ 方案 B 核心——**本次将真正接入生产**
- `WeightGenerator.assignWeights`（距离权重/前 4 归一化）✓ 需扩展显式骨骼掩码（SDF 网格用）
- `RenderThread` 描边/阴影/材质分支 ✓ 预览模式复用
- 骨骼 22 骨协议 + `computePose`（恒等=绑定）✓ 不变

## 1.6 M12-A 需要新建的能力（缺口清单）

1. 人体上半身 **SDF 场合成**（各解剖区域的场函数 + smooth 组合）——不存在
2. 场 → 网格的**生产提取**（各向异性分辨率 + 区域权重）——SdfFusion 只有通用局部体积
3. **拓扑分析器**（连通组件/边界边/非流形边/退化/重复顶点）——不存在
4. **Anatomy Preview Mode**（隐藏服装/发/脸、中性材质、固定相机 8 预设、线框/法线调试视图）——不存在
5. 静态绑定姿势的确定方式（当前 auto-cycle 动作会变化，证据需要固定姿态）——不存在
6. 参数化对比证据链（参数变化 → 几何真实变化的可测证明）——部分存在（eyeSpacing/shirtHem），需扩展到解剖参数

## 结论

M12-A 的技术路线（方案 B：统一隐式人体表面 + Surface Nets 提取 + 场区域权重）在现有代码基础上的缺口是明确且可实现的：SdfFusion 的提取器已验证，缺的是**人体场设计**与**生产集成**。方案 A（纯参数化）作为对比 PoC 保留（现有 spline 工具即为其实现基础）。
