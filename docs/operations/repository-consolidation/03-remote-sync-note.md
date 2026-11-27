# PAE 远端同步情况说明（2026-11-27）

> 性质：**治理 / 过程记录**，不计入工程里程碑（`AGENT.md` §3）。本次同步只有文档，没有代码变更。
> 起因：负责人指示“最新代码同步到远端，并生成对应的情况说明”。
> 关联：[00-consolidation-review.md](00-consolidation-review.md)、[01-execution-checklist.md](01-execution-checklist.md)
> 分支：`poc-sm/shape-model-v2`　仓库：`https://github.com/Vin535LFJ/ProceduralAvatarEngine`

## 1. 结论

- **代码本来就已经在远端。** 同步前核对过：5 条有上游的分支与远端逐 SHA 一致，两个工作区的已跟踪改动为空，本地没有未提交的代码。
- 本次实际同步的是**文档**：交接说明 1 份、仓库收敛治理文档 3 份（第一笔提交 `6df3872`），以及本说明和两处状态更新（紧随其后的第二笔提交）。
- 没有执行：`bf7f854` 的迁移、任何分支删除、任何目录归档或移动、merge / cherry-pick / rebase / reset。

## 2. 同步前的状态（2026-11-27 09:50 核对）

| 分支 | 本地 | 远端（`ls-remote`） | 差异 |
|---|---|---|---|
| `poc-sm/shape-model-v2` | `a0783a2` | `a0783a20f8d9dc6104f799d773ec8f081bc2abc3` | 无 |
| `feature/procedural-character-foundation` | `2ac101f` | `2ac101fd40e533d2f08776cb6f4e9356cd1b25d1` | 无 |
| `main` | `c539afb` | `c539afbec299ed4a066429ea09f9d3d36e67f74d` | 无 |
| `m11-anatomy-visual-rebuild` | `67ccb39` | `67ccb3940950623b1a24d6b3e843750c5c4a99b3` | 无 |
| `m12-unified-human-geometry` | `c539afb` | `c539afbec299ed4a066429ea09f9d3d36e67f74d` | 无 |

最近的代码提交是 `0baa698`（Host-only 人体模板工具）和 `a0783a2`（研究文档与证据），两者此前已在远端。

## 3. 本次同步的内容

### 第一笔：`6df3872850909891e4f10c4d060ff9b50f8e9ffc`

`docs(operations): add the repository consolidation review, checklist, inventory and handover note`

父提交 `a0783a2`。4 个文件，+553 行，全部是新增文档：

| 文件 | 行数 | 内容 |
|---|---|---|
| `docs/research/human-template-reuse/07-handover-note.md` | 97 | 两分支现状、提交差异、专家评审清单、待拍板事项 |
| `docs/operations/repository-consolidation/00-consolidation-review.md` | 257 | 收敛评审：目录状态、备份、重建验证、分支审计、结论 |
| `docs/operations/repository-consolidation/01-execution-checklist.md` | 68 | 已执行动作与待批准操作 |
| `docs/operations/repository-consolidation/02-local-workspace-inventory.md` | 131 | 逐目录盘点表与分支清单 |

提交前核对：暂存区恰好这 4 个文件；暂存内容与工作区逐字节一致（文件为 LF 换行，仓库设置 `core.autocrlf=true`，重新检出时会变成 CRLF，这些文档没有登记哈希，不受影响）。

### 第二笔：紧随 `6df3872` 之后

`docs(operations): add the remote sync status note`

3 个文件：本说明（新增），以及 `00` 和 `01` 中把“未入库 / 未 push”改为当前事实的状态更新。提交内容里不含任何代码。

本说明无法写进它自己所在提交的 SHA。核对远端头请用：

```powershell
git -C E:\AI-Tools\ProceduralAvatarEngine-poc-sm rev-parse HEAD
git -C E:\AI-Tools\ProceduralAvatarEngine-poc-sm ls-remote origin refs/heads/poc-sm/shape-model-v2
```

两行输出相同，即为已同步。

## 4. 没有同步的内容及原因

| 内容 | 位置 | 原因 |
|---|---|---|
| `.workbuddy-ai/memory/2026-11-24.md` | poc-sm，未跟踪 | 本地 Agent 记忆，不是本轮成果，是否入库由负责人定 |
| `.qoder-input-files/4e253c55-….md` | poc-sm，未跟踪 | 会话附件暂存 |
| `evidence/character-v6/*.png`、`evidence/character-v7/*.png`（4 张） | poc-sm，未跟踪 | 11-24 的旧截图，不是本轮生成；已进归档 |
| `m13-procedural-retopology`、`poc-a-g2-fair-comparison` | 本地分支，无上游 | 独有提交为 0，内容已可从其他分支到达；已在归档 bundle 中 |
| 主仓根目录的 `00-`、`01-`、`02-`、`07-` 四个 md | 主仓工作树，未跟踪 | 展示用副本，原件已入库 |

以上文件都已在 `E:\AI-Tools\_PAE-Repository-Archive\` 中有备份。

## 5. 仍未执行、等负责人决定

1. 迁入 `bf7f854` 的 8 行 AUTH-08 执行记录（预检已通过，未应用）。
2. 主仓目录的处置方式（它持有 poc-sm 依赖的 `.git`，不能直接移动或删除）。
3. 删除远端 `m11` / `m12`（还需确认没有 open PR、没有其他 Agent 在用）。
4. 主仓根目录展示副本是否移入回收站。
5. ADR-014 批准、Rig / Weights 两个文件的许可、人眼视觉验收。

## 6. 回退

两笔提交都只增加或修改文档。需要撤销时，在 poc-sm 上对相应提交执行 `git revert`，再推送；不要用 `reset` + 强推。

## 7. 如实说明

- 本次同步由代理在负责人指示下执行，提交作者沿用仓库既有身份配置，未做更改。
- “代码已同步”的依据是第 2 节的逐 SHA 比对，不是推断。
- 推送结果以第 3 节末尾的核对命令为准。
