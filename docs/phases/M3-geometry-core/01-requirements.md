# M3 — 01 需求

- FUN-AV-001 前置：MeshData 顶点/索引/SubMesh/AABB 语义；图元与连续曲面生成器。
- TEST-001：Math/Geometry 单测覆盖边界、确定性、非法参数、法线、索引、退化面、权重。
- COMP-001 前置：生成数据上传 Renderer 后正确显示。
- 验收：相同参数/seed 结果一致；无 NaN/Inf、越界索引、非法 AABB、退化三角形；法线归一化；生成图元可在 GLES 画面观察。
