# VR-1B / 08 — M7 Appearance & Character Presentation Plan

Status: `M7_APPEARANCE = PLANNED` · Date: 2026-11-28
Authority: [`MASTER-PLAN`](../../roadmap/PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md) §10。

## Goal
独立于几何/骨骼的程序化表现：材质、头发、服装、灯光达到自然半写实。**服装/头发不得遮盖人体几何与动作缺陷。**

## Scope
Procedural Material Baker（Albedo/AO/Roughness/Metallic/Height/Normal）；皮肤（Color/Roughness/MicroDetail/Pore/SSS 近似）；Hair Cards/Strands；Procedural Clothing。

## Dependencies
M6 几何/变形稳定（表现不掩盖缺陷）。Saltline 借鉴范围需先研究（GLSL surf(uv)/Triplanar/UV/Texture Density/Seed/Cache/Material Profiles）——**借鉴范围研究尚未做**。

## Allowed / Forbidden Paths
授权后白名单（材质 baker / 渲染 shader，独立路径）。冻结边界同 M4。不得用未审核外部纹理资产。

## Input / Output Contract
输入：RenderMesh + UV + Material Profile + Seed。输出：PBR 贴图/程序化材质 + 渲染图。

## 实施内容
- Material Baker：可缓存、可复现（Seed）、Texture Density 一致。
- 皮肤 SSS 近似、毛孔微细节。
- 头发 cards/strands；程序化服装。

## Tests / Acceptance（M7 Gate）
**分别验收 Clay / Material / Final Lighting** 三档；材质不掩盖几何缺陷（对照 Clay）。
## Evidence
三档渲染对比、材质 profile、缓存/确定性日志。
## Stop Conditions
材质掩盖几何/动作缺陷；Saltline 许可证未审核。
## Owner Sign-off
负责人三档人眼验收。

## 未解决
Saltline 借鉴范围与许可证；SSS 近似方案；头发/服装与变形网格的绑定。