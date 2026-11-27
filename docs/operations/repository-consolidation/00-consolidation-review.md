# PAE 仓库收敛评审（PAE-REPO-CONSOLIDATION-01）

> 状态：**只读审计、完整备份、Host proof 重建验证已完成**。目录归档、分支删除、提交迁移、commit/push **均未执行**，等负责人批准。
> 生成时间：2026-11-26 20:10 +08:00
> 仓库：`https://github.com/Vin535LFJ/ProceduralAvatarEngine`（默认分支 `main`，tag 数 0）
> 配套文件：[01-execution-checklist.md](01-execution-checklist.md)（实际执行记录与待批操作）、[02-local-workspace-inventory.md](02-local-workspace-inventory.md)（目录盘点表）
> 归档：`E:\AI-Tools\_PAE-Repository-Archive\`（2,859 个文件 / 761.8 MB，`SHA256SUMS.txt` 2,858 条，自校验 0 不符）

## 0. 结论

| 状态项 | 结论 | 依据 |
|---|---|---|
| `WORKSPACE_CONSOLIDATION` | **PARTIAL** | 唯一活动工作区已确认，10 项条件全部成立；但 feature 有 8 行独有记录未迁入，交接说明未入库 |
| `LOCAL_CLONE_ARCHIVE` | **NOT READY** | 主仓目录持有两个工作区共用的 `.git`，移走或删除会让 poc-sm 工作树失效 |
| `REMOTE_BRANCH_PRUNE` | **NOT READY** | m11/m12 的技术条件满足；是否被 PR 或其他 Agent 占用未核实，且需批准 |
| `FEATURE_COMMIT_MIGRATION` | **REQUIRED** | 仅 1 笔：`bf7f854`，1 个文件 +8 行，可干净落到 poc-sm |
| `ADR-014` | **DRAFT** | 未签署 |
| `VR-1B` | **BLOCKED** | 等待 ADR-014 批准与负责人授权 |

**一句话**：poc-sm 已经是事实上的唯一活动工作区并且可完整重建；本轮没有删、移、合并任何东西。主仓目录不能按“重复 Clone”处理——它不是副本，而是 poc-sm 所依赖的 Git 管理目录所在。

## 1. 本地目录状态（三个目录）

| # | 目录 | 性质 | 分支 | HEAD | 与远端 | 已跟踪改动 | 未跟踪 | 重要忽略文件 |
|---|---|---|---|---|---|---|---|---|
| 1 | `C:\Users\Giant\Desktop\GITCODE\Procedural Avatar Engine` | 主工作树，持有 `.git`（63.5 MB） | `feature/procedural-character-foundation` | `2ac101fd40e533d2f08776cb6f4e9356cd1b25d1` | 一致（0/0） | 无 | 5 个 / 0.08 MB | 5 个 / 57.3 MB |
| 2 | `E:\AI-Tools\ProceduralAvatarEngine-poc-sm` | 同一仓库的 linked worktree | `poc-sm/shape-model-v2` | `a0783a20f8d9dc6104f799d773ec8f081bc2abc3` | 一致（0/0） | 无 | 7 个 / 0.7 MB | 427 个 / 360.8 MB |
| 3 | `E:\AI-Tools\PAE-human-template-poc` | 非 Git 目录：外部原始资产与隔离实验区 | — | — | — | — | — | 2,357 个 / 274.7 MB（不含 `build\`） |

与任务书的出入（以实测为准）：

- **本地只有 1 个 Git clone，不是 3 个**。目录 1 和 2 共用同一个 `.git`；目录 2 的 `.git` 是一个文件，内容为 `gitdir: C:/Users/Giant/Desktop/GITCODE/Procedural Avatar Engine/.git/worktrees/ProceduralAvatarEngine-poc-sm`。
- 交接说明只记录了两个 Git 工作目录，与实测一致。
- 远端实际 5 条分支；本地另有 2 条没有上游的分支：`m13-procedural-retopology`（`bf5415f`）、`poc-a-g2-fair-comparison`（`2ac101f`），两者相对其他分支的独有提交均为 0。任务书未提到这两条。
- 两个工作区都没有 stash、没有子模块。

## 2. 唯一活动工作区建议

**`E:\AI-Tools\ProceduralAvatarEngine-poc-sm`，分支 `poc-sm/shape-model-v2`，HEAD `a0783a2`。** HEAD 与任务书给出的预期一致，没有更新的提交，未做任何 reset。

| # | 条件 | 结果 | 证据 |
|---|---|---|---|
| 1 | HEAD 包含 `0baa698` | 成立 | `merge-base --is-ancestor` 退出码 0 |
| 2 | HEAD 包含 `a0783a2` | 成立 | 即 HEAD |
| 3 | `CanonicalSkinGenerator.h` 存在 | 成立 | 主仓工作树上不存在 |
| 4 | Host-only proof 工具存在 | 成立 | `tools/human-template-poc/host/`，已跟踪 29 个文件 |
| 5 | Host-only proof 能重新构建 | 成立 | 见 §4 |
| 6 | 101 张图和对应日志存在 | 成立 | `evidence/host-proof/images` 84 + `images-proof-b` 17 + `logs` 10 |
| 7 | R1/R2 生产代码和证据完整 | 成立 | `299ae3b`、`f1b4404`、`66a7e59`、`9cdde4c` 均为 HEAD 的祖先；`evidence/` 222、`native/` 132、`tests/` 32 个已跟踪文件 |
| 8 | 未提交内容已登记 | 成立 | 7 个未跟踪 + 427 个重要忽略文件，逐个列入归档清单 |
| 9 | 外部原始资产路径明确 | 成立 | `E:\AI-Tools\PAE-human-template-poc\assets\`，已备份 |
| 10 | `.workbuddy-ai/` 和 v6/v7 证据已保护 | 成立 | 在归档 `untracked-backups/poc-sm/`，哈希一致 |

## 3. 备份

归档目录 `E:\AI-Tools\_PAE-Repository-Archive\`，备份时间 2026-11-26 19:55–19:56 +08:00。

| 内容 | 位置 | 结果 |
|---|---|---|
| Git 全历史（1 个 bundle，覆盖两个工作区） | `git-bundles/pae-all-refs.bundle` | 64,932,972 字节；`create` / `verify` 退出码均为 0；15 条引用（7 条本地分支、6 条远端跟踪、`HEAD`、poc-sm worktree 的 `HEAD`） |
| bundle SHA-256 | — | `346220339cc84f222e55d8845d5b90a71bf6565d9de82386c898a0bd071da24b` |
| staged / unstaged 补丁 | `inventory/*.patch` | 4 份均为 0 字节（两个工作区都没有已跟踪改动） |
| 未跟踪文件 | `untracked-backups/` | 主仓 5/5、poc-sm 7/7，逐文件哈希一致 |
| 重要忽略文件 | `ignored-data/` | 主仓 5/5、poc-sm 427/427，逐文件哈希一致 |
| 外部资产 | `external-assets/PAE-human-template-poc/` | 2,357/2,357，逐文件哈希一致 |
| 被排除的构建中间产物 | 仅登记清单 | 主仓 1,428 个、poc-sm 1,683 个（`.gradle`、`.cxx`、`build/` 等，可重新生成） |
| 全归档 SHA-256 清单 | `SHA256SUMS.txt` | 2,858 条，自校验 0 不符；清单自身 SHA-256 `b369d48790e445c2011147947c32d98f15a549f92edced03bc56918fbf2c6d7a` |
| 归档说明 | `README.txt` | 来源、目录结构、校验与恢复方法 |

被忽略文件里需要点名保护的都已备份：3 个已生成的模板（`.paht` / `.paehuman`）、VR-1A 的两个 APK、`build-host/` 下的历史日志与补丁、`libpae_core.a`、链 A 流水线输出（248 个文件）。原有的两份旧备份 `E:\AI-Tools\pae-snapshot-20261124`（220 个文件）和 `E:\AI-Tools\git-backups`（2 个文件）只登记、未复制、原样保留。

任务书结构里的 `clone-1/2/3` 子目录没有创建：本地只有一个 clone，按“bundle + 未跟踪 + 忽略文件”分开存放比按目录复制更完整，也避免把 63.5 MB 的 `.git` 与 bundle 重复保存。

## 4. Host-only proof 重建验证

从仓库内位置 `tools/human-template-poc/host/` 重新构建并运行，输出写到仓库外的临时目录。

| 步骤 | 结果 |
|---|---|
| 链 A 构建 | 0 警告 0 错误 |
| 链 A 测试 | `65 passed, 0 failed, 65 total`，65 个唯一编号，退出码 0 |
| 链 A 流水线 | 退出码 0，9.1 s，84 张图；模板 4,113,384 字节，写出→读回逐字节一致 |
| 无权重回退 | `body 175 kept-ancestor + 0 nearest-bone, aux 648 + 0, 0 ids unweighted` |
| 链 B proof | 编译 0 警告；退出码 0；`DIRECTION_CHECK actions=8 failed=0`；墙钟 9.59 s，峰值内存 38,019,072 字节 |
| 链 B 渲染 | 17 张 |
| 与已提交证据比对 | 经同一压缩步骤后，84/84 + 17/17 张图逐字节相同 |
| 仓库状态 | 已跟踪文件零改动，HEAD 仍为 `a0783a2` |

一个需要知道的事实：`.paht` 模板文件尾部内嵌 UTC 生成时间，所以两次运行生成的模板有 3 个字节不同（长度相同）。除此之外可复现。

## 5. 远端分支关系

远端事实取自 2026-11-26 20:04 +08:00 的 `git ls-remote --heads origin`（期间网络抖动过一次，重试后成功）。

| 远端分支 | SHA | 本地跟踪 | 说明 |
|---|---|---|---|
| `main` | `c539afbec299ed4a066429ea09f9d3d36e67f74d` | 一致 | 默认分支 |
| `feature/procedural-character-foundation` | `2ac101fd40e533d2f08776cb6f4e9356cd1b25d1` | 一致 | |
| `poc-sm/shape-model-v2` | `a0783a20f8d9dc6104f799d773ec8f081bc2abc3` | 一致 | 主开发分支 |
| `m11-anatomy-visual-rebuild` | `67ccb3940950623b1a24d6b3e843750c5c4a99b3` | 一致 | |
| `m12-unified-human-geometry` | `c539afbec299ed4a066429ea09f9d3d36e67f74d` | 无上游 | 与 `main` 同一提交 |

```text
main (c539afb)                      ← feature 与 poc-sm 的共同祖先之一，两者都严格领先于它
  │
  ├── feature/procedural-character-foundation (2ac101f)   仅此分支独有：11 笔
  │        与 poc-sm 的共同祖先：a32251b
  └── poc-sm/shape-model-v2 (a0783a2)                     仅此分支独有：16 笔
           ├─ 0baa698  Host-only 人体模板工具
           └─ a0783a2  研究文档与证据
```

- `main` 是 `feature` 和 `poc-sm` 两者的祖先（`merge-base --is-ancestor` 均为 0）。
- 两棵文件树的差异：727 个文件，+113,762 / −551 行。

## 6. feature 的 11 笔独有提交逐笔分类

判定方法：不看补丁是否等价，而是逐文件比较两边 HEAD 的内容（忽略换行符差异）。11 笔提交共触及 56 个路径：46 个内容已在 poc-sm，3 个是被 feature 自己的后续提交删掉的草稿，7 个内容有差异。

7 个有差异的文件里，6 个是 poc-sm 上的版本更新（feature 上的行是被改写前的旧版本），只有 1 个文件含有 feature 独有的实质内容：

| 文件 | feature 有而 poc-sm 没有的内容 | 实际性质 |
|---|---|---|
| `10-decision-records/04-AUTH-08-granted.md` | 8 行“AUTH-08 执行结果记录” | **feature 独有** |
| `07-open-issues.md` | OQ-01/14/16/17/21 的旧版行 | poc-sm 上是 round 2 的扩展版 |
| `06-owner-signoff-checklist.md` | “5 项已签署，1 项待决”旧版 | poc-sm 上是 round 2 版，已记录 DEC-05 决策 |
| `09-…/03-canonical-acceptance-sheet-v0.md` | “体型原型三选一”待决段 | poc-sm 上已选定 C |
| `docs/project-status.md` | “本文件已非权威状态源”提示段 | poc-sm 上已重写为一页式当前状态 |
| `12-poc-sm-vertical-slice.md` | 1 行旧测试数字 | poc-sm 上已更新 |
| `.gitattributes` | 无 | poc-sm 是超集 |

逐笔分类：

| 提交 | 说明 | 分类 | 处理 |
|---|---|---|---|
| `e8e697e` | G3 DEFER 提案与独立路线评审提示词 | B 已被带入 | 无 |
| `92765cb` | 记录 G3 DEFER、视觉 rubric、ADR 草稿 | B 已被带入（2 个草稿后被删除） | 无 |
| `f792d52` | G3 决策记录草稿、签署清单 | B 已被 poc-sm round 2 版取代 | 无 |
| `7c451ae` | 待决问题总表 | B 已被 poc-sm round 2 版取代 | 无 |
| `1ce1a21` | DEC-05 与 AUTH-08 评审包归档 | B 已被带入；验收单已被 poc-sm 版取代 | 无 |
| `a2f2783` | 签署 ADR-011/012、OQ-03/04/05、AUTH-08 | B 决策记录已在 poc-sm（经 `3b3532e` 带入） | 无 |
| `d085ff3` | 删除被取代的 G3 决策记录草稿 | B 两边都已没有该文件 | 无 |
| `bf7f854` | 记录 AUTH-08 执行结果与分支 SHA | **A 必须保留** | 待批准迁入 |
| `a921c00` | 原工作区未提交改动盘点 | 补丁等价（poc-sm 的 `582952d`） | 无 |
| `a09428b` | 把垂直切片报告镜像到主分支 | B poc-sm 版本更新 | 无 |
| `2ac101f` | 采纳轮次验收契约（AGENT.md + ADR-013） | 补丁等价（poc-sm 的 `8d1377e`） | 无 |

A 类 1 笔，B 类 8 笔，补丁等价 2 笔；C、D、E 类均为 0。

`bf7f854` 的迁移已做过只读预检：补丁对 poc-sm 的工作区和已提交树 `git apply --check` 均干净，未实际应用。

## 7. 是否应当完整合并 feature 与 poc-sm

**不应当。** 理由：

- 只读合并模拟（`git merge-tree`）的冲突文件正好是上面 7 个，全部在 `docs/` 和 `.gitattributes`；没有代码冲突，也没有任何代码收益。
- feature 相对 poc-sm 的全部实质增量是 1 个文件里的 8 行。合并会把 7 个已被 poc-sm 取代的文档版本重新拉成冲突。
- 没有执行 `merge`，也没有让两个分支指向同一提交。

## 8. 远端分支处置建议

| 分支 | 默认分支 | 独有提交 | tag 依赖 | 本地检出 | 已入 bundle | 建议 |
|---|---|---|---|---|---|---|
| `poc-sm/shape-model-v2` | 否 | 16 | 无 | poc-sm | 是 | 唯一主开发分支 |
| `main` | **是** | — | 无 | 否 | 是 | 保留为稳定基线 |
| `feature/procedural-character-foundation` | 否 | 11 | 无 | 主仓 | 是 | 暂时只读保留，直到 `bf7f854` 迁入 |
| `m11-anatomy-visual-rebuild` | 否 | 相对 `main` 为 0 | 无 | 否 | 是 | 候选删除 |
| `m12-unified-human-geometry` | 否 | 与 `main` 同一提交 | 无 | 否 | 是 | 候选删除 |

**未核实项**（本机没有安装 `gh`）：是否有 open PR、是否有其他 Agent 在使用这些分支。任务书要求的 7 项检查里这两项没有完成，所以 `REMOTE_BRANCH_PRUNE` 判为 NOT READY。

本地另有两条不在远端的分支 `m13-procedural-retopology`、`poc-a-g2-fair-comparison`，独有提交均为 0，已包含在 bundle 中；它们不涉及远端删除。

## 9. 建议保留和删除的目录

| 目录 | 建议 |
|---|---|
| `E:\AI-Tools\ProceduralAvatarEngine-poc-sm` | 保留，唯一活动工作区 |
| `E:\AI-Tools\_PAE-Repository-Archive` | 保留，只读归档 |
| `C:\Users\Giant\Desktop\GITCODE\Procedural Avatar Engine` | **暂不能归档或删除**：poc-sm 的 Git 管理目录在它里面 |
| `E:\AI-Tools\PAE-human-template-poc` | 保留为外部资产来源；已有归档副本 |
| `E:\AI-Tools\pae-snapshot-20261124`、`E:\AI-Tools\git-backups` | 旧备份，原样保留，本轮未动 |

主仓目录的处置有两条路，需要负责人选：

1. **原地保留、只读**（推荐，零风险）：不在该目录开发、不切换分支，仅作为 `.git` 的宿主。缺点是项目仍跨两块盘，归档目录里不会出现“clone-1”这样的实体目录。
2. **把 poc-sm 变成独立 clone**（需批准，步骤多）：从远端或本地重新 clone 到 E 盘，从归档恢复 7 个未跟踪文件和 427 个忽略文件，重建 `libpae_core.a` 并重跑两条链验证通过后，主仓目录才能归档。好处是彻底脱离 C 盘（C 盘当前剩余约 25 GB，E 盘约 136 GB）。

## 10. 07-handover-note 校验

| 项 | 结果 |
|---|---|
| 原件 | `E:\AI-Tools\ProceduralAvatarEngine-poc-sm\docs\research\human-template-reuse\07-handover-note.md`，存在，未跟踪 |
| 副本 | `C:\Users\Giant\Desktop\GITCODE\Procedural Avatar Engine\07-handover-note.md`，存在 |
| SHA-256 | 两份均为 `8bfa6e4d847ed97db056a06763740ac85344248d91e76b5a7c54b03c34be959a`，一致 |
| 处理 | 原件保留并已备份；副本未删除；是否入库待批准 |

## 11. ADR-014 状态

`DRAFT`，未签署。草案在 `docs/architecture/human-template/02-human-geometry-architecture-decision.md`。批准前需要同步处理的旧文件：`project-charter.md`、ADR-004、product-scope，以及 11-25 研究记录里“MakeHuman 网格不导入”的约束。本轮没有改写任何一份。

## 12. Rig / Weights 许可证状态

`default.mhskel`、`default_weights.mhw` 仍为 `PENDING_CONFIRMATION`：两个文件只在文件内部声明 CC0，官方 `LICENSE.md` 没有逐项列出。工具清单与文档中的标签已统一为待确认。本轮没有修改任何骨骼或渲染 ABI。

## 13. 人眼视觉验收状态

未完成。101 张 Host 证据图至今只经过图像描述模型检查，没有负责人亲自看过。三类验证要分开记：

| 验证 | 状态 |
|---|---|
| 图像描述模型检查 | 已做，结论不可靠，不能当验收 |
| 负责人亲自查看 | **未做** |
| Android GLES3 设备验证 | **未做** |

## 14. VR-1B 启动条件

1. ADR-014 获批。
2. Rig / Weights 两个文件的许可得到确认，或决定由 PAE 自建 Rig V2 与权重。
3. 负责人完成人眼视觉验收。
4. 本报告 §15 的迁移与入库获准并执行。

满足后的第一个 Android 结果：在不修改 LegacyRigV1 的前提下，把 Host proof 生成的 `.paht` 模板加载到 Android 并显示完整静态人体。顺序为 WP0 → WP1 → WP2，本轮不启动。

## 15. 未完成事项

- `bf7f854` 的 8 行未迁入 poc-sm。
- `07-handover-note.md` 和本目录下的三份文档都未入库。
- 主仓目录未归档（见 §9）。
- 远端 m11 / m12 未删除；open PR 与其他 Agent 占用情况未核实。
- ADR-014 未签署；Rig / Weights 许可未确认；人眼验收未做。
- Android 侧一个字节未改。

## 16. 风险与回退

| 风险 | 说明 | 回退 |
|---|---|---|
| 归档本身出错 | 可能性低：每个文件复制后都与源做了哈希比对 | `SHA256SUMS.txt` 可在任意机器上重校验 |
| bundle 损坏 | 可能性低：`verify` 退出码 0，15 条引用齐全 | 从远端重新 clone；本地独有的两条分支只在 bundle 里 |
| 误动主仓目录 | 会让 poc-sm 工作树失效 | 本轮未动；执行任何目录操作前先确认 §9 的选择 |
| 网络抖动 | 本轮 `ls-remote` 失败过一次 | 远端事实已在归档 `inventory/remote-refs.txt` 留档 |

本轮没有不可逆操作：没有删除任何目录或分支，没有 merge、cherry-pick、rebase、reset、clean，没有修改 origin，没有 commit/push。

## 17. 对任务书 11 个问题的回答

1. **哪些是重复 Clone？** 没有重复 Clone。本地只有一个 clone 和它的两个工作树。
2. **哪个是唯一活动工作区？** `E:\AI-Tools\ProceduralAvatarEngine-poc-sm`，分支 `poc-sm/shape-model-v2`。
3. **其他目录能否安全归档？** 暂不能。主仓目录是 Git 管理目录的宿主。
4. **是否有未提交且只有某个目录拥有的成果？** 有，且都已备份：主仓的 `.workbuddy-ai/memory/2026-11-23.md` 和 `.workbuddy-ai/skills/round-acceptance/SKILL.md`；poc-sm 的交接说明原件、v6/v7 截图、`.workbuddy-ai/memory/2026-11-24.md`；外部原始资产。
5. **feature 的 11 笔哪些要保留？** 只有 `bf7f854`（8 行）。
6. **是否有理由完整合并？** 没有。
7. **m11 / m12 能否安全删除？** 技术上可以；还需确认没有 open PR 和其他 Agent 占用。
8. **main 是否保留为稳定基线？** 是，它是默认分支，且两条开发分支都领先于它。
9. **交接说明是否应加入 Git？** 建议加入，它只有一份原件。
10. **ADR-014 和 Rig/Weights 许可还差什么？** 都差负责人的决定。
11. **VR-1B 最小 Android 验证如何启动？** 见 §14。
