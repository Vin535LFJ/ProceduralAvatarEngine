# New Feature Workflow

适用：新增功能/参数/能力。强制走 EL-01..EL-14，不直接改代码。

1. **Preflight + 授权**（READ_ONLY 起步）：核对 Git/白名单/其他 Agent。读 SKILL.md §0 权威文件。
2. **EL-01 Intake**：用 [../templates/requirement-intake.md](../templates/requirement-intake.md) 记录意图/约束/未知/初始授权。不替用户定技术路线。
3. **EL-02 Analysis**：读当前代码与接口（如新体型参数→查 ShapeParameterVector/CharacterDefinition/现有参数接口）。区分事实/期望/假设。
4. **EL-03 Specification**：用 [../templates/requirements-specification.md](../templates/requirements-specification.md) 写可验证 REQ-ID + 验收准则。
5. **EL-04/05 归属**：判断属于 M1–M8 哪个阶段（查 MASTER-PLAN）。属现有范围→更新对应阶段计划/WP；超出→提路线调整 ADR 提案，不建平行路线。
6. **EL-06 Phase Planning**：用 [../templates/phase-development-plan.md](../templates/phase-development-plan.md) + [../templates/phase-readiness-review.md](../templates/phase-readiness-review.md)。未达 Entry Criteria/未授权不开始生产实现。
7. **EL-07 WP Design**：用 [../templates/work-package-specification.md](../templates/work-package-specification.md)，定输入/输出契约/白名单/验收/停止条件。高风险数学/算法先做最小可复现实验（[../templates/experiment-plan.md](../templates/experiment-plan.md)）。
8. **EL-08 Task Breakdown**：用 [../templates/task-checklist.md](../templates/task-checklist.md) 拆有序 Task。
9. **EL-09 逐点实现**：每 Task 最小修改 + 验证 + Diff + 日志。不一次性大改，不同时改多变量。
10. **EL-10 证据**：按 [../references/test-and-evidence-protocol.md](../references/test-and-evidence-protocol.md) 生成本轮真实证据（参数影响/Before-After/固定 Mask）。
11. **EL-11/12 集成 + Gate**：用 [../templates/gate-assessment.md](../templates/gate-assessment.md)。AI 评估，人工 Gate 不代签。
12. **EL-13 交付**：用 [../templates/implementation-report.md](../templates/implementation-report.md) + [../templates/agent-handoff.md](../templates/agent-handoff.md)，更新 STATUS。提交需授权（见 authorization-and-safety）。

检查负结果库避免重复失败。全程标记授权级别。