# Task Checklist — {{WP-ID}}

每个 Task 有序，满足前置才开始后续。

| Task ID | Description | Preconditions | Input | Expected Output | Allowed Files | Verification Command | Pass Criteria | Status | Evidence Ref |
|---|---|---|---|---|---|---|---|---|---|
| T-01 | | | | | | | | TODO | |
| T-02 | | | | | | | | TODO | |

Task 状态：TODO / IN_PROGRESS / PASS / FAIL / BLOCKED / NOT VERIFIED。

## 逐点实现纪律（EL-09）

每 Task：读源码→核对契约→核对前置→确认白名单→最小修改→修改→跑测试→查 Diff→存日志→更新状态→决定下一步。

失败：存证据→停依赖任务→析根因→修复/返回设计/BLOCKED→保留原始失败信息→不降标准。

**不得同时改多个独立变量**（旋转轴/Joint/Weight/拓扑/DQS 逐项验证）。