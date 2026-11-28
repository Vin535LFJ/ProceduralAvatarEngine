# Tool 与 MCP 集成（Skill + Tools + MCP 三层工作流）

本文件说明 PAE 工作流如何把 **Skill（规则）+ Tools（执行）+ MCP（扩展产出）** 组合成可运行流程，以及编排脚本 `scripts/pae_round.ps1` 的定位与安全边界。技术事实仍以仓库权威文件为准（见 SKILL.md §0）。

## 1. 三层角色

| 层 | 角色 | 在本工作流中 |
|---|---|---|
| **Skill** | 程序性知识/治理规则，加载进上下文指导决策 | `pae-project-development`：EL 生命周期、授权冻结、单变量、不降标准、证据协议、负结果库 |
| **Tools** | 宿主内置、直接调用的执行能力 | bash/git、read/write/edit/grep、task 子代理、cron（定时）、automation（事件触发）、router（媒体）、web_search |
| **MCP** | 上游工具服务器（懒加载目录发现） | 当前仅 `qwoffice`（Word/Excel/PPT 读写编辑）；用 `qwenwork_mcp_tool_list/get/call` 发现与调用 |

**关键区分**：Skill 是"怎么想/守什么规则"（程序性指南，markdown）；Tools/MCP 是"动手做"。让工作流"真正跑起来"= 把 Skill 的步骤接到 Tools/MCP 的实际调用上。`pae_round.ps1` 就是机械步骤的固化执行体。

## 2. 组合模式

```
[触发] 手动 / cron 定时 / automation 事件
   ↓
[Skill] 加载 pae-project-development → 强制授权检查、EL 阶段、单变量、不降标准
   ↓
[Tools] pae_round.ps1 机械阶段：baseline → build → test → boundary
        → recompress 证据 → verify-staged → (授权后) commit → push → verify-remote
   ↓
[MCP]  qwoffice 把 Gate 评估写 Word、指标写 Excel、汇报写 PPT（可选交付物）
   ↓
[交付] qwenwork_file_present_files（本地文件）/ artifact_show_link（在线资源）
```

## 3. 编排脚本 `scripts/pae_round.ps1`

**只自动化机械、确定性、可重复的环节**。授权判断、单变量纪律、不降测试标准、人工视觉 Gate **仍由代理/负责人负责**，脚本不替代。

阶段（`-Stage`）：

| 阶段 | 作用 | 写安全 |
|---|---|---|
| `baseline` | HEAD/分支/noreply 身份/ahead-behind/paht 哈希/已跟踪改动/近期写入（其他代理检查） | 只读 |
| `build` | 调 `tools/.../build.ps1`，统计 warning/error | 写 build 目录（git-ignored） |
| `test` | wp3/wp4/ht 测试 + ctest，采集日志与汇总 | 只读（scratch 在 OutDir） |
| `boundary` | 生产路径零 diff + paht 哈希不变 | 只读 |
| `recompress` | 调 `recompress_png.py`（像素往返校验），需 `-ImageSrc/-ImageDst` | 写新目录，不删源 |
| `verify-staged` | 暂存字节核对（按 `check-attr text` 区分 `-text` 用 `--no-filters`） | 只读 |
| `commit` | `git commit -F`，核对 noreply 身份 | **需 `-AllowWrite`**，否则拒绝 |
| `push` | 快进推送（**绝不 --force**），带重试 | **需 `-AllowWrite`**，否则拒绝 |
| `verify-remote` | ls-remote 重试，比对 remote==local + ahead/behind | 只读 |
| `round` | baseline+build+test+boundary（只读机械核心） | 只读 |

**安全不变量**（与 SKILL.md §3 一致）：
- 默认 `baseline` 只读；commit/push 无 `-AllowWrite` 直接拒绝并打印意图。
- 无 reset/clean/stash/删分支/merge/rebase/force-push/文件删除。
- 脚本是**代理获授权后调用的工具**，不是自主提交器。提交分组、路径选择、授权由代理决定。

**已实测**（2026-11-28，HEAD `f2761ee`）：parse 0 错误；round 构建 0 警告、wp3 33/33、wp4 30/30、ht 65/65、ctest 22/22、生产零 diff、paht 不变；verify-remote MATCH；commit/push 无 `-AllowWrite` 均拒绝且 HEAD 未变。

### 用法

```powershell
$S = '.agents/skills/pae-project-development/scripts/pae_round.ps1'
# 只读机械核心
& $S -Stage round -OutDir C:\temp\pae-round
# 证据重压缩
& $S -Stage recompress -ImageSrc <out>\images -ImageDst <out>\images-compressed
# 获负责人批准后：核对暂存 → 提交 → 推送 → 核实
& $S -Stage verify-staged
& $S -Stage commit -AllowWrite -MessageFile C:\temp\msg.txt
& $S -Stage push -AllowWrite
& $S -Stage verify-remote
```

参数：`-Workspace`（仓库根）、`-Assets`（MakeHuman 固定 SHA 资产）、`-Branch`、`-OutDir`（日志）。

## 4. MCP 产出交付物（qwoffice）

当需要把结果产出为办公文档（而非仅 markdown/CSV）时：
1. `qwenwork_mcp_tool_list`（keyword=qwoffice）发现工具 → `qwenwork_mcp_tool_get` 取 schema → `qwenwork_mcp_tool_call` 执行。
2. 编辑 Word/Excel/PPT 前先 `docview_getlist` 核对 documentId；首次用某格式先 `load_guide`。
3. 典型映射：Gate 评估 → Word（`apply_word_ops`）；逐关节指标/响应曲线 → Excel（`apply_sheet_ops`）；阶段汇报 → PPT（`apply_pptx_ops`）。
4. 新建整份文件用宿主文件生成能力，不用 qwoffice 编辑工具创建；完成后 `save_document` + 呈现文件卡片。

**边界**：MCP 产出是交付物形式，不改变证据协议——数据仍须本轮真实、固定可比口径（见 test-and-evidence-protocol.md）。办公文档不能替代原始 CSV/日志/图证据。

## 5. 触发层（可选）

- **cron**（`qwenwork_cron_manage_job`）：定时跑只读回归（如每日 `-Stage round`）并把汇总发到 IM。
- **automation**（`qwenwork_automation_manage_rule`）：事件触发（如收到某消息启动审查）。
- 触发层只启动**只读/已授权**流程；commit/push 仍须负责人批准，不得借定时任务绕过授权。

## 6. 不做什么

- 不把授权/单变量/视觉 Gate 自动化掉。
- 不另建平行路线图或权威（沿用 MASTER-PLAN/STATUS）。
- 不为凑结构创建空脚本；新增脚本须解决实际问题、文档化依赖、显式错误处理。