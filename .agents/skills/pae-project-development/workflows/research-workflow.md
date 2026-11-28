# Research Workflow

适用：技术研究/实验（RESEARCH_ONLY，不直接进生产）。

1. **Preflight + 授权**：RESEARCH_ONLY。核对 Git/白名单（通常 Host 工具目录）。
2. **查负结果库**：[../references/known-failures-and-anti-patterns.md](../references/known-failures-and-anti-patterns.md)。已失败的方案不重复（如统一细分治握拳）。
3. **EL-02 假设**：明确 HYPOTHESIS + 可证伪判据。区分已确认事实与假设。
4. **实验设计**：用 [../templates/experiment-plan.md](../templates/experiment-plan.md)。最小可复现；固定对照条件；单变量；Before/After。
5. **执行 + 证据**：生成本轮真实数据/图/日志。负结果必须保存。不移动 Mask 掩盖。
6. **结论分级**：CONFIRMED（有测试/数据）vs HYPOTHESIS（未证实）。不夸大。
7. **报告**：用 [../templates/implementation-report.md](../templates/implementation-report.md) 记录背景/方法/数据/结论/局限/下一步。
8. **路线影响**：若证伪现有假设或发现根因→触发 EL-14 Replanning（[../templates/replanning-record.md](../templates/replanning-record.md)）。研究建议≠架构决定；新方向需 ADR 提案 + 负责人批准，不自动升级。

研究产出不得自动进入生产实现；实施需单独授权。