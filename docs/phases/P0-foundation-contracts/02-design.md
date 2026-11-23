# P0 — 02 Design

- 审计状态机：`PRELIMINARY FINDING → CODE AUDIT → REPRODUCTION TEST → CONFIRMED ISSUE → FIX / ADR`。bindLocal 与 UV 属 PRELIMINARY，不直接当缺陷。
- Shoulder/UpperArm：Shoulder = chest→shoulder（肩带/锁骨），UpperArm = shoulder→elbow；BoneId 0–21 冻结；同步修正 Synthetic shoulder landmark 取 bone.tail，Deltoid 权重归 UpperArm。
- 回归层次：单元测试（语义、IBM、权重和、肩区权重）、动作测试（motion_tests）、真机 GPU Skinning 定性对照（历史 M11 armraise vs P0 修复后截图）。
- 最小安全修改：仅改 `Skeleton.cpp` 两处 set、`SyntheticPoseProvider.cpp` landmark 取尾点、`AnatomyField.cpp` 两行（trapRbBelly 字段与 Deltoid regionBone），不重构渲染/网格结构。
- 契约文档：docs/foundation/00–06；索引 docs/ADR_INDEX.md、docs/RTM.md、docs/RISK_REGISTER.md。
