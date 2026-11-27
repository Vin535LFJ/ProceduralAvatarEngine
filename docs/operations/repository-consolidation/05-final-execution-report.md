# PAE 仓库收敛最终执行报告（PAE-CONSOLIDATION-03）

> 日期：2026-11-27
> 性质：**治理 / 过程记录**，不计入人体工程实现里程碑（`AGENT.md` §3）。本轮没有修改任何 Android 生产代码。
> 批准依据：负责人 2026-11-27 会话提交 PAE-CONSOLIDATION-03 计划并指示"按照以上计划执行"。
> 前序：[04-consolidation-closeout-report.md](04-consolidation-closeout-report.md)、[15-vr1b-wp0-decision-package.md](../../phases/P1-visual-recovery/15-vr1b-wp0-decision-package.md)
> 证据：`E:\AI-Tools\_PAE-Repository-Archive\inventory\consolidation-03-20261127\`

## 0. 最终状态

| 状态项 | 结论 |
|---|---|
| `REPOSITORY_CONSOLIDATION` | **COMPLETE**（唯一残留：C 盘源目录的回收站操作被宿主中止，已验证的冗余副本留待手动清理，见 §6） |
| `ACTIVE_WORKSPACE` | **VERIFIED** — `E:\AI-Tools\ProceduralAvatarEngine-poc-sm`，独立 `.git`，提交后全链路重验证通过 |
| `BRANCH_CLEANUP` | **COMPLETE** — 4 条候选已删（远端普通删除、本地 `-d`）；main / poc-sm / feature 保留 |
| `ADR-014` | **APPROVED（ACCEPTED，2026-11-27）** — 正式文本 + 五处同步修订已入库 |
| `HUMAN_TEMPLATE_BASELINE` | **ACCEPTED（ACCEPTED_FOR_VR1B）** — 不是最终视觉 Gate PASS |
| `VR-1B_WP1_WP2` | **AUTHORIZED** — 白名单生效，生产实现尚未开始（下一轮） |

## 1. 提交与远端

| 提交 | 内容 | 核对 |
|---|---|---|
| `49d85a89211a77cdaf562dcf585e011466c0e2a4` | `cherry-pick -x bf7f854`：AUTH-08 执行记录 8 行 | 恰好 1 文件 +8/−0；`show --check` 干净；带 `(cherry picked from …)` 来源行；原作者保留（历史），committer = noreply 身份 |
| `bc0ab6dba41c3d5d1e56710176328abeaca9b982` | ADR-014 正式文本 + charter / ADR-004 / product-scope / 研究注记 / ADR_INDEX 五处同步修订 | 6 文件 +109/−4；暂存字节核对 0 不符 |
| `a8a5a60ef0cd7106204d2647cf5eb2bf1bfce90a` | 04 收尾报告、15 决策包（已决策状态）、18 张视觉验收包、R1 哈希更正 | 23 文件 +464/−1；18 图哈希全对；无 `.workbuddy-ai` / `.qoder-input-files` / 构建输出 / 原始资产混入 |

推送：普通快进 `6a1c29d..a8a5a60`，无 force。推送后 `ls-remote` = 本地 HEAD = `a8a5a60`（0/0）。
本报告所在提交为紧随其后的第四笔；远端头以核对命令为准：

```powershell
git -C E:\AI-Tools\ProceduralAvatarEngine-poc-sm rev-parse HEAD
git -C E:\AI-Tools\ProceduralAvatarEngine-poc-sm ls-remote origin refs/heads/poc-sm/shape-model-v2
```

提交身份：staging 克隆继承全局公司邮箱，已按负责人既定规则设置 **local** config 为
`Vin535LFJ <169535939+Vin535LFJ@users.noreply.github.com>`（未动全局）；三笔新提交 committer 均已核实。
两份完整 staged diff 与 R1 更正补丁存归档 `consolidation-03-20261127/`。

## 2. R1 SHA-256 更正迁移（切换前关键检查）

- 旧 poc-sm 导出 git diff（1,312 B，仅 1 行）；staging 当时仍为错误的 65 位值——**确认未自动转移**。
- 实算 zip SHA-256 = `1604d6b35925c8b1c0a7a5e8be8d1f01a53ca8dd80e07dea95c1ba84b192e9b5`（64 位），与归档备份、归档清单三方一致。
- `apply --check` 通过后仅应用这一项；随 `a8a5a60` 入库。R1 其余内容无丢失（diff 仅 1 行）。

## 3. 正式切换（独立 Clone → 唯一活动工作区）

切换前检查全部通过：`.git` 为真实目录、origin 正确、分支 `poc-sm/shape-model-v2`、HEAD = 远端、433 个恢复文件哈希一致、R1 更正与 AUTH-08 已提交、工作区干净。

执行顺序（全部保留回退）：

1. 旧 linked worktree `E:\AI-Tools\ProceduralAvatarEngine-poc-sm` **整体移动**到归档 `retired-worktrees\ProceduralAvatarEngine-poc-sm-linked`（同盘改名，未删除任何文件）。
2. 原 Git 宿主执行 `git worktree prune`（git 原生命令清理失效元数据；未手删 `.git/worktrees/`）。
3. staging `E:\AI-Tools\PAE-independent-clone-staging` **改名**为 `E:\AI-Tools\ProceduralAvatarEngine-poc-sm`。

新路径实测：

```text
toplevel         = E:/AI-Tools/ProceduralAvatarEngine-poc-sm
absolute-git-dir = E:/AI-Tools/ProceduralAvatarEngine-poc-sm/.git   （不依赖 C 盘）
branch           = poc-sm/shape-model-v2
HEAD             = a8a5a60ef0cd7106204d2647cf5eb2bf1bfce90a = 远端
```

**切换后重验证**（新路径，`build.ps1` 原地重建零警告）：测试 `65 passed, 0 failed`；流水线退出码 0、84 张图、模板往返逐字节一致；链 B 编译零警告、`DIRECTION_CHECK actions=8 failed=0`、17 张渲染。已跟踪文件零改动。
（提交后切换前还做过一轮全量重建：引擎库 + 两条链 + 101/101 图与已提交证据逐字节一致，见归档 `post-commit-reverify-summary.txt`。）

一处如实说明：引擎 host 库的 CMakeCache 记录的是改名前的 staging 路径，下次**完整**重建引擎库时需重新 configure（链 A 工具构建目录的缓存路径恰与新路径一致，已原地重建验证；链 B 链接的 `libpae_core.a` 是切换前从相同源码重建的产物）。

## 4. 分支清理（复核后执行）

即时复核：4 条候选独有提交均 0；bundle verify 退出码 0 且含全部候选引用；open PR = 0（GitHub API）；无 worktree 占用。

| 操作 | 结果 |
|---|---|
| `git push origin --delete m11-anatomy-visual-rebuild` | `[deleted]`，退出码 0 |
| `git push origin --delete m12-unified-human-geometry` | `[deleted]`，退出码 0 |
| `git branch -d m13-procedural-retopology` | Deleted（was `bf5415f`），未用 `-D` |
| `git branch -d poc-a-g2-fair-comparison` | Deleted（was `2ac101f`），未用 `-D` |

远端现存 3 条：`main`（默认稳定基线）、`poc-sm/shape-model-v2`（唯一主开发）、`feature/procedural-character-foundation`（只读历史，按指示保留）。
宿主本地 m11/m12 分支不在计划的本地删除清单内，随 C 盘 `.git` 归档保留。全部被删分支在 bundle 中可恢复。

## 5. 展示副本与临时输出清理

**展示副本（6 个，全部进系统回收站，可恢复）**：00/01/02 差异仅为已被取代的过期状态行；04 与原件 SHA 完全一致；15 与 00-README 的副本独有行全部是本轮决策更新取代的 DRAFT/PENDING/占位文本（逐行核对，无独有内容）。逐文件 SHA 与差异定性存归档 `display-copy-handling.txt`。加上前轮的 03/07，根目录展示副本共清理 8 个。

**临时输出（218 MB，599 个文件，进系统回收站）**：`pae-staging-verify`（84.5 MB）与 `pae-proof-verify`（133.7 MB）——日志均已归档，其余为可再生输出且已验证与已提交证据逐字节一致。逐文件清单存归档 `temp-cleanup-list-20261127.txt`。
保留：staging/新工作区构建产物、外部 MakeHuman 资产、主归档、正式证据、全部脚本。

## 6. C 盘主仓归档（含一次故障与恢复，如实记录）

条件核对（全部成立）：staging 已切换；旧 worktree 已移除；feature 已跟踪零改动；未跟踪 4 个 + 重要忽略 5 个文件与归档哈希全对；bundle verify 通过；独有提交均在远端/bundle；外部资产未触碰。

**故障**：跨盘 `Move-Item` 在删除源 `.git` 时被只读属性中断，文件被拆分（源 2,112 + 目标 1,509 = 原 3,621，**无丢失**）。
**恢复**：`robocopy /E /COPY:DAT` 合并剩余 2,112 个文件；目标达到 **3,621 文件 / 414,257,260 字节 = 原盘点精确一致**；目标 git HEAD `2ac101f`、feature 分支、已跟踪干净、`fsck --no-dangling` 退出码 0；源残留 2,112 个文件逐文件哈希与目标一致（0 不符）。

归档路径：`E:\AI-Tools\_PAE-Repository-Archive\retired-worktrees\ProceduralAvatarEngine-main-C-drive`。

**唯一残留**：C 盘源目录（已验证的冗余副本）两次 `host_safe_delete` 均返回 `Operation was aborted`（已先清除只读属性，仍失败；推测宿主保护当前会话锚定的工作区目录）。按纪律停止重试。**该目录不含任何独有内容**，负责人可在关闭/重锚工作区后手动移入回收站，或在新会话中批准重试。在此之前不在该目录做任何开发。

## 7. 治理决定落地

- **ADR-014**：`docs/architecture-decisions/ADR-014-offline-compiled-human-base-topology.md`（ACCEPTED，2026-11-27）。决策记录按 ADR-013 格式转录负责人指示原话与计划章节，**未代签**。五处同步修订（charter ×2、ADR-004、product-scope、研究注记）+ ADR_INDEX 登记，原文保留可追溯。
- **Rig/Weights**：PAE 自建 HumanRigV2（20-bone core）+ 自生成权重；`default.mhskel` / `default_weights.mhw` 保持 `PENDING_CONFIRMATION` 并排除出分发资产；肩外展拉伸缺陷如实记录，未标记生产通过。
- **视觉基线**：`HUMAN_TEMPLATE_BASELINE = ACCEPTED_FOR_VR1B`（15 号决策包 §3 与验收包 README 已记录依据与边界）；`FINAL_HUMAN_VISUAL_GATE` 未通过；Android 渲染 NOT RUN。
- **WP1/WP2 授权**：15 号决策包 §4 白名单生效（AUTHORIZED）；§4.3 禁止项全部有效；D1 时间戳问题须在模板进仓/进 APK 前解决。

## 8. NOT RUN / NOT VERIFIED

| 项 | 状态 |
|---|---|
| WP1/WP2 生产实现（模板编译器产品化、Android 静态加载） | NOT RUN — 已授权，下一轮启动 |
| Android 全链路（GLES3 / 真机 / 生命周期 / 内存） | NOT RUN |
| C 盘源目录回收站清理 | BLOCKED — 宿主中止，留待手动/新会话（§6） |
| 引擎 host 库改名后完整 reconfigure | NOT RUN — 下次完整重建时执行（§3） |
| 负责人对 18 张图的逐图亲笔签署 | PENDING — 基线决定已记录，细节签署栏保留 |
| 其他代理未来是否使用已删分支 | 不适用 — 删除前观察窗口无活动，bundle 可恢复 |

## 9. 归档增量

本轮新增证据：`inventory/consolidation-03-20261127/`（staged diff ×2、R1 补丁、清理清单、重验证汇总 ×2、新工作区日志 ×3、分支删除/切换/副本记录 ×3）+ `retired-worktrees/`（旧 linked worktree + C 盘主仓完整副本）。
归档总量 **10,032 个文件**；`SHA256SUMS.txt` **10,031 条，自校验 0 不符**，清单自身 SHA-256 `f341b52f2839112d6d1cc757c0b089c7a1a8768ad650a77f1153f41efdd78ba2`。

## 10. 下一步

仓库治理到此结束。开发重心转向：**在 Android 真机上第一次正确加载和渲染 PAE 完整人体模板**（WP1 → WP2，按 15 号决策包 §4 白名单与 R8 顺序）。