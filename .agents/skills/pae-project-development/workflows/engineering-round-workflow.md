# 工程轮工作流（Skill + Tools + MCP 协同）

一个完整 PAE 工程轮的可运行流程。机械环节由 `scripts/pae_round.ps1` 固化执行；治理判断由代理按 SKILL.md 负责。三层组合见 [references/tool-and-mcp-integration.md](../references/tool-and-mcp-integration.md)。

## 前置（强制，不可跳过）

1. 加载 SKILL.md，执行 Repository Preflight（只读）。
2. 判定授权级别（默认 READ_ONLY）与 EL 阶段。
3. 读相关权威文件（AGENT.md / ADR / MASTER-PLAN / STATUS / 契约 / 计划）。
4. **确认负责人授权范围**：本轮改什么、白名单、是否允许 commit/push。建议≠授权。

## 阶段流程

### A. 基线（EL-02，只读）
```powershell
& $S -Stage baseline -OutDir $Out
```
核对 HEAD/分支/noreply/ahead-behind/paht 哈希/已跟踪改动/其他代理近期写入。工作区不干净时记录，不静默覆盖。

### B. 实现（EL-09，代理负责，脚本不替代）
按 Task 逐点改：读源码→核对契约/白名单→最小修改→跑指定测试→查 Diff→存日志。**单变量、不降标准**。此阶段是代理的判断工作，脚本只做机械验证。

### C. 机械验证核心（EL-10，只读）
```powershell
& $S -Stage round -OutDir $Out   # baseline + build + test + boundary
```
构建 0 警告、测试全过、生产路径零 diff、paht 不变。任何失败→存证据→析根因→修复或 BLOCKED，不降标准。

### D. 证据组装（EL-10）
生成本轮真实证据（固定视角 Clay/Wire/Normal/Closeup/CSV/日志/Before-After）。PNG 重压缩：
```powershell
& $S -Stage recompress -ImageSrc $Out\images -ImageDst $Out\images-compressed
```
像素往返校验通过后才入库。负结果必须保存。

### E. 集成与 Gate（EL-11/12）
四层集成；独立 Gate 评估。技术 PASS≠视觉 PASS；人工视觉 Gate 由负责人签，AI 不代签。

### F. 提交推送（EL-13，须负责人批准）
获批准后，代理按任务分组暂存路径，然后：
```powershell
& $S -Stage verify-staged                 # 暂存字节核对（check-attr 感知）
& $S -Stage commit -AllowWrite -MessageFile $Msg   # 核对 noreply 身份
& $S -Stage push -AllowWrite              # 快进，绝不 force
& $S -Stage verify-remote                 # ls-remote 核实 remote==local
```
无 `-AllowWrite` 时 commit/push 拒绝。提交分组/路径/授权由代理决定，脚本不自主提交。

### G. 交付（EL-13，可选 MCP 产出）
- 本地文件：`qwenwork_file_present_files`。
- 办公文档（可选）：qwoffice 把 Gate 评估写 Word、指标写 Excel、汇报写 PPT（见集成 reference §4）。
- 在线资源：`artifact_show_link`。
- 更新 STATUS（据实，不静默改写正式路线图/ADR）。

## 停止条件

- 授权不足 → 停，出授权提案。
- 测试失败/边界破坏 → 停依赖任务，析根因。
- 极端姿态/视觉缺陷触发 WP 停止条件 → 如实报告，不扩权。
- 网络抖动推送超时 → 单独 ls-remote 核实是否已推，不盲目重试。

## 触发（可选）

cron 定时跑只读 `-Stage round` 并发 IM 汇总；automation 事件触发审查。触发层不得绕过 commit/push 授权。