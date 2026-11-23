# P1-A — 02 Design

- 新增 `native/procedural/character/`，按“少模块、真几何”原则只落 3 个文件；下列是该模块内部的逻辑职责，不各建一个类：
  - `CharacterDefinition.h`：版本化参数与 detail level。
  - `BodyPatchGenerator.h`：生成入口、boundary/拓扑报告结构。
  - `BodyPatchGenerator.cpp`：Anatomical Landmarks 派生、Topology Template / boundary 契约、躯干 Surface Patch、肩胸 bridge、Variable-profile Sweep（非圆截面 + PTF + 面积加权法线）、Patch Assembler（复用顶点索引缝合）、权重输出。
- 躯干是多截面非圆 loft；肩胸 bridge 从躯干顶部边界弧连续过渡到上臂起始环；上臂到前臂是多截面 sweep，截面宽深与形状指数沿程变化，法线由三角面面积加权计算。
- 权重：躯干→Spine/Chest；肩桥→Shoulder/UpperArm；上臂/前臂→UpperArm/LowerArm；供关节动作验证。
- 渲染：新增 previewMode=2，复用现有 gray clay / normals / wireframe shader 和 camera preset；SDF anatomy 保持 previewMode=1，不删除。
