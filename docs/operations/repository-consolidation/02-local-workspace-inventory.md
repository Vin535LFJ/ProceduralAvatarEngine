# PAE 本地工作区盘点表（PAE-REPO-CONSOLIDATION-01）

> 数据来源：2026-11-26 19:53–20:05 +08:00 的只读检查，原始输出保存在 `E:\AI-Tools\_PAE-Repository-Archive\inventory\`。
> 结论见 [00-consolidation-review.md](00-consolidation-review.md)。

## 总览

| 目录 | 性质 | 建议 |
|---|---|---|
| `E:\AI-Tools\ProceduralAvatarEngine-poc-sm` | Git linked worktree，本轮全部成果所在 | **唯一活动工作区** |
| `C:\Users\Giant\Desktop\GITCODE\Procedural Avatar Engine` | Git 主工作树，持有两个工作区共用的 `.git` | 保留、只读；暂不可归档 |
| `E:\AI-Tools\PAE-human-template-poc` | 非 Git：MakeHuman CC0 原始资产、MPFB2 格式文档、隔离实验区 | 保留为外部资产来源 |
| `E:\AI-Tools\pae-snapshot-20261124` | 非 Git：旧备份，220 个文件 / 61.4 MB | 原样保留 |
| `E:\AI-Tools\git-backups` | 非 Git：旧备份，2 个文件 / 39 MB | 原样保留 |
| `E:\AI-Tools\_PAE-Repository-Archive` | 本轮新建的只读归档 | 保留 |

## 目录 1：`C:\Users\Giant\Desktop\GITCODE\Procedural Avatar Engine`

| 字段 | 内容 |
|---|---|
| Directory Path | `C:\Users\Giant\Desktop\GITCODE\Procedural Avatar Engine` |
| Git Root | 同上（`git rev-parse --git-common-dir` = `.git`；1,472 个文件 / 63.5 MB） |
| Current Branch | `feature/procedural-character-foundation` |
| HEAD SHA | `2ac101fd40e533d2f08776cb6f4e9356cd1b25d1` |
| Remote | `https://github.com/Vin535LFJ/ProceduralAvatarEngine.git` |
| Remote HEAD | `2ac101fd40e533d2f08776cb6f4e9356cd1b25d1`（`ls-remote`，20:04） |
| Ahead/Behind | 0 / 0 |
| Tracked Changes | 无（staged 补丁 0 字节，unstaged 补丁 0 字节） |
| Untracked | 5 个文件 / 79,336 字节 |
| Ignored Important | 5 个文件 / 60,089,020 字节 |
| Proof Dependencies | `native/procedural/humanoid/canonical/CanonicalSkinGenerator.h` **不存在**；`build-host/libpae_core.a` **不存在** → 链 B 无法在此重建 |
| Unique Content | 见下 |
| Recommendation | 保留、只读；不归档、不删除 |

未跟踪文件（已备份）：

```text
.qoder-input-files/d02b9d65-….txt                      本次专家附件
.workbuddy-ai/memory/2026-11-23.md                     仅此目录有
.workbuddy-ai/memory/2026-11-24.md
.workbuddy-ai/skills/round-acceptance/SKILL.md         仅此目录有
07-handover-note.md                                    交接说明的展示副本
```

重要忽略文件（已备份）：

```text
.zcode/plans/plan-sess_005c883a-….md
app/build/outputs/apk/debug/app-debug.apk
app/build/outputs/apk/debug/output-metadata.json
app/build/outputs/logs/manifest-merger-debug-report.txt
local.properties
```

被排除的构建中间产物：1,428 个文件（`.gradle/`、`app/.cxx/` 等），可重新生成。

## 目录 2：`E:\AI-Tools\ProceduralAvatarEngine-poc-sm`

| 字段 | 内容 |
|---|---|
| Directory Path | `E:\AI-Tools\ProceduralAvatarEngine-poc-sm` |
| Git Root | linked worktree；`.git` 文件指向 `C:/Users/Giant/Desktop/GITCODE/Procedural Avatar Engine/.git/worktrees/ProceduralAvatarEngine-poc-sm` |
| Current Branch | `poc-sm/shape-model-v2` |
| HEAD SHA | `a0783a20f8d9dc6104f799d773ec8f081bc2abc3` |
| Remote | 同上 |
| Remote HEAD | `a0783a20f8d9dc6104f799d773ec8f081bc2abc3`（`ls-remote`，20:04） |
| Ahead/Behind | 0 / 0 |
| Tracked Changes | 无（两份补丁均为 0 字节） |
| Untracked | 7 个文件 / 742,727 字节 |
| Ignored Important | 427 个文件 / 378,284,466 字节 |
| Proof Dependencies | `CanonicalSkinGenerator.h` 存在；`build-host/libpae_core.a` 存在；`tools/human-template-poc/host/` 已跟踪 29 个文件 → 链 A、链 B 均可重建（本轮已验证） |
| Unique Content | P1-VR-ARCH-1 全部成果、VR-1A / VR-1A-R2 证据、交接说明原件、v6/v7 截图 |
| Recommendation | **唯一活动工作区** |

未跟踪文件（已备份）：

```text
.qoder-input-files/4e253c55-….md
.workbuddy-ai/memory/2026-11-24.md
docs/research/human-template-reuse/07-handover-note.md   交接说明原件
evidence/character-v6/anatomy-clay.png
evidence/character-v6/anatomy-shoulder.png
evidence/character-v7/face.png
evidence/character-v7/front.png
```

盘点之后的变化（2026-11-27）：交接说明原件和 `docs/operations/repository-consolidation/` 下的治理文档已入库并推送；poc-sm 的未跟踪文件剩 6 个。

重要忽略文件的构成：

| 类别 | 数量 | 大小 |
|---|---|---|
| `docs/phases/P1-visual-recovery/evidence/vr1a-r2/implementation/` 下两个 APK | 2 | 117.5 MB |
| `app/build/outputs/` 下 APK 与构建报告 | 3 | 58.8 MB |
| `build-host/human-template-poc/out/` 链 A / 链 B 输出 | 248 | 80.6 MB |
| `build-host/*.bin`（P1B 渲染与 canonical 转储） | 6 | 84.0 MB |
| `build-host/libpae_core.a` | 1 | 10.8 MB |
| `build-host/` 根下的历史日志、补丁、探针源码、阶段性源码快照与可执行文件 | 64 | 5.1 MB |
| `build-host/human-template-poc/` 的构建产物 | 103 | 4.0 MB |

合计 427 个文件 / 378,284,466 字节。

其中需要点名保护的文件：`pae-human-template-mh-a8bc2d54.paht`、`pae-human-template-test.paht`、`base.paehuman`、`vr1a-r1-baseline.apk`、`vr1a-r2.apk`。

被排除的构建中间产物：1,683 个文件。

## 目录 3：`E:\AI-Tools\PAE-human-template-poc`（非 Git）

| 字段 | 内容 |
|---|---|
| Directory Path | `E:\AI-Tools\PAE-human-template-poc` |
| Git Root | 无 |
| 内容 | `assets\makehuman-a8bc2d54\…\`（固定 SHA 的 MakeHuman 归档，含 `base.obj`、targets、`default.mhskel`、`default_weights.mhw`、`LICENSE.md`）、`assets\mpfb2\`（3 份格式文档）、`assets\_download\`（2 个 zip）、`host\`、`out\` |
| 文件数 / 大小 | 2,357 个 / 274.7 MB（不含 `build\`） |
| Unique Content | 原始第三方资产的唯一本地副本；仓库内工具运行时需要读取它 |
| 备注 | 2026-11-26 11:46–12:09 另有一个代理在此目录并行写入过文件 |
| Recommendation | 保留为外部资产来源；归档中已有完整副本 |

## 分支清单（本地 7 条，远端 5 条）

| 分支 | 提交 | 上游 | 独有提交 | 检出于 |
|---|---|---|---|---|
| `poc-sm/shape-model-v2` | `a0783a2` | `origin/poc-sm/shape-model-v2` | 16（相对 feature） | poc-sm |
| `feature/procedural-character-foundation` | `2ac101f` | `origin/feature/…` | 11（相对 poc-sm） | 主仓 |
| `main` | `c539afb` | `origin/main` | — | 无 |
| `m11-anatomy-visual-rebuild` | `67ccb39` | `origin/m11-…` | 相对 `main` 为 0 | 无 |
| `m12-unified-human-geometry` | `c539afb` | 无 | 与 `main` 同一提交 | 无 |
| `m13-procedural-retopology` | `bf5415f` | 无（仅本地） | 0 | 无 |
| `poc-a-g2-fair-comparison` | `2ac101f` | 无（仅本地） | 与 feature 同一提交 | 无 |

tag：0 个。stash：无。子模块：无。
