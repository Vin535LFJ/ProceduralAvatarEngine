# 03 — G3 DEFER 与下一阶段推进方案

> 状态：**PROPOSAL / 待负责人签署**。本文件不授权任何新代码实施。
> 上游设计：`docs/proposals/anatomy-driven-geometry-redesign/09-final-recommendation.md`（仍为 PROPOSAL，未批准）。
> 本阶段证据：`a32251b`（G0/G1/G2 代码与证据），已推送 `origin/feature/procedural-character-foundation`。

## 0. 一句话结论

**停止加 R2/R3 的代码量。** G3 记为 `DEFER`；先把负责人认可的视觉标准立起来，再做一次以 09 文档为设计依据的 **Shape Model v2 形态 PoC**；R2 固定拓扑投影作为后端候选保留，不参与本轮形态竞争。

---

## 1. 与 09 — Final Recommendation 的关系（继承，不另起炉灶）

09 文档已经给出正确架构，本方案只做两件事：**用 G2 实测回填它的未验证项**，以及**补齐它的授权缺口**。

| 09 文档原判断 | G2 实测回填 | 结论 |
|---|---|---|
| "M12 连续性有了，但 shape model 仍不足" | R3 包络在 y≈1.08 处只到 \|x\|=0.1442 m，参考面 0.2792 m（上胸缺口 13.5 cm）；E3 32.11 mm；有效参数仅 7/20 | **实证成立**。提高模板分辨率不收敛（粗→生产 max 仅 −2%） |
| "肩 bridge 只是连接带，不是肩带/腋窝共同形态模型" | 肩根 crop 环最大二面角 R1/R2 47.66°、R3 16.13°，均超 ≤5° 合同值 | **实证成立**。接缝失败是上层形态问题，不是模板密度问题 |
| "Shared skin envelope + quad template 是强推荐候选，尚未通过新 PoC 实测" | R2 固定拓扑投影首次实测：E2 0.98 mm、0 sliver、stable ID 跨 3 参数组恒定 | **后端候选得到支持**；但 **shared envelope 的新形态仍未被验证** |
| "没有负责人认可的目标形态，AI 只能做数值对照" | G2 全部 §13 阈值除 ≤5° 外仍为 TBD；63 张灰模图无人签署 | **仍然成立**，且是当前最大的阻塞项 |
| "新代码/正式实施：未开始；等待负责人明确是否授权新的 ADR/PoC" | AUTH-07 只覆盖 R2/R3 最小实现，不覆盖新 Shape Model | **授权缺口仍然存在**，本方案显式请求 AUTH-08 |

### 命名冲突必须先解决

同一个 `PoC-A` 现在指两件不同的事：

- **09 文档的 PoC-A** = 第一个*形态* PoC（完整胸腹腰盆 + 双侧肩连接 landmarks）
- **SPEC v2.1 的 PoC-A** = 刚完成的*路线公平比较*（G1/G2，R1/R2/R3）

后续文档统一改用下列名称，避免混淆：

| 新名称 | 内容 | 状态 |
|---|---|---|
| **PoC-R** | 路线公平比较（原 SPEC PoC-A / 本次 G1+G2） | 已完成，`a32251b` |
| **PoC-SM** | Shape Model v2 形态 PoC（原 09 的 PoC-A） | 本方案申请授权 |
| **PoC-SHOULDER** | 肩带、三角肌、腋窝凹面（09 的 PoC-B；SPEC T3） | 未开始 |
| **PoC-LIMB** | 上臂、肘、前臂与动作（09 的 PoC-C；SPEC T4/T5） | 未开始 |
| **PoC-FULL** | 上半身整合 / 全身（09 的 PoC-D） | 未开始 |

---

## 2. G3 决策记录变更（按 SPEC §7.2 格式）

| 字段 | 值 |
|---|---|
| Gate | GATE-04 / G3 Owner Investment Decision |
| 原状态 | `OPEN / NOT SIGNED` |
| **建议状态** | **`DEFER`** |
| DEFER 的含义（SPEC §7.2） | 补指定证据后**重开 G2**，不得默认为通过 |
| `approved route` | **空**（不选定最终生产路线） |
| 路线处置 | R1 = 参考基准，保留；R2 = 固定拓扑投影候选，保留但暂停加码；R3 = 暂停投入 |
| 理由 | ① §13 阈值未批（AI 不得判 PASS）；② owner 视觉 Gate 未行使；③ ≤5° 接缝合同失败未解决；④ 共同上游 Shape Model 形态不足，此时选路线等于锁定一个不够好的目标 |
| 补证要求 | 负责人批准 DEC-05 参考图/rubric；PoC-SM 的 Before/After 灰模对照；以新 Target Reference 重跑的 G2；≤5° 接缝复测 |
| 签字 | **待负责人**。AI 与审核人均不得代签 |

> 注意：G3 的 `DEFER` 不是"失败"，也不是"通过"。它明确要求补证后**回到 G2 重测**，而不是原地继续加代码。

---

## 3. 五步推进方案（把你的建议变成可执行的 Gate）

### 阶段 0 — 冻结与授权（现在，无需新授权）

- [x] G2 证据冻结在 `a32251b` 并推送远端
- [x] 独立审核提示词已生成：`docs/phases/anatomy-driven-geometry-poc/02-route-review-prompt.md`
- [x] 负责人 2026-11-23「同意，采纳你全部建议」→ G3 记 `DEFER`、`approved route` 保持空
- [x] **DEC-09 = APPROVED**：采纳 09 文档的分层 Shape Model 契约（已写入 SPEC §7.1）
- [x] 09 文档状态更新为 `ADOPTED IN PRINCIPLE`，并写入 G2 证据回填
- [x] 视觉评价协议草案：`04-visual-rubric.md`（AI 只定协议，参考图/判定留负责人）
- [x] ADR-011（Shape Model 契约）、ADR-012（Saltline 研究）草案已起草
- [ ] **AUTH-08 仍为 OPEN**：负责人需给出基线 commit 与 worktree 路径；在此之前 AI 不实施 PoC-SM
- [ ] DEC-05 参考图 / rubric 仍待负责人提供（没有它不得宣布视觉合格、不得重跑 G2）
- [ ] 独立审核人意见回收（提示词已生成，审核尚未执行）

### 阶段 1 — 视觉标准（前置：负责人，不是 AI）

这是 SPEC **DEC-05** 的落地，当前仍是 OPEN。

- **负责人提供/批准**目标参考图与 rubric：正面、侧面、45°、背面；重点评价胸廓、腰臀过渡、肩膀、腋窝。
- AI 只负责**评价协议**，不定义"什么是好看"：
  - 复用现有 `viewFrame()` + `ClayRenderer`（同相机、同光、同 band、同视图内取景）；
  - 固定灰模材质、背景、分辨率、色彩管理；
  - 每个视角给出可复算的轮廓/比例采样点，供 owner 逐项打勾。
- 输出：`visual-rubric.md`（负责人签字栏为空）+ 参考图清单。
- **没有 DEC-05 批准，不得宣布任何视觉通过**，也不得进入阶段 3 的 G2 重测。

### 阶段 2 — PoC-SM（需 AUTH-08）

目标不是"更像人体"这种口号，而是**先解决整体体积关系**：

1. 按 09 文档实现 `CharacterDefinition` 的五个子契约：`ProportionProfile` / `AnatomicalLandmarks` / `AnatomicalRegions` / `ShapeParameters` / `JointDeformationRegions`。
2. 实现 **Shared Skin Envelope**：胸腔 B-spline guide + 腹盆 implicit volume field + 肩带 shared field + 凹面约束 → **一份最终外表面**。区域保留 RegionId 用于权重/拓扑，不各自输出可见 mesh。
3. 不用独立肩球、不用圆管、不用 mesh append 冒充连续曲面（SPEC §14.1）。
4. **Before/After 用同一灰模协议**，Baseline = 当前 SDF Shape Model，After = PoC-SM。
5. 验收条件包含：≤5° 肩根接缝复测、15 个 landmark、7 视角轮廓、参数有效集不再出现 R3 那种"7/20 且是子集"的塌陷。

**必须避开的测量陷阱**：Before/After 不能继续拿旧 SDF 当 Target Reference，否则只测到"新模型离旧模型多远"。PoC-SM 需产生自己的、由 DEC-05 背书的参考形态，再进入阶段 3。

### 阶段 3 — 以新目标重跑 G2，G3 重新开放

- R2 后端候选继续保留：它的 E2 0.98 mm、0 sliver、stable ID 结论**与具体的 Shape Model 无关**，可继承。
- **作废**的数字：所有 E1/E3、landmark、截面、7 视角偏差（因为 Target Reference 变了）。
- **保留**的方法：E1/E2/E3 分离、Layer A/B、`TopologyAudit` 的 crop/裂缝区分、剪影协议。
- G3 从 `DEFER` 回到 `OPEN`，由负责人签署唯一 `approved route`。

### 阶段 4 — 视觉 Gate 通过后的扩展顺序

`PoC-SHOULDER`（肩带/腋窝，SPEC T3）→ `PoC-LIMB`（手臂/肘，T4/T5）→ `PoC-FULL`（全身 + Humanoid Rig + 换装）。任一关 owner 否决即 STOP。

---

## 4. Saltline 模块化（并行研究，不进生产）

按你的建议先研究，不引入 Three.js 或网页运行时依赖，也不把当前引擎改成 Web 栈。

- 只抽取**真正会复用**的原语：分层 SDF / 包络构造、参数化截面与 guide surface、区域权重场、资源构建管线。
- 先写研究 ADR：候选方案、接口边界、C++17/NDK/GLES3 可行性、与现有 `MeshData`/`Skeleton` ABI 的兼容方式。
- **硬边界**：SPEC §4 明确禁止建设"迷你 Blender"。不实现通用 quad remeshing（§14 延期项）、不做通用 SDF sculpting 工具。
- 研究阶段不修改生产源码；若决定纳入，另开 ADR + 授权。

---

## 5. AUTH-08 授权请求（待负责人填写状态）

| 项 | 内容 |
|---|---|
| ID | AUTH-08 |
| 操作 | PoC-SM（Shape Model v2 形态 PoC）实现与对照实验 |
| 范围 | 新建 `native/procedural/shape/v2/` 下的 host-only 契约与 envelope；新增 host 测试与 Before/After 灰模探针；从 `a32251b` 另开 worktree 分支 |
| 禁止 | 不改 BoneId 0–21 / UBO[22] ABI；不改 `RenderThread` 生产入口；不替换 legacy 三路径；不碰原工作区 L1；不引入 Three.js / JS 运行时 |
| 生成物 | `CharacterDefinition` 契约、Shared Skin Envelope 参考面、Before/After 报告与灰模图、ADR-011 草案 |
| 前置 | DEC-05 参考图/rubric 批准 + DEC-09 采纳 09 分层契约 + G3 `DEFER` 签署 |
| 修改工作区 | 仅新 worktree（L3/L4） |
| 风险 | 中（形态语义契约设计错误会导致重做；用 DEC-05 + 阶段 1 rubric 控制） |
| 状态 | **OPEN — 未授权，AI 不得开始实施** |

---

## 6. 决策记录与剩余输入

**2026-11-23 负责人已确认**（"同意，采纳你全部建议"）：

1. ✅ G3 = `DEFER`；R1 保留参考 / R2 保留候选并暂停加码 / R3 暂停投入。
2. ✅ 采纳 09 文档的 `CharacterDefinition` 分层契约 → **DEC-09 = APPROVED**。
3. ✅ ≤5° 接缝失败列为 PoC-SM **阻塞验收条件**（非可接受风险）。
4. ✅ Saltline 方法研究**单独立 ADR**（ADR-012 草案），不并入 ADR-011，不引入 Three.js。

**仍需负责人给出具体值（AI 不能代填）**：

5. ⬜ DEC-05 的**授权参考图 + rubric**来源。没有它，阶段 1 与阶段 3 均无法完成；`04-visual-rubric.md` 的判定栏保持空。
6. ⬜ AUTH-08 的**基线 commit** 与 **worktree 路径**。在此之前 PoC-SM 代码实施不启动。
7. ⬜ ADR-011 / ADR-012 的签署（当前均为 DRAFT）。

---

## 7. 一句话给负责人

> 现在选路线，等于在一个尚未被认可的 Shape Model 上锁定后端；建议 G3 记 DEFER，先用你认可的参考图把视觉标准立住，再用一次 Shape Model 形态 PoC 把胸腹腰盆和肩带体积关系做对，然后让 R2 在这个新目标上重新证明自己。
