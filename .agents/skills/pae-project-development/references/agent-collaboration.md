# Agent Collaboration

## 角色模型（按复杂度启用）

Requirement Analyst / System Architect / Technical Researcher / Project Planner / Phase Planner / WP Designer / Implementer / Test Engineer / Visual QA / Integration Engineer / Independent Reviewer / Release Owner。

小任务单 Agent 可兼多角色；复杂高风险任务实现与审查分离。

## 多 Agent 并行原则（并行前必须定义）

WP 分工 / 输入输出契约 / 共享类型 / 文件白名单 / 禁止修改路径 / 依赖顺序 / 写入所有权 / 集成负责人 / 交接标准。

**同一生产文件同一时间只能有一个权威写入者。** 不得让多个 Agent 同时在同一工作树修改相同文件。能同时工作≠技术上可并行（须稳定契约 + 独立验证）。

## Handoff Protocol（每次交接至少提供）

Agent Role / Task ID / WP ID / Baseline SHA / 当前状态 / 已修改文件 / 未修改文件 / 输入输出契约版本 / 测试命令与真实日志 / 新生成证据 / 失败项 / 未验证项 / 下一步工作 / 禁止继续修改的文件。

目标：新 Agent 仅靠仓库内容即可继续，不依赖前一个 Agent 的隐藏会话记忆。模板：[../templates/agent-handoff.md](../templates/agent-handoff.md)。

## Independent Review

Review Agent 必须核对：被审提交 SHA / 当前分支 / 任务书版本 / 实际 Git Diff / 源代码 / 测试日志 / 证据版本 / 相关 ADR / 相关 Gate。**不允许审查旧快照后将结论直接应用于新版本。**

收到其他 AI 的评审粘贴（无指令）时，先逐条本机核实再动手。

## 集成（EL-11）

所有依赖 WP 完成后集成。检查数据契约 / 坐标空间 / 计量单位 / Stable Vertex ID / Bone Semantic ID / Weight Format / Version Hash / Cache Invalidation / Error Handling / Memory / Performance / Regression。四层：单模块 → 两模块接口 → 子系统 → 完整阶段。集成失败定位具体模块/接口，不覆盖其他 Agent 工作。