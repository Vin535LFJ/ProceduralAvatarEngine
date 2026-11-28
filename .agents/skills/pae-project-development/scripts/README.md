# Scripts

PAE 工作流辅助脚本。

## 本技能维护的编排脚本

- `scripts/pae_round.ps1`：**PAE 工程轮编排器**（PowerShell 5.1）。固化机械、确定性、可重复环节：
  `baseline / build / test / boundary / recompress / verify-staged / commit / push / verify-remote / round`。
  - 只读机械核心：`-Stage round`（baseline+build+test+boundary）。
  - **写安全**：`commit`/`push` 需显式 `-AllowWrite`，否则拒绝；push 仅快进（绝不 --force）；无 reset/clean/stash/删分支/merge/rebase/文件删除。
  - 脚本是代理获授权后调用的工具，**不替代**授权判断、单变量纪律、不降标准、人工视觉 Gate。
  - 用法与三层（Skill+Tools+MCP）组合见 [../references/tool-and-mcp-integration.md](../references/tool-and-mcp-integration.md) 与 [../workflows/engineering-round-workflow.md](../workflows/engineering-round-workflow.md)。
  - 已实测（2026-11-28）：parse 0 错误；round 构建 0 警告、wp3 33/33、wp4 30/30、ht 65/65、ctest 22/22、生产零 diff、paht 不变；verify-remote MATCH；无 -AllowWrite 时 commit/push 拒绝。

## 仓库已有可复用工具（不在本技能内重复维护，由编排脚本调用）

- `tools/human-template-poc/host/tools/recompress_png.py`：证据 PNG 无损重压缩（像素往返校验）。
- `tools/human-template-poc/host/build.ps1`：Host 工具构建（含 CMake configure）。
- `tools/png_probe.py`：截图覆盖率探针（真机证据）。

## 新增脚本约定

须：解决实际问题而非敷衍；文档化依赖；显式错误处理；写操作显式授权门；不用 Windows 风格硬编码路径于文档示例（用 `scripts/helper.py`）。不为凑目录结构创建空脚本。