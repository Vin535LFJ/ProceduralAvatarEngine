# PAE 仓库收敛执行清单（PAE-REPO-CONSOLIDATION-01）

> 记录 2026-11-26 本轮**实际执行过**的动作，以及**待负责人批准**的操作。
> 本轮没有删除任何目录或分支，没有 merge / cherry-pick / rebase / reset / clean，没有修改 origin，没有 commit / push。
> 评审结论见 [00-consolidation-review.md](00-consolidation-review.md)。

## A. 已执行（全部可回退）

| 时间 (+08:00) | 动作 | 结果 | 证据 |
|---|---|---|---|
| 19:53–19:54 | 只读盘点两个工作区与外部资产 | 主仓 HEAD `2ac101f`、poc-sm HEAD `a0783a2`，已跟踪改动均为空 | 归档 `inventory/git-state.txt` |
| 19:54 | 修正备份脚本：找不到的文件改为报错、检查 git / robocopy 退出码 | 修正后才执行备份 | 归档 `inventory/scripts/pae_backup.ps1` |
| 19:55–19:56 | 备份到 `E:\AI-Tools\_PAE-Repository-Archive\` | bundle `create` / `verify` 退出码 0；未跟踪 5/5 + 7/7；忽略 5/5 + 427/427；外部 2,357/2,357，哈希全部一致 | `inventory/BACKUP-SUMMARY.txt`、`inventory/*-copy-verify.log` |
| 19:57–20:00 | 远端与分支只读审计 | `ls-remote` 5 条分支与本地一致；`merge-base` = `a32251b`；feature 独有 11 笔、poc-sm 独有 16 笔；`merge-tree` 模拟冲突 7 个文件 | `inventory/remote-refs.txt`、`inventory/branch-audit/` |
| 19:57–20:00 | feature 11 笔逐文件内容比对 | 56 个路径：46 个相同、3 个已删草稿、7 个有差异；独有实质内容为 `bf7f854` 的 8 行 | `inventory/branch-audit/feature-commit-audit.txt` |
| 20:00 | `bf7f854` 迁移只读预检 | `git apply --check` 对工作区和已提交树均干净，**未应用** | `inventory/branch-audit/bf7f854-AUTH-08-execution-record.patch` |
| 20:00–20:02 | Host-only proof 重建验证 | 构建 0 警告；测试 65/0；流水线退出码 0、84 张图；链 B 退出码 0、方向 8/8、17 张图；101 张图与已提交证据逐字节相同 | `inventory/host-proof-verify/` |
| 20:02–20:05 | 归档收尾 | 修正汇总格式、写入干净的 bundle verify 日志、追加远端事实与审计证据、重算 `SHA256SUMS.txt`（自校验 0 不符） | `inventory/FINALIZATION-NOTES.txt` |
| 20:15 | 写入归档 `README.txt` 并最终重算清单 | 2,859 个文件 / 761.8 MB；`SHA256SUMS.txt` 2,858 条，自校验 0 不符 | `SHA256SUMS.txt` |
| 20:06–20:10 | 新建本目录三份文档 | 在 poc-sm 中为未跟踪状态 | 本文件 |

验证性构建只写入了 poc-sm 里被 Git 忽略的 `build-host/human-template-poc/`，其余输出都在仓库外的临时目录。两个工作区的已跟踪文件在整轮结束后仍是零改动。

## B. 待负责人批准（均未执行）

| # | 操作 | 建议 | 批准后的做法 | 影响面 |
|---|---|---|---|---|
| B1 | 迁入 `bf7f854` 的 8 行 AUTH-08 执行记录 | 建议执行 | 在 poc-sm 上 `git cherry-pick -x bf7f854`（或 `git apply` 同一补丁），单独一笔提交 | 1 个文档，+8 行；预检已通过 |
| B2 | `07-handover-note.md` 加入 Git | 建议执行 | `git add docs/research/human-template-reuse/07-handover-note.md`，单独一笔 `docs:` 提交 | 1 个文档 |
| B3 | 本目录三份治理文档入库 | 建议执行，可与 B2 合为一笔 | `git add docs/operations/repository-consolidation/` | 3 个文档 |
| B4 | commit / push 上述提交 | 待 B1–B3 | 推送到 `origin/poc-sm/shape-model-v2` | 快进 |
| B5 | 主仓目录处置方式 | 见下 | — | — |
| B6 | 删除远端 `m11-anatomy-visual-rebuild` | 条件满足后可删 | 先确认无 open PR、无其他 Agent 使用，再 `git push origin --delete m11-anatomy-visual-rebuild` | 独有提交 0，已在 bundle |
| B7 | 删除远端 `m12-unified-human-geometry` | 条件满足后可删 | 同上 | 与 `main` 同一提交 |
| B8 | 删除本地分支 `m13-procedural-retopology`、`poc-a-g2-fair-comparison` | 可选 | `git branch -d`（两者均无上游、独有提交 0） | 仅本地 |
| B9 | 桌面副本 `07-handover-note.md` 移入回收站 | B2 入库后再做 | 走系统回收站，不直接删除 | 1 个展示副本 |
| B10 | 签署 ADR-014 | 需负责人决定 | 修订 `project-charter.md`、ADR-004、product-scope | 治理文件 |
| B11 | Rig / Weights 两个文件的许可 | 需负责人或法务决定 | 确认许可，或决定由 PAE 自建 Rig V2 与权重 | 运行时路线 |
| B12 | 人眼视觉验收 | 需负责人亲自看 | `evidence/host-proof/images/` 的 `closeup-*.png`、`pose-finger-curl-*`，`images-proof-b/` 17 张 | 验收记录 |

### B5 的两种做法

**做法一（推荐，零风险）：原地保留主仓目录，只读。** 不执行任何目录操作。约束：不在该目录开发、不切换分支、不让 Agent 往里写。无需额外命令。

**做法二（需单独批准，步骤多）：把 poc-sm 变成独立 clone，之后主仓目录才能归档。**

1. `git clone --branch poc-sm/shape-model-v2 https://github.com/Vin535LFJ/ProceduralAvatarEngine.git <新目录>`
2. 从归档 `untracked-backups/poc-sm/` 恢复 7 个未跟踪文件，从 `ignored-data/poc-sm/` 恢复 427 个忽略文件。
3. 重建引擎 host 库 `build-host/libpae_core.a`，再按本轮相同步骤重跑链 A、链 B。
4. 验证通过后，才把主仓目录移入归档。

在做法二完成之前，主仓目录不能移动、重命名或删除。

## C. 本轮明确没有做的事

- 没有合并 `feature` 与 `poc-sm`，也没有让两个分支指向同一提交。
- 没有批量 cherry-pick，也没有执行任何一笔 cherry-pick。
- 没有删除任何远端或本地分支，没有删除 `main`。
- 没有切换任何工作区的分支。
- 没有改写 ADR-014 的正式状态，没有修改 Android 生产路径、骨骼或渲染 ABI。
- 没有把 MakeHuman 原始资产、`.paht`、`.workbuddy-ai/` 复制进任何 Git 仓库。

## D. 需要如实说明的限制

- 本机没有安装 `gh`，无法查询 open PR；其他 Agent 是否占用某条分支也无从核实。
- 本轮 `ls-remote` 因网络抖动失败过一次（`exit=128`），重试后成功；失败与成功的记录都保留在 `inventory/remote-refs.txt`。
- Host proof 的“图是否像完整的人”没有经过人眼判断，本轮验证的是**可复现性**，不是视觉效果。
- `E:\AI-Tools\PAE-human-template-poc\` 此前有另一个代理并行写入过文件；本轮只读取和备份，没有改动其中任何文件。
