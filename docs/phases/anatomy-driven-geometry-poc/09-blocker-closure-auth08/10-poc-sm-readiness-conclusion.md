# 10 — PoC-SM 启动条件结论

> 状态：**结论 / CONCLUSION**。本文件给出明确判断，不代替负责人签署任何决策。

---

## 1. 一句话结论

**PoC-SM 可以开始，但差两份签字。** OQ-02（AUTH-08 授权）与 OQ-04（ADR-011 / ADR-012 签署）的材料已在本轮备齐，负责人签署后即可启动编码；**PoC-SM 的几何验收、G2 重测、G3 重新批准、G4 均尚不可开始**。

---

## 2. 明确结论表

| 阶段 | 是否具备启动条件 | 依据 | 仍缺什么 |
|---|---|---|---|
| **PoC-SM 编码** | **条件具备，差 2 份签字** | 本轮 06（ADR 签署稿）、本轮 08（AUTH-08 授权书 + 路径预检通过） | ① OQ-04 签署 ② OQ-02 签署（含 OQ-33 裁定） |
| **PoC-SM 几何验收**（`G-P1-GEO`） | **不具备** | `G-P1-GEO` 当前 **FAIL**（R1/R2 接缝 47.66°、R3 16.13° vs ≤5°）；无 Canonical Acceptance Sheet → 无验收目标 | ① OQ-01 家族关闭（OQ-28/31 至少）② OQ-03 性质确认 ③ PoC-SM 达成 ≤5° ④ OQ-14/16/17/21 复测 |
| **PoC-SM 拓扑验收**（`G-P1-TOPO`） | **部分具备** | R2 已满足全部合同项（sliver 0 / 未登记 boundary 0 / non-manifold 0 / degenerate 0 / components 1 / 正交 quad 比 0.935 / ID 恒定） | OQ-09 审核人复核；OQ-17 阈值 |
| **PoC-SM 动画验收**（`G-P1-ANIM`） | **部分具备** | 权重合法 ✔；CPU 刚体 22/22 零误差 ✔ | OQ-30/32（姿态定义）、OQ-33（协议授权）、OQ-19（GPU parity） |
| **G2 重测** | **不具备** | 四项补证全未达成（本轮 07 §2.2） | OQ-18（体积口径先统一）、OQ-03/05 签署、OQ-01/28 |
| **G3 重新批准** | **不具备** | `G-P1-GEO` = FAIL；`approved route` 保持空 | G2 重测完成 + 负责人重新决策 |
| **G4** | **不具备** | SPEC 要求 G3 已签署**且**视觉 Gate 通过；`G-FINAL-VIS` = NOT RUN | OQ-19、OQ-20、OQ-35 |

---

## 3. 阻塞项的三类处置（本轮实际结果）

### 3.1 真正关闭的：**0 项**

`[VERIFIED]` **本轮没有关闭任何一个阻塞项。** 关闭的定义是"负责人已签字/已给出值"，而本轮 AI 未获任何签字。

`[INFERENCE]` 这是本轮唯一正确的表述方式。**不能**用"材料已备齐"或"文档已完成"冒充"阻塞已关闭"。

### 3.2 只关闭了"输入准备"部分的：6 项

| OQ | 原状态 | 本轮进展 | 仍缺 |
|---|---|---|---|
| **OQ-01** | BLOCKING / OPEN，**无任何材料** | 参考矩阵更新至 20 条来源（含 4 条本轮实抓正文）；Canonical Acceptance Sheet v0 候选产出；五 Gate Matrix 抽出；rubric 修订提案完成 | **负责人批准 DEC-05**（含 5 个子项） |
| OQ-28 | 不存在 | Canonical Sheet §2 给出 A/B/C 三条路径与 3 个体型原型候选，全部带一手来源 | 路径 + 原型 + 风格化系数 |
| OQ-29 | 不存在 | Layer B 已核验来源从 2 条增至 **6 条**（T1×1、T2×2、T3×1 + 原 2 条）；缺口从 4 组降至 **3 组** | 3 组拓扑参考的原始链接 |
| OQ-30 | 不存在 | A-Pose 区间线索（45°–75°）给出，来源与置信等级已标注 | 角度值 |
| OQ-31 | 不存在 | 两个候选（解剖轴 / 固定世界 Y）列出 | 选定 |
| OQ-32 | 不存在 | 建议动作姿态集列出 | 确认 |
| **OQ-02** | BLOCKING / OPEN，**无路径候选** | 基线已核实（含"不丢代码但丢治理上下文"的关键发现）；**3 条路径候选全部通过 5 项预检**；授权书草案完成 | 负责人指定路径 + 签署 |
| OQ-33 | 不存在 | 方案已给出（纳入 AL-7 / 排除） | 裁定 |
| **OQ-03** | BLOCKING / OPEN | **五条决策文本 + 接缝测量口径固定表**已给出 | 签字 |
| **OQ-04** | BLOCKING / DRAFT | 两份 ADR 的签署稿完成，范围与冻结边界逐条列明 | 签字 |
| **OQ-05** | BLOCKING / DRAFT | DEFER 决策文本 + 四项补证清单 + 完整 G3 记录稿完成 | 签字 |

### 3.3 完全未推进的：8 项

| OQ | 原因 |
|---|---|
| OQ-06…OQ-13（8 项） | 需**审核人**回答，AI 不能代答 |
| OQ-19 | 需设备与 GPU 专项 |
| OQ-20 | 需真机复验 |
| OQ-27 | 待负责人决定是否写入 SPEC |
| OQ-34 | 待负责人决定是否成文 |
| OQ-35 | 待负责人给出材质/光照基线与性能预算 |

---

## 4. 本轮**确实**产生的实质技术进展（可核验、非文档性）

`[VERIFIED]` 以下不是"文档完成"，而是**可复核的外部事实**：

| # | 进展 | 证据 |
|---|---|---|
| T-1 | **取得 ANSUR II / NC State 一手人体测量汇总表**（36 项，男性 5/50/95 百分位 + SD），原件已留存并附 SHA256 | `source/ANSUR-II-Anthropometry-Summary-Tables.pdf`（542,674 B，SHA256 `a1ea470e…`） |
| T-2 | **取得 CDC NHANES SR50 一手报告**，含腰围**测量位置原文**："Waist circumference was measured with a tape measure at the uppermost lateral border of the hip crest (ilium)."，及男性 20+ / 20–29 腰围与臀围全套百分位 | `source/CDC-NHANES-SR50-Anthropometric-Reference-Data.pdf`（1,812,869 B，SHA256 `ccb8d52b…`） |
| T-3 | **核验并留存的 4 条 Layer B 来源正文**：Thunder Cloud Studio（ArtStation，2024-10-18，授权未标注）、Blender Studio（Julien Kaspar 署名）、Renderosity 腋窝帖（pjz99，21 posts，2008）、BodyParts3D（**CC BY 4.0**，许可章节原文已取得） | 本轮 02 §3.1 |
| T-4 | **建立 4 级权威阶梯**，禁止把论坛帖 / 教学文 / 解剖库并列 | 本轮 02 §1 |
| T-5 | **发现并解决"两套并行 Register"**：OQ-27 在正式 Register 中空缺；草稿 OQ-28/29/33 与正式 OQ-01/02 重复 → 合并后独立 BLOCKING 主项回到 **5 个**（原最多 14） | 本轮 04 |
| T-6 | **发现基线治理上下文缺口**：`a32251b` 与 HEAD 之间 4 个提交全是 `docs/`，从该基线建 worktree 会丢失 G3 DEFER / DEC-09 / AUTH-08 状态 → 给出 `checkout 7c451ae -- docs/` 的带入方案 | 本轮 01 §5.5、本轮 08 §4 |
| T-7 | **固定肩根接缝的测量口径**（对象/角度域/法线朝向/纳入边界/统计量/不适用处理），使 ≤5° 合同值首次具备可复核定义 | 本轮 07 §1.3 |
| T-8 | **核验 G2 基线与当前 HEAD 的差异面**：代码/测试/证据**零丢失** | 本轮 01 §5.5 |
| T-9 | **确认 David Rig 不可核验**（两次尝试失败），维持 `UNVERIFIED`，未按名称推断 | 本轮 02 §2.1 |
| T-10 | **确认 Layer B 的胸廓宽度 / 腰宽 / 严格盆深在两份权威来源中均缺失**，明确标为 `TBD` 并给出候选来源，未以模型产出冒充人体参考 | 本轮 03 §8 |

---

## 5. 明确的否定声明

`[VERIFIED]` 以下**不成立**，本轮不主张：

| # | 否定声明 |
|---|---|
| N-1 | **"文档完成"不等于"阻塞关闭"**。本轮 10 份文件全部是草案；阻塞项关闭数为 **0** |
| N-2 | **"AI 声称完成"不等于批准**。本轮没有任何一项被 AI 判定为已批准 |
| N-3 | **"Git 工作区干净"不等于验收通过**。`git status` 为空只说明无未提交改动；`G-P1-GEO` 仍为 FAIL |
| N-4 | **"Canonical Sheet 有数值"不等于"数值已获批"**。状态为 `CANDIDATE`，签字栏为空 |
| N-5 | **"五 Gate Matrix 已建立"不等于任何 Gate 已 PASS**。`G-P1-GEO` = FAIL，`G-P1-ANIM` = PARTIAL，`G-P2-MOD` / `G-FINAL-VIS` = NOT RUN |
| N-6 | **"参考矩阵已核验 20 条"不等于 Layer A 已就绪**。20 条中**无一条**是 Layer A Canonical Target |
| N-7 | **"ADR 签署稿已备好"不等于 ADR 已签署**。两份仍为 `DRAFT` |
| N-8 | **"AUTH-08 预检通过"不等于授权已发出**。未执行 `git worktree add`，未创建分支 |

---

## 6. 建议的下一步顺序（最小动作）

`[RECOMMENDATION]` 只需 **2 份签字 + 1 个裁定**，即可把项目从"阻塞"推进到"编码中"：

### 第 1 步：一次性签署 4 份文件（全部已备好）

| 签署对象 | 文件 | 效果 |
|---|---|---|
| ADR-011 | 本轮 06-A | 关闭 OQ-04 的一半；解锁 PoC-SM 契约 |
| ADR-012 | 本轮 06-B | 关闭 OQ-04 的另一半；**明确非 PoC-SM 前置** |
| G3 DEFER 记录 | 本轮 07 §3 | 关闭 OQ-05；G3 正式记为 DEFER，`approved route` 留空 |
| ≤5° 性质决策 | 本轮 07 §1 | 关闭 OQ-03；**解锁 PoC-SM 编码**（不再是启动阻塞） |

### 第 2 步：签署 AUTH-08（本轮 08）

需填 4 项：基线 commit（建议 `a32251bf798e46159696a637a699a062453df345`）、worktree 路径（建议**候选 1**：`E:/AI-Tools/ProceduralAvatarEngine-poc-sm`）、分支名（建议 `poc-sm/shape-model-v2`）、OQ-33 裁定（建议**纳入** AL-7）。

### 第 3 步（与第 1–2 步并行，不阻塞编码）

DEC-05 批准路径（OQ-01 家族）：Canonical Sheet 路径 A/B/C + 体型原型 + 风格化系数 + 姿态角度 + 相机高度绑定 + 必检动作集。
**这一步不阻塞编码，只阻塞几何验收。**

### 第 4 步：执行（负责人签署后才做）

按本轮 08 §4 的三条命令创建 worktree 并带入 `docs/` 治理上下文，然后开始 PoC-SM 实现。

---

## 7. 本轮执行边界（逐条声明）

`[VERIFIED]` 本轮**未**执行任何一项被禁止的操作：

| 禁止项 | 状态 |
|---|---|
| 修改 C++ 源码 | **未执行** |
| 创建 Worktree | **未执行**（仅只读预检） |
| 创建分支 | **未执行**（仅 `git branch --list` 只读查询） |
| 修改生产工程 | **未执行** |
| 修改骨骼 ABI | **未执行** |
| 修改 JNI | **未执行** |
| 编译或部署新版本 | **未执行** |
| Commit / Push | **未执行** |
| 自行签署 ADR | **未执行**（两份仍为 DRAFT，签字栏为空） |
| 自行签署 G3 | **未执行**（状态仍为 DRAFT） |
| 自行批准 DEC-05 | **未执行**（状态仍为 OPEN，签字栏为空） |
| 修改项目文档 | **未执行**（本轮 04 的 Register 替换稿是待批草案，未写回仓库） |
| 触碰原工作区 | **未执行**（`C:/Users/Giant/Desktop/GITCODE/Procedural Avatar Engine` 未被读写） |

`[VERIFIED]` 仓库 `E:/AI-Tools/ProceduralAvatarEngine-PoC` 在本轮结束时：`git status --short` 为空，HEAD = `7c451ae18137316291a4a51ecd03e57d2bea2d0f`，`git worktree list` 仍为 2 项，远端 feature 分支仍 = `7c451ae`。

`[VERIFIED]` 本轮全部新增文件位于 `E:/AI-Tools/DEC-05-Review-Drafts/blocker-closure/`（仓库外），共 10 份 Markdown + 2 份来源原件。
