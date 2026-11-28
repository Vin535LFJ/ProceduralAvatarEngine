# PAE Development Roadmap (导航)

**唯一权威总计划：`docs/roadmap/PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md`（M1–M8）。** 本文件仅导航，不创建第二份路线图。`PAE-PROCEDURAL-HUMAN-GENERATION-ROADMAP.md` 已被取代，仅历史快照。

## M1–M8 概览（详见 MASTER-PLAN）

- M0 Human Template Foundation（ACCEPTED）
- M1 Human Shape Parameterization（WP3-R2 求解器）
- M2 Human Rig & Skinning（Rig V2 / 关节 / 权重 / CPU LBS）
- M3 Integrated Character Instance
- M4 Android Dynamic Human Runtime（须 M3 + 授权）
- M5 Procedural Animation & IK
- M6 Anatomical Visual Refinement
- M7 Appearance（材质/头发/服装）
- M8 Production Quality & Final Visual Gate

## 依赖与并行

M1 与 M2 Host 可并行（共享 CharacterInstance 契约 + 文件白名单 + 独立验证）。M3 汇合 M1+M2。M4 须 M3 Gate + 授权。M5 须 M4 稳定。M6/M7 几何变形稳定后。M8 最终。

## 阶段计划位置

`docs/phases/VR-1B/`：01 WP3-R2 计划、02 WP4 计划、02a M2 坐标审计报告、03 集成契约、04 M3、05 M4、06 M5、07 M6、07a M6 手指负结果、08 M7、09 M8。

## 新任务归属规则

属于 M1–M8 范围 → 优先更新对应阶段计划/WP。超出范围 → 先提路线调整方案（ADR 提案），不自行创建平行产品路线图。

## 当前待授权方向（不得自动实施）

M6-FINGER-SEPARATION-02（指间分离拓扑 + FingerId 约束权重 + 自然握拳 MCP/PIP/DIP + 关节限位）；DQS/Corrective 评估；M3 整合；M4 Android。