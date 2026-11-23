# P0 — 00 Scope

- 目标：把 v1.0 三份正式文档落地为当前仓库的权威基线；审计 M0–M13 现有实现；冻结 Skeleton / Mesh / Skinning / Visual / Thread 契约；处理最小安全代码问题；准备 P1-A。
- 范围：文档契约、仓库审计、ADR-010、Shoulder/UpperArm 语义修复与回归、基线构建/测试、Git 分支基线。
- 非目标：P1 几何生产、ML Kit 修复、P2 换装、P3 材质、P5 视频优化；不顺带重构 Skeleton/MeshData/Renderer。
- 用户 Gate：P0 审计报告与 ADR 提交后等待用户确认；未确认不进入 P1-A。
- 权威路径差异：用户引用的 `Procedural_Character_Foundation_v1.0/` 子目录不存在，实际四份文档在仓库根目录，仅有同名 zip；已记入审计。
