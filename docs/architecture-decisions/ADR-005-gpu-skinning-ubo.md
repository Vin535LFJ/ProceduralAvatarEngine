# ADR-005：GPU Skinning Buffer 与骨骼数量限制

- 日期：2026-11-21
- 状态：accepted
- 背景：GLES 3.0 设备上 `uniform mat4[]` 数量限制不一，需稳定上传骨骼矩阵。
- 候选：(A) 普通 uniform 数组；(B) std140 UBO 上传 Bone Palette；(C) CPU Skinning 每帧更新 VBO。
- 决定：采用 (B)。22 根主骨，`std140` mat4 步长 64 字节，每顶点最多 4 影响；CPU 蒙皮仅作测试对照；非均匀身材变化烘焙进几何而非骨骼矩阵。
- 理由：UBO 是 GLES3 标准能力，容量与布局可预测；避免每帧顶点上传。
- 影响：C++ 写入与 GLSL 布局须有单元测试；骨骼数增加需重新核算 UBO 大小与设备限制。
- 回滚/替代：不支持 UBO 的设备直接判为不满足最低能力，不做隐式降级。
