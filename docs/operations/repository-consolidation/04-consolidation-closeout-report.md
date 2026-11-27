# PAE 仓库收敛收尾报告（PAE-CONSOLIDATION-02）

> 日期：2026-11-27
> 性质：**治理 / 过程记录**，不计入工程里程碑（`AGENT.md` §3）。
> 本轮**没有** commit / push、没有删除任何分支、没有移动或删除任何工作树、没有修改 Android 生产代码、没有签署任何 ADR。
> 唯一的删除动作：2 个 SHA 完全一致的展示副本移入系统回收站（可恢复，见 §5）。
> 前序：[00-consolidation-review.md](00-consolidation-review.md)、[01-execution-checklist.md](01-execution-checklist.md)、[02-local-workspace-inventory.md](02-local-workspace-inventory.md)、[03-remote-sync-note.md](03-remote-sync-note.md)
> 配套决策包：[15-vr1b-wp0-decision-package.md](../../phases/P1-visual-recovery/15-vr1b-wp0-decision-package.md)

## 0. 最终状态

| 状态项 | 结论 |
|---|---|
| `REPOSITORY_CONSOLIDATION` | **PARTIAL** — 独立 Clone staging 已建立并通过全部重建验证；正式切换与旧工作树归档等负责人批准；`bf7f854` 迁移演练通过但未提交 |
| `BRANCH_CLEANUP` | **READY** — 4 条候选全部复核完毕（独有提交 0、无 tag、open PR = 0、无 worktree 占用），删除命令已备好，未执行 |
| `ADR-014` | **DRAFT** — 正式签署包已备齐（见决策包 §1），未签署 |
| `HUMAN_TEMPLATE_VISUAL_ACCEPTANCE` | **PENDING** — 18 张精选验收包已备好（见 §8），负责人未看 |
| `VR-1B_WP1_WP2` | **READY_FOR_OWNER_AUTHORIZATION** — WP0 材料齐备；授权前提是 ADR-014 签署 + Rig/Weights 裁定 + 视觉基线确认 |

## 1. 当前 Git 事实（本轮实测）

| 位置 | 分支 | HEAD | 与远端 |
|---|---|---|---|
| 主仓 `C:\Users\Giant\Desktop\GITCODE\Procedural Avatar Engine` | `feature/procedural-character-foundation` | `2ac101fd40e533d2f08776cb6f4e9356cd1b25d1` | 一致 |
| poc-sm 工作树 `E:\AI-Tools\ProceduralAvatarEngine-poc-sm` | `poc-sm/shape-model-v2` | `6a1c29daa009061d4adbaac1f63ef9516c8451b1` | 一致 |
| **staging 独立 Clone** `E:\AI-Tools\PAE-independent-clone-staging` | `poc-sm/shape-model-v2` | `6a1c29daa009061d4adbaac1f63ef9516c8451b1` | 一致（直接从远端克隆） |

远端 5 条分支 SHA 于本轮 `ls-remote` 核实（网络恢复后成功；输出存归档 `inventory/branch-audit/migration-rehearsal-20261127/remote-refs-20261127.txt`）：`main` = `c539afbec299ed4a066429ea09f9d3d36e67f74d`（默认分支），其余与本地跟踪引用逐 SHA 一致。远端 tag：0。open PR：**0**（GitHub API 实测）。

两个原工作区已跟踪文件除一处哈希更正外零改动（R1 第 23 行 zip SHA-256 抄写错误，见 §10.8，未 commit）。poc-sm 的 `.git` 仍指向主仓目录——**主仓目录在正式切换完成前不能移动、重命名或删除**。

## 2. bf7f854 迁移演练（未提交）

按任务书 §4 在隔离 worktree 中演练，全程未 commit：

| 检查项 | 结果 |
|---|---|
| 补丁来源 | `git format-patch -1 bf7f8547d50d904595f2486cba3025e2a109858f`（git 直接写字节，2,379 B；不用 PowerShell 重编码） |
| 演练基线 | 临时 detached worktree @ `6a1c29d`（当前 poc-sm HEAD，不是旧报告的 `a0783a2`） |
| `git apply --check` | 退出码 0 |
| 影响面 | **恰好 1 个文件**：`docs/phases/anatomy-driven-geometry-poc/10-decision-records/04-AUTH-08-granted.md`，numstat `8 0`（+8/−0） |
| 内容核对 | 应用后文件内容与 feature 版逐字一致（忽略 EOL）；blob `2c19247…` → `18bb44d…`，与补丁 index 行一致 |
| 不覆盖后续记录 | 8 行插入在 AUTH-08 执行结果表内，poc-sm 上该文件的其余内容（含后续 round 的记录）不变 |
| 生产代码 | 零触碰 |
| 演练后 | 临时 worktree 已用 `git worktree remove` 移除；poc-sm 工作区未被应用补丁，HEAD 不变 |

证据：归档 `inventory/branch-audit/migration-rehearsal-20261127/`（补丁、应用后 diff、演练日志）。

**待负责人审查 diff 后**，正式迁入方式：在 poc-sm 上 `git cherry-pick -x bf7f854`（或应用同一补丁后单独一笔 `docs:` 提交），再推送。本轮未做。

一个过程性事实（如实记录）：首次演练 `apply --check` 失败，原因是我用 `Out-File` 写补丁时 PowerShell 把 LF 改成 CRLF 且 ascii 编码破坏了中文/emoji；改用 git 自身输出后通过。补丁内容本身自始至终没有问题。

## 3. 独立 Clone staging（已建立、已验证，未切换）

### 3.1 建立

- 来源：**远端** `https://github.com/Vin535LFJ/ProceduralAvatarEngine.git`，`git clone --branch poc-sm/shape-model-v2`（本轮网络恢复后直接成功，未用本地回退）。
- 位置：`E:\AI-Tools\PAE-independent-clone-staging`。
- 独立性：`.git` 是**目录**（`git rev-parse --git-common-dir` = `.git`），不依赖 C 盘主仓；origin 指向 GitHub。
- HEAD = `6a1c29d` = 远端；`CanonicalSkinGenerator.h` 存在；`tools/human-template-poc/` 已跟踪 29 个文件齐全。

### 3.2 从归档恢复本地文件

| 类别 | 数量 | 结果 |
|---|---|---|
| 未跟踪文件 | 6/6（另 1 个跳过） | 逐文件 SHA-256 一致 |
| 重要忽略文件 | 427/427 | 逐文件 SHA-256 一致 |
| 跳过项 | `07-handover-note.md` | 已入库（`6df3872`），克隆检出内容与备份内容一致；字节差仅是 `core.autocrlf` 检出转换（备份 LF 5,433 B / 检出 CRLF 5,530 B），规范化后 Git blob 同为 `552f2c8c21a6de3b2a41f13120a0303ed3339e51` |

恢复后 staging 已跟踪文件零改动。保护清单全部在位：`.workbuddy-ai/`、`.qoder-input-files/`、v6/v7 截图、2 个 VR-1A APK、3 个模板（`.paht`/`.paehuman`）、`libpae_core.a`、Host proof 日志。MakeHuman 原始资产仍在仓库外 `E:\AI-Tools\PAE-human-template-poc\assets\`，未复制进任何 Git 仓库。

### 3.3 重建验证（全部从 staging 源码重新构建）

| 步骤 | 结果 |
|---|---|
| 引擎 host 库 CMake configure + build（`native/` → `build-host/`，`-DPAE_HOST=ON`，Release） | 退出码 0，79 s，**0 警告** |
| 链 A configure + build（新目录 `build-host/human-template-poc-verify/`） | 退出码 0，20.8 s，0 警告 |
| 65 项原型测试 | `65 passed, 0 failed, 65 total`，65 个唯一编号，退出码 0 |
| 链 A 流水线 | 退出码 0，18.1 s，84 张图；模板 4,113,384 B，写出→读回**逐字节一致**；回退 `body 175 kept-ancestor + 0 nearest-bone, aux 648 + 0, 0 ids unweighted` |
| 链 B proof（链接 staging 重建的 `libpae_core.a`） | 编译 0 警告；退出码 0；`DIRECTION_CHECK actions=8 failed=0`；墙钟 2.86 s，峰值内存 38,055,936 B |
| 链 B 渲染 | 17 张 |
| 与已提交证据比对 | 经同一压缩步骤后 **84/84 + 17/17 张逐字节相同** |
| staging 已跟踪文件 | 整轮零改动 |

证据：归档 `inventory/staging-clone-verify-20261127/`（汇总 + 全部日志原文）。

两点如实说明：

1. **链 A 用了新构建目录**：恢复的 `build-host/human-template-poc/` 带着指向 poc-sm 的旧 `CMakeCache.txt`，直接 configure 会报缓存目录不符。未删除旧目录（保护策略），改为 configure 到 `human-template-poc-verify/`。正式切换时该旧目录可整体废弃。
2. **`libpae_core.a` 体积口径不同**：staging 重建（Release）897,910 B；归档恢复的旧构建 11,293,494 B（含调试信息）。链 B 用重建库全部通过、101 张图逐字节一致，功能等价已验证；体积差异来自构建类型，不是源码差异。

### 3.4 旧工作树处置建议（待批准，未执行）

staging 验证已满足任务书 §5.2 的全部条件。正式切换建议步骤（每一步都需负责人批准后执行）：

1. 批准 `bf7f854` 迁入（§2）并在 poc-sm 提交推送；staging `git pull` 同步。
2. 把 staging 定为正式独立 Clone（可保留现名，或经批准改名/移动到目标路径）。
3. 确认 staging 上工作流正常后，将旧 poc-sm linked worktree 与主仓目录**移入归档**（走回收站或归档移动，不永久删除）；此前先在主仓执行 `git worktree remove` 解除链接关系。
4. 归档后复核：staging 独立运行全部验证仍通过。

在步骤 3 之前，主仓目录与旧 poc-sm 工作树**保持原样**。`E:\AI-Tools\PAE-human-template-poc` 是外部资产实验室，不是重复 Clone，任何情况下都不按重复项目处理。

## 4. 分支删除候选（已复核，未删除）

| 分支 | SHA | 独有提交 | 复核结果 | 处置 |
|---|---|---|---|---|
| `m11-anatomy-visual-rebuild`（远端+本地） | `67ccb3940950623b1a24d6b3e843750c5c4a99b3` | 相对保留分支 0 | 无 tag、无 open PR、无 worktree 检出、`main` 可达 | 候选，待批准 |
| `m12-unified-human-geometry`（远端+本地） | `c539afbec299ed4a066429ea09f9d3d36e67f74d` | 与 `main` 同一提交 | 同上 | 候选，待批准 |
| `m13-procedural-retopology`（仅本地） | `bf5415fc9f51af02e6f80f5adef255df87bc97e3` | 0（`main` 可达） | 无上游、无 worktree 检出、已在 bundle | 候选，待批准 |
| `poc-a-g2-fair-comparison`（仅本地） | `2ac101fd40e533d2f08776cb6f4e9356cd1b25d1` | 与 feature 同一提交 | 同上 | 候选，待批准 |
| `feature/procedural-character-foundation` | `2ac101f…` | 11 笔，实质独有仅 `bf7f854` 8 行 | 迁移完成后列为候选 | **在迁移 + 独立 Clone 切换 + 旧工作树归档之前不删除** |
| `main` | `c539afb…` | — | 默认稳定分支 | **永久保留** |
| `poc-sm/shape-model-v2` | `6a1c29d…` | — | 唯一主开发分支 | **永久保留** |

其他代理占用：本轮观察窗口内（主仓/poc-sm 近 8 小时、外部实验室近 24 小时）除本代理的同步轮编辑外无写入。**无法保证未来没有其他代理使用**，删除前请负责人确认。

批准后的命令（本轮未执行）：

```powershell
git -C <repo> push origin --delete m11-anatomy-visual-rebuild
git -C <repo> push origin --delete m12-unified-human-geometry
git -C <repo> branch -d m11-anatomy-visual-rebuild m12-unified-human-geometry m13-procedural-retopology poc-a-g2-fair-comparison
```

全部候选已在归档 bundle（`git-bundles/pae-all-refs.bundle`，SHA-256 `346220339cc84f222e55d8845d5b90a71bf6565d9de82386c898a0bd071da24b`）中，删除可恢复。

## 5. 五个展示副本处理

| 副本（主仓根目录） | 与原件 SHA-256 | 处理 |
|---|---|---|
| `03-remote-sync-note.md` | 一致（`aa2480b9…`） | **已移入系统回收站**（`status=deleted, recoverable=true`） |
| `07-handover-note.md` | 一致（`8bfa6e4d…`） | **已移入系统回收站**（同上） |
| `00-consolidation-review.md` | 不一致 | **保留**。副本是同步前版本；差异仅为 11-27 状态更新（"未入库/待批准"→"已执行"），副本无任何独有实质内容 |
| `01-execution-checklist.md` | 不一致 | 同上（B2–B4 状态行） |
| `02-local-workspace-inventory.md` | 不一致 | 同上（1 行未跟踪文件说明） |

按任务书 §7"SHA 不一致先报告、不清理"，后三个留待负责人表态：它们的内容已被入库原件完全取代，批准后可同样走回收站。逐文件哈希与差异定性存归档 `inventory/display-copy-handling-20261127.txt`。

## 6. ADR-014 与 Rig/Weights

- ADR-014 正式签署包：见 [15-vr1b-wp0-decision-package.md](../../phases/P1-visual-recovery/15-vr1b-wp0-decision-package.md) §1。状态 **DRAFT**，本轮未改写 `project-charter.md`、ADR-004、product-scope 的任何原文。
- Rig/Weights：建议正式采用 **PAE 自建 HumanRigV2 Core（20 骨）+ PAE 自生成权重**；`default.mhskel`、`default_weights.mhw` 保持 `PENDING_CONFIRMATION`，不作为 Android 生产依赖。已知缺陷（肩外展拉伸、握拳翻面）如实记录在决策包 §2。

## 7. 备份与归档增量

归档 `E:\AI-Tools\_PAE-Repository-Archive\` 本轮新增 18 个证据文件（迁移演练 5、staging 验证日志 11、分支复核 1、副本处理 1），总量 **2,877 个文件**；`SHA256SUMS.txt` 重算为 **2,876 条，自校验 0 不符**，清单自身 SHA-256 `d7430be6d3c704c9ca544d1ec8442f6e6a6a42c4e839fb17174deb8b24decd08`。原有备份内容未改动。

## 8. 视觉验收

18 张精选验收包：`docs/research/human-template-reuse/visual-review/`（未跟踪，含逐文件 SHA-256 与来源映射）。验收问题、已知缺陷、逐图清单与签署栏见其 `00-README.md`。状态 **PENDING**。

## 9. NOT RUN / NOT VERIFIED 清单

| 项 | 状态 |
|---|---|
| `bf7f854` 正式迁入（commit/push） | NOT RUN — 等负责人审查 diff |
| 任何分支删除 | NOT RUN — 候选已备齐 |
| 主仓目录 / 旧 worktree 归档移动 | NOT RUN — 等正式切换批准 |
| staging 改名/移动为正式工作区 | NOT RUN |
| ADR-014 签署与治理文件修订 | NOT RUN — DRAFT |
| 负责人人眼视觉验收 | NOT RUN — 验收包已备 |
| Android 全链路（GLES3/UBO/JNI/RenderThread/真机） | NOT RUN — VR-1B 范围 |
| 00/01/02 展示副本清理 | NOT RUN — SHA 不一致，按任务书先报告 |
| 其他代理未来是否使用候选分支 | NOT VERIFIED — 仅本轮观察窗口 |
| staging 上 Android/Gradle 构建 | NOT RUN — 本轮只验证 Host 链（任务书 §5.2 范围） |

## 10. 本轮动作总账（全部可回退）

1. 只读核查与文档阅读。
2. 临时 detached worktree 迁移演练 → 已移除；poc-sm 未动。
3. 从远端克隆 staging；从归档恢复 433 个文件（哈希核对）。
4. staging 内重建验证（写入 staging 的 git-ignored 构建目录与仓库外临时目录）。
5. 2 个 SHA 一致的展示副本 → 系统回收站。
6. 归档新增 18 个证据文件并重算清单。
7. 新建本报告、决策包、视觉验收包（poc-sm 内未跟踪，**未 commit**）。
8. **更正 R1 的一处哈希抄写错误**：`01-asset-license-and-provenance.md` 第 23 行的归档 zip SHA-256 原为 65 位（多一个 "1"），已改为实算的 64 位 `1604d6b3…b192e9b5`（与归档备份、新清单三方一致）。这是 poc-sm 本轮唯一的已跟踪文件改动，**未 commit**，随下一批文档提交入库。

没有 merge / cherry-pick / rebase / reset / clean / force push，没有修改 origin，没有切换任何既有工作区的分支，没有触碰 Android 生产路径。