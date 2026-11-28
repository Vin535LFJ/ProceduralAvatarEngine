# Lifecycle & Stage Gates (EL-01..EL-14)

权威：本文件是技能操作规范；阶段验收契约见根 `AGENT.md` 与 `ADR-013`。

## 生命周期（强制，不得从 EL-01 直跳改代码）

EL-01 Intake → EL-02 Analysis → EL-03 Specification → EL-04 Decomposition → EL-05 Project Planning → EL-06 Phase Planning → EL-07 WP Design → EL-08 Task Breakdown → EL-09 Incremental Implementation → EL-10 Testing & Evidence → EL-11 Integration → EL-12 Stage Acceptance → EL-13 Release & Handoff → EL-14 Maintenance & Replanning。

小任务可合并部分流程，但须记录简化依据，保留授权/基线/验证。任何 Agent 必须能判断当前任务处于哪个阶段。

## 五层分解

Project → Phase → Milestone → Work Package → Task。Phase 按技术依赖/可交付能力/风险划分，不按代码目录。依赖图标明 BLOCKED BY / BLOCKS / CAN RUN IN PARALLEL WITH。并行须有稳定契约 + 白名单 + 独立验证（能同时工作≠技术上可并行）。

## 状态机

- WP：PROPOSED / DESIGNED / READY / AUTHORIZED / IN_PROGRESS / BLOCKED / IMPLEMENTED / VERIFIED / ACCEPTED。**DESIGNED≠AUTHORIZED；IMPLEMENTED≠ACCEPTED。**
- Task：TODO / IN_PROGRESS / PASS / FAIL / BLOCKED / NOT VERIFIED。
- Gate：PASS / FAIL / PARTIAL / NOT VERIFIED / BLOCKED。
- Phase：NOT STARTED / PLANNED / READY / IN PROGRESS / BLOCKED / IMPLEMENTED / TECHNICAL PARTIAL / TECHNICAL PASS / VISUAL PENDING / ACCEPTED。

## Phase Readiness Review（进入生产实现前）

前置阶段通过？依赖接口明确？任务可执行？需先做技术实验？无未解决 P0 风险？有测试工具？有文件白名单？已获负责人授权？未达 Entry Criteria 不得开始生产实现。

## Gate 类型

Functional / Architecture / Test / Evidence / Performance / Visual / Compatibility / Safety-Licensing。AI 提供 Gate Assessment，**不代签需人工批准的 Gate**。Gate 失败只能：修复当前 WP / 返回设计 / 增补充 WP / 重划阶段 / BLOCKED / 经负责人批准修订标准。禁止无记录带失败进入下一阶段。

## 逐点实现（EL-09）

每 Task：读源码→核对契约→核对前置→确认白名单→最小修改→修改→跑测试→查 Diff→存日志→更新状态→决定下一步。失败存证据、停依赖任务、析根因、不降标准。不得同时改多个独立变量。

## Replanning（EL-14）

触发：假设证伪 / 发现根因 / 架构不满足 / 前置失败 / 接口变化 / 性能设备限制 / 许可证风险 / 用户调整目标。记录原计划/触发/新证据/调整/影响阶段与 WP/接口变化/是否需批准。保留原计划历史，不静默覆盖。