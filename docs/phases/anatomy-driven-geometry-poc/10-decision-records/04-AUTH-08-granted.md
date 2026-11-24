# AUTH-08 授权书 — PoC-SM / Shape Model v2 形态 PoC 实施授权

> **状态：`GRANTED`（已签发）** — 2026-11-24。
> 关闭 OQ-02（主项）与 OQ-33（子项）。
> 授权号：**AUTH-08**。

## 决策记录（DECISION RECORD）

| 字段 | 值 |
|---|---|
| 授权号 | **AUTH-08** |
| 标题 | PoC-SM / Shape Model v2 形态 PoC 实施授权 |
| 状态 | **`GRANTED`** |
| 签发人 | **项目负责人** |
| 签发日期 | **2026-11-24** |
| 决策来源 | 负责人在 WorkBuddy 会话中明确选择「签 4 份 + 下发 AUTH-08」执行包，并选定「新建独立 worktree」落地形态 |
| 记录方式 | AI **仅作转录与归档**；未代签，未添加负责人未表达的判断 |

## 1. 授权参数（负责人指定值）

| 字段 | 值 |
|---|---|
| **基线 commit** | `a32251bf798e46159696a637a699a062453df345`（G0/G1/G2 证据冻结点） |
| **Worktree 路径** | `E:/AI-Tools/ProceduralAvatarEngine-poc-sm`（候选 1） |
| **分支名** | `poc-sm/shape-model-v2` |
| **姿态轴协议（OQ-33）** | **纳入 AL-7** —— `applyPose` + 裁剪带参数化 `band = none \| torso` |
| 依据 | DEC-09（已批准分层契约）、ADR-011（已签署）、G3 DEFER 记录（已签署） |
| 关联 OQ | **OQ-02**（主项）、**OQ-33**（子项） |

> **为何选候选 1**：满足路径规则、零歧义、与现有仓库 `ProceduralAvatarEngine-PoC` 名称区分度最大。
> 候选 3（`ProceduralAvatarEngine-PoC-sm`）与现有仓库名仅一字之差，是三条中误操作风险最高的一条，**不建议使用**。

## 2. 授权的技术边界（允许）

| # | 允许 | 位置 |
|---|---|---|
| AL-1 | 新建 `native/procedural/shape/v2/` | 独立目录，与现有 `shape/` 并列 |
| AL-2 | 新建 host-only 契约（`CharacterDefinition` 五层子契约的 C++ 表达） | `native/procedural/shape/v2/` |
| AL-3 | 实现 **Shared Skin Envelope**（胸腔 B-spline guide + 腹盆 implicit volume field + 肩带 shared field + 凹面约束 → 一份最终外表面） | `native/procedural/shape/v2/` |
| AL-4 | 对应 host tests | `tests/unit/` |
| AL-5 | Before/After 证据生成工具（灰模、Before/After 同框对照） | `tests/unit/` 或 `tools/` |
| AL-6 | 参数影响差分测试（逐参数，跨区域影响量化） | `tests/unit/` |
| AL-7 | **姿态轴协议扩展：`applyPose` + 裁剪带参数化 `band = none \| torso`** | `native/procedural/shape/` 的评估工具侧 |

> AL-7 因 **OQ-33 裁定"纳入"**而生效。OQ-30（A-Pose 具体角度值）属 OQ-01 子项，
> **尚未决定**；AL-7 只授权"机制"，**不预设角度值**。

## 3. 禁止（越界即视为违反授权）

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
| FB-12 | **宣称任何 Gate 为 PASS** |

## 4. 授权的终止与回滚

| # | 条款 |
|---|---|
| RB-1 | 全部 PoC-SM 代码位于独立 worktree 的 host-only 目录；**不触碰生产入口与 ABI** |
| RB-2 | 失败即丢弃该分支；`a32251b` 的 G2 证据保持不变 |
| RB-3 | 撤销授权即删除该 worktree 与分支；本仓库 `origin/feature/procedural-character-foundation` 不受影响 |

## 5. 执行记录（AI 实测，2026-11-24）

### 5.1 预检（五项硬性检查）

| # | 检查项 | 结果 | 实测证据 |
|---|---|---|---|
| C-1 | 目标路径不存在 | ✅ | `ls E:/AI-Tools/ProceduralAvatarEngine-poc-sm` → *No such file or directory* |
| C-2 | 不覆盖任何已有 worktree | ✅ | `git worktree list` 预检时仅 2 项，与目标路径无交集 |
| C-3 | 不覆盖用户文件 | ✅ | 同 C-1；目标路径及其父目录无既有文件 |
| C-4 | 目标磁盘空间充足 | ✅ | `df -h E:/` → `325G total, 185G used, 141G avail (57%)` |
| C-5 | 与原工作区隔离 | ✅ | 原工作区在 `C:/Users/Giant/Desktop/GITCODE/...`（C 盘）；本 worktree 在 E 盘，不同盘符、不同路径 |
| X-1 | 分支名未被占用 | ✅ | `git branch -a --list "*poc-sm*"` → 无输出 |
| X-2 | 构建产物不会被误提交 | ✅ | `.gitignore` 含 `build/`、`build-host/`、`*.apk`、`.cxx/`、`.gradle/`、`.idea/`、`local.properties` |
| X-3 | 基线 commit 可用 | ✅ | `git cat-file -t a32251b…` → `commit`；`git merge-base --is-ancestor a32251b HEAD` → 成功 |

### 5.2 实际执行

```bash
# 1) 以基线创建 worktree（detached，隔离分支）
git -C "E:/AI-Tools/ProceduralAvatarEngine-PoC" worktree add --detach \
    "E:/AI-Tools/ProceduralAvatarEngine-poc-sm" \
    a32251bf798e46159696a637a699a062453df345

# 2) 在新 worktree 内建分支
git -C "E:/AI-Tools/ProceduralAvatarEngine-poc-sm" switch -c poc-sm/shape-model-v2

# 3) 带入治理上下文（docs/ 树）
git -C "E:/AI-Tools/ProceduralAvatarEngine-poc-sm" checkout <GOV_COMMIT> -- docs/ .gitattributes
```

**执行结果**：

| 项 | 结果 |
|---|---|
| worktree 路径 | `E:/AI-Tools/ProceduralAvatarEngine-poc-sm` ✅ 已创建 |
| 分支 | `poc-sm/shape-model-v2` ✅ 已创建 |
| 起点 commit | `a32251bf798e46159696a637a699a062453df345` ✅ |
| 治理上下文带入 | commit `3b3532e`（`docs/` 树取自 `d085ff3`），已推送 `origin/poc-sm/shape-model-v2` ✅ |
| `git worktree list` | 现有 **3** 项（原工作区 / PoC-A 工作区 / poc-sm 工作区）✅ |
| 新 worktree `git status` | 干净 ✅ |
| 生产代码改动 | **无** —— 仅 `docs/` 与 `.gitattributes`；`native/` `tests/` `tools/` `evidence/` `app/` 均未触碰 ✅ |

> **分支结构**：
> `poc-sm/shape-model-v2` = `a32251b`（G2 证据冻结点）+ `3b3532e`（治理上下文带入）。
> 真正的 PoC-SM 实现提交将叠加在 `3b3532e` 之上。
> 回滚 = 删除该分支与 worktree，`a32251b` 的 G2 证据与
> `origin/feature/procedural-character-foundation` 均不受影响（RB-2 / RB-3）。

> **采用 `checkout -- docs/` 而非 `cherry-pick`**：cherry-pick 会在新分支产生多个与 PoC-SM 无关的提交，污染分支历史；
> `checkout -- docs/` 只取文件树。两种方式产出的 `docs/` 内容**完全等价**。

## 6. 授权后的预期产出（**产出 ≠ Gate 通过**）

| # | 产出 | 归属 Gate |
|---|---|---|
| 1 | `native/procedural/shape/v2/` 下的五层契约 host-only 实现 | — |
| 2 | Shared Skin Envelope 实现，产出一份连续外表面（无独立肩球 / 无圆管 / 无 append 拼接） | `G-P1-GEO` |
| 3 | Before/After 灰模对照（同相机/同光/同带/同取景） | `G-P1-GEO` |
| 4 | 肩根接缝复测（按 OQ-03 固定口径，≤5°） | `G-P1-GEO` |
| 5 | landmark / 7 视角轮廓 / 10 段截面偏差（阈值待 DEC-05 批准 → **当前 TBD，不可判 PASS**） | `G-P1-GEO` |
| 6 | 逐参数跨区域影响差分测试 | `G-P1-GEO` + `G-P1-TOPO` |
| 7 | 拓扑审计（components / 未登记 boundary / non-manifold / degenerate / sliver / 正交 quad 比 / 顶点 ID 稳定性） | `G-P1-TOPO` |
| 8 | 权重合法性 + CPU 变形 + 姿态变形对比（AL-7 已纳入） | `G-P1-ANIM` |
| 9 | 以新 Target Reference 重跑 G2 的证据 | G2 重开 |

`[VERIFIED]` **PoC-SM 自身的产出不等于任何 Gate 通过。** 它是"重跑 G2 的前置材料"，Gate 判定仍需负责人按 DEC-05 rubric 执行。

## 7. 关闭后的影响

| 后果 | 说明 |
|---|---|
| **OQ-02 关闭** | 从 `BLOCKING / OPEN` → `DONE`（基线 + 路径 + 范围 + 签署四要素齐备） |
| **OQ-33 关闭** | 从 `OPEN` → `DONE`（结论：纳入 AL-7） |
| **PoC-SM 编码解锁** | 本授权是**唯一**的编码授权来源 |
| **OQ-01 仍未关闭** | DEC-05 家族未签署 → **几何验收与视觉判定仍不可执行** |
| **不产生任何 Gate PASS** | FB-12 明确禁止 |
| **`G-P1-GEO` 仍 FAIL** | 契约与授权都不是实现结果 |

## 8. 边界声明

本授权**仅**放行"在独立 worktree 内实施 PoC-SM 形态 PoC"这一件事。它**不**放行：
生产入口修改、ABI 变更、legacy 路径替换、外部资产引入、JS 运行时引入、
任何 Gate 的 PASS 判定、任何 SPEC 阈值的臆造。
