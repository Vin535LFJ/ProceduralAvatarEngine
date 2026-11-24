# 08 — AUTH-08 授权范围与 Worktree 路径候选

> 状态：**授权书草案 + 只读预检**。
> **本轮未执行 `git worktree add`，未创建分支，未修改任何文件。**
> 全部预检命令均为只读（`ls` / `df` / `git worktree list` / `git ls-files` / `git branch --list`）。

---

## 1. AUTH-08 授权书（草案，待负责人签署）

### 1.1 授权标识

| 字段 | 值 |
|---|---|
| 授权号 | **AUTH-08** |
| 标题 | PoC-SM / Shape Model v2 形态 PoC 实施授权 |
| 状态 | **DRAFT — 待负责人签署** |
| 签发人 | **项目负责人**（AI 不得代签） |
| 依据 | DEC-09（已批准分层契约）、ADR-011（本轮 06-A 签署稿）、G3 DEFER 记录（本轮 07 签署稿） |
| 关联 OQ | OQ-02（主项）、OQ-33（子项：姿态轴协议是否纳入范围） |

### 1.2 授权的技术边界

| # | 允许 | 位置 |
|---|---|---|
| AL-1 | 新建 `native/procedural/shape/v2/` | 独立目录，与现有 `shape/` 并列 |
| AL-2 | 新建 host-only 契约（`CharacterDefinition` 五层子契约的 C++ 表达） | `native/procedural/shape/v2/` |
| AL-3 | 实现 **Shared Skin Envelope**（胸腔 B-spline guide + 腹盆 implicit volume field + 肩带 shared field + 凹面约束 → 一份最终外表面） | `native/procedural/shape/v2/` |
| AL-4 | 对应 host tests | `tests/unit/` |
| AL-5 | Before/After 证据生成工具（灰模、Before/After 同框对照） | `tests/unit/` 或 `tools/` |
| AL-6 | 参数影响差分测试（逐参数，跨区域影响量化） | `tests/unit/` |
| AL-7 | 姿态轴协议扩展：`applyPose` + 裁剪带参数化 `band = none \| torso`（**若 OQ-33 批准纳入**） | `native/procedural/shape/` 的评估工具侧 |

### 1.3 禁止（越界即视为违反授权）

| # | 禁止 |
|---|---|
| FB-1 | 修改 **BoneId**（0–21） |
| FB-2 | 修改 **UBO[22]** |
| FB-3 | 修改 **64B GPU vertex packing** |
| FB-4 | 修改 **JNI ABI** |
| FB-5 | 修改 **RenderThread 生产入口** |
| FB-6 | 替换 legacy 三路径（M11 / M12 / P1-A） |
| FB-7 | 修改 **Android 生产渲染路径** |
| FB-8 | 引入外部模型资产（任何 GLB/GLTF/FBX/OBJ 角色资产、外部贴图） |
| FB-9 | 引入 **Three.js** 或任何 JS Runtime |
| FB-10 | 通用 quad remeshing / 通用 SDF sculpting / 完整 NURBS 编辑器 |
| FB-11 | 修改 legacy 三路径中的任何生产 C++ 文件（`SdfFusion.cpp` 等已冻结项） |
| FB-12 | 宣称任何 Gate 为 PASS |

### 1.4 授权的终止与回滚

| # | 条款 |
|---|---|
| RB-1 | 全部 PoC-SM 代码位于独立 worktree 的 host-only 目录；**不触碰生产入口与 ABI** |
| RB-2 | 失败即丢弃该分支；`a32251b` 的 G2 证据保持不变 |
| RB-3 | 撤销授权即删除该 worktree 与分支；本仓库 `origin/feature/procedural-character-foundation` 不受影响 |

### 1.5 签署

```
AUTH-08 状态： DRAFT / GRANTED / DENIED      ← 负责人圈选
基线 commit：  a32251bf798e46159696a637a699a062453df345  （或负责人另指定：________）
Worktree 路径： 候选 1 / 候选 2 / 候选 3 / 其他：____________________
分支名：        poc-sm/shape-model-v2
姿态轴协议（OQ-33）： 纳入 AL-7 / 不纳入
负责人签字：____________________  日期：____________
```

---

## 2. Worktree 路径候选（3 个）

`[VERIFIED]` **已核实的仓库父目录 = `E:/AI-Tools/`**（仓库根 `E:/AI-Tools/ProceduralAvatarEngine-PoC` 的直接父目录）。

| 候选 | 路径 | 状态 | 特点 |
|---|---|---|---|
| **候选 1** | `E:/AI-Tools/ProceduralAvatarEngine-poc-sm` | **空闲** | 与主仓 `ProceduralAvatarEngine-PoC` 同级，命名规则严格符合 `<父目录>/ProceduralAvatarEngine-poc-sm`。路径最短，最不易混淆 |
| **候选 2** | `E:/AI-Tools/poc-sm/ProceduralAvatarEngine-poc-sm` | **空闲**（父目录 `E:/AI-Tools/poc-sm` 亦空闲，需先创建） | 把 PoC-SM 相关 worktree 集中在专用分组目录，便于将来 PoC-SHOULDER / PoC-LIMB / PoC-FULL 并列管理。**路径需先建父目录** |
| **候选 3** | `E:/AI-Tools/ProceduralAvatarEngine-PoC-sm` | **空闲** | 与候选 1 语义相同、仅大小写不同。**与现有仓库 `ProceduralAvatarEngine-PoC` 仅一字之差，是三条中误操作风险最高的一条** |

`[RECOMMENDATION]` **推荐候选 1**。理由：满足路径规则、零歧义、与现有 `ProceduralAvatarEngine-PoC` 名称区分度最大（`poc-sm` vs `PoC`）。若负责人希望集中管理后续 PoC 分支，选候选 2。**不建议候选 3**——与现有仓库名仅差一个后缀，人工操作时极易混淆。

---

## 3. 路径预检（五项硬性检查全部实测）

| # | 检查项 | 候选 1 | 候选 2 | 候选 3 | 实测证据 |
|---|---|---|---|---|---|
| C-1 | **目录不存在**（不会覆盖任何已有内容） | ✅ 不存在 | ✅ 不存在（父目录 `poc-sm` 也不存在） | ✅ 不存在 | `ls -la E:/AI-Tools/` 输出仅含 `DEC-05-Review-Drafts`、`ProceduralAvatarEngine-PoC`、`SHA256SUMS.txt`、`_mp_skills_verify`、`dns-split`、`doc-governance`、`gemini-web2api.linux`、`llm-hub-sync`、`pet-plugin-inspect`、`pet-sdk-governance`、`skills-zh-full`、`test`、`test_skill`；三个候选路径均判定 `FREE` |
| C-2 | **不覆盖任何已有 Worktree** | ✅ | ✅ | ✅ | `git worktree list --porcelain` 仅列出 `C:/Users/Giant/Desktop/GITCODE/Procedural Avatar Engine`（`99941a9`）与 `E:/AI-Tools/ProceduralAvatarEngine-PoC`（`7c451ae`）；三者均无交集 |
| C-3 | **不覆盖用户文件** | ✅ | ✅ | ✅ | 同 C-1；三个路径及其父目录均为空闲或需新建，无任何既有文件 |
| C-4 | **目标磁盘空间充足** | ✅ | ✅ | ✅ | `df -h E:/` → `325G total, 185G used, 141G avail (57%)`；本仓**受版本控制的检出体积实测仅 18 MB**（`git ls-files -z \| xargs -0 du -ch` → `18M total`；其中 docs 366 文件、evidence 146、native 90、tests 16）。新 worktree 占用可忽略 |
| C-5 | **与原工作区隔离** | ✅ | ✅ | ✅ | 原工作区 = `C:/Users/Giant/Desktop/GITCODE/Procedural Avatar Engine`（WT-1，C 盘）。三候选均在 `E:/AI-Tools/` 下，与 WT-1 **不同盘符、不同路径**。注意：`E:/AI-Tools/ProceduralAvatarEngine-PoC`（WT-2）**不是**原工作区，而是本项目的 PoC-A/G2 工作区 |

`[VERIFIED]` **五项全部通过**，三条候选路径均可用；差异只在目录组织方式。

### 3.1 附加预检

| # | 检查项 | 结果 | 证据 |
|---|---|---|---|
| X-1 | 分支名 `poc-sm/shape-model-v2` 是否已被占用 | **未占用** | `git branch -a --list "*poc-sm*"` → 无输出 |
| X-2 | 新 worktree 的构建产物是否会被误提交 | **不会** | `.gitignore` 已含 `build/`、`build-host/`、`*.apk`、`.cxx/`、`.gradle/`、`.idea/`、`local.properties` |
| X-3 | 基线 commit 是否可作为 worktree 起点 | **可以** | `git cat-file -t a32251bf…` → `commit`；`git merge-base --is-ancestor a32251b HEAD` → 成功 |
| X-4 | 新 worktree 是否会与本仓库共享 object store | **会**（worktree 机制本身） | `.git` 在 worktree 中是文件而非目录，磁盘占用不重复 |
| X-5 | C 盘余量是否够用 | **不适用**（三候选均在 E 盘） | `df -h C:/` → `44G avail (86%)`；原工作区在 C 盘，但本轮不改动它 |

---

## 4. 基线与治理上下文带入（必须执行，否则 PoC-SM 会缺授权记录）

`[VERIFIED — 本轮 01 §5.5]` `a32251b` 与当前 HEAD 之间的 4 个提交**全部是 `docs/` 变更**，不含任何代码/测试/证据改动。

`[RECOMMENDATION]` 在新 worktree 中执行（**负责人批准后**）：

```bash
# 1) 以指定基线创建 worktree（detached，隔离分支）
git -C "E:/AI-Tools/ProceduralAvatarEngine-PoC" worktree add \
    --detach "E:/AI-Tools/ProceduralAvatarEngine-poc-sm" \
    a32251bf798e46159696a637a699a062453df345

# 2) 在新 worktree 内建分支
git -C "E:/AI-Tools/ProceduralAvatarEngine-poc-sm" switch -c poc-sm/shape-model-v2

# 3) 带入治理上下文（4 个 docs 提交的最终形态）
git -C "E:/AI-Tools/ProceduralAvatarEngine-poc-sm" checkout 7c451ae -- docs/
```

`[VERIFIED]` 第 3 步的效果 = 新 worktree 的 `docs/` 树与 `7c451ae` **完全一致**，包含：SPEC register 的 `GATE-04 DEFER` / `DEC-09 APPROVED` / `AUTH-08 OPEN` 状态、`docs/ADR_INDEX.md` 的 ADR-011/012 登记、02–07 系列阶段文档、ADR-011/012 草案。

`[RECOMMENDATION]` **采用 `checkout -- docs/` 而非 `cherry-pick`** 的理由：
- cherry-pick 4 个提交会在新分支产生 4 个与 PoC-SM 无关的提交，污染分支历史；
- `checkout -- docs/` 只取文件树，不产生提交；
- 两种方式产出的 `docs/` 内容**完全等价**。

`[INFERENCE]` **不做这一步的后果**：PoC-SM 将在一份**没有 G3 DEFER 记录、没有 DEC-09、没有 AUTH-08 状态行**的 SPEC 上开发，"是否已获授权"将不可审计。这是本轮审计发现的**唯一实际风险点**（本轮 01 的 A-5）。

---

## 5. AUTH-08 授权后的预期产出（供负责人判断授权范围是否合适）

| # | 产出 | 归属 Gate |
|---|---|---|
| 1 | `native/procedural/shape/v2/` 下的五层契约 host-only 实现 | — |
| 2 | Shared Skin Envelope 实现，产出一份连续外表面（无独立肩球 / 无圆管 / 无 append 拼接） | `G-P1-GEO` |
| 3 | Before/After 灰模对照（同相机/同光/同带/同取景） | `G-P1-GEO` |
| 4 | 肩根接缝复测（按本轮 07 §1.3 固定口径，≤5°） | `G-P1-GEO` |
| 5 | landmark / 7 视角轮廓 / 10 段截面偏差（阈值待 DEC-05 批准） | `G-P1-GEO` |
| 6 | 逐参数跨区域影响差分测试 | `G-P1-GEO` + `G-P1-TOPO` |
| 7 | 拓扑审计（components / 未登记 boundary / non-manifold / degenerate / sliver / 正交 quad 比 / 顶点 ID 稳定性） | `G-P1-TOPO` |
| 8 | 权重合法性 + CPU 变形 + 姿态变形对比（若 OQ-33 纳入） | `G-P1-ANIM` |
| 9 | 以新 Target Reference 重跑 G2 的证据 | G2 重开 |

`[VERIFIED]` **PoC-SM 自身的产出不等于任何 Gate 通过。** 它是"重跑 G2 的前置材料"，Gate 判定仍需负责人按 DEC-05 rubric 执行。

---

## 6. 本轮未执行的动作（逐条声明）

`[VERIFIED]` 以下动作**本轮均未执行**：

| # | 动作 | 状态 |
|---|---|---|
| 1 | `git worktree add` | **未执行** |
| 2 | `git worktree remove` | **未执行** |
| 3 | `git branch`（创建任何分支） | **未执行**（仅执行了只读的 `git branch --list`） |
| 4 | `git switch` / `git checkout` | **未执行** |
| 5 | 创建候选目录 | **未执行** |
| 6 | 修改 C++ 源码 | **未执行** |
| 7 | 编译 / 部署 | **未执行** |
| 8 | Commit / Push | **未执行** |
| 9 | 触碰原工作区 `C:/Users/Giant/Desktop/GITCODE/Procedural Avatar Engine` | **未执行** |

`[VERIFIED]` 仓库 `E:/AI-Tools/ProceduralAvatarEngine-PoC` 在本轮结束时 `git status --short` 仍为空，HEAD 仍为 `7c451ae18137316291a4a51ecd03e57d2bea2d0f`，`git worktree list` 仍只有 2 项。
