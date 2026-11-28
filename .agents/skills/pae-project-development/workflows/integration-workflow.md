# Integration Workflow

适用：依赖 WP 完成后的集成（EL-11）。

1. **前置**：所有依赖 WP 已 VERIFIED。核对各 WP 输出契约版本。
2. **集成检查**：数据契约 / 坐标空间 / 计量单位 / Stable Vertex ID / Bone Semantic ID / Weight Format / Version Hash / Cache Invalidation / Error Handling / Memory / Performance / Regression。
3. **四层集成**：单模块 → 两模块接口 → 子系统 → 完整阶段。逐层验证。
4. **失败定位**：定位具体模块或接口，**不直接覆盖其他 Agent 工作**。同一生产文件唯一权威写入者。
5. **回归**：全量测试（WP4/WP3/Host/CTest），核对当前测试目标不沿用历史数字。
6. **报告**：用 [../templates/integration-report.md](../templates/integration-report.md)。
7. **Gate**：集成 Gate 评估，人工 Gate 不代签。

多 Agent 汇合须按 agent-collaboration 的契约/白名单/写入所有权/集成负责人执行。