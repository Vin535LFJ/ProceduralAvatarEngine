# PAE Procedural Human Generation — MASTER PLAN (M1–M8)

Status: `MASTER_PLAN = READY_FOR_REVIEW`
Date: 2026-11-28
Branch: `poc-sm/shape-model-v2` @ `d40e21316032e67322fabb1e91843f9fa9ba4df1`（本轮开始本地=远端 ls-remote 核实一致；任务书假设的 `bfcada7` 已因 WP4-HOST-RIG-01 入库前进到 `d40e213`）

**本文件是 M1–M8 唯一权威总计划。** 取代 `PAE-PROCEDURAL-HUMAN-GENERATION-ROADMAP.md`（已降为历史指针）与 R1/R2 时期 `proposals/.../07-poc-roadmap.md`。所有分阶段计划（`docs/phases/VR-1B/01..09`）、架构契约（`docs/architecture/human-character-instance/`、`human-rig-v2/`）与状态板（`PAE-HUMAN-GENERATION-STATUS.md`）均引用本文件，不得另立技术权威。

最终目标：**用 PAE 自己的程序化系统，按人体语义参数自动生成完整自然半写实成人，自动构建/控制骨骼、权重与动作，经 Android NDK + OpenGL ES 3.0 实时渲染。** 运行时不依赖 Blender/MakeHuman/MPFB2/在线 AI/导入 FBX 动画。允许许可证已审核的成熟 Base Topology + Shape Targets 离线编译为 PAE 模板（ADR-014）。

---

## 1. 最终完整系统是什么

一条数据流（权威定义见 `human-character-instance/01-character-instance-contract.md`）：

```
HumanTemplateIdentity (PAHT v3, CC0 编译)
   + ShapeParameterVector / 人体测量目标
        → [M1 Shape Solver] → ResolvedShapeState (网格+实测+质量)
        → [M2 JointResolver] → ResolvedJointSet
        + HumanRigDefinition (PAE 语义骨骼)
        → [M2 WeightGenerator] → SkinWeightSet (Top-4)
        → [M3 CharacterInstanceBuilder] → CharacterInstance
        + PoseState
        → [M2/M3 CPU LBS] → DeformedCanonicalMesh
        → RenderMesh → [M4 Android GLES3 动态路径]
        → [M5 程序化动画/IK] 驱动 PoseState
        → [M6 解剖细化] / [M7 材质头发服装] → [M8 最终验收]
```

## 2. 当前已完成什么（真实入库状态）

| 阶段 | 状态 | 提交/证据 |
|---|---|---|
| M0 模板基础 | **ACCEPTED** | WP1/WP2 `7ede072`、WP2.1 `b084102`；PAHT v3 sha `d3bcbbe…417b`；真机静态 13,380 顶点/26,756 三角形 53–56 fps |
| WP3-HOST-01 + R1（M1 测量基础） | **IMPLEMENTED** | `091a414/d7e83ee/56c5f9f/5540da1/bfcada7`；五类参数响应、身高锚点、联动语义、截面归属、9 体型证据 |
| M2 WP4-HOST-RIG-01 | **IMPLEMENTED / POSE_QUALITY PARTIAL** | `3c0ad02/340db63/d40e213`；Rig V2 92/20、CC0 关节解析、Distance/Heat/Region 权重、CPU LBS、22 姿态、184 图 |
| M1 WP3-R2 求解器 | **IMPLEMENTED** | `wp3_r2.h/.cpp` + `wp3_r2_main.cpp`；forward model + 有界 LM 逆求解；known/holdout/unreachable/非加性/确定性全过；VR-1B/01a 报告 + evidence |
| M3–M8 | NOT_STARTED / PLANNED / NOT_AUTHORIZED | 见状态板 |

## 3. 每个模块缺少什么（详见各分阶段计划）

- M1：逆求解器（查找表/分段曲线/有界非线性最小二乘 LM/TR、数值 Jacobian、可辨识性、非加性、不可达处理）、语义测量统一、Parameter Profile。
- M2：极端姿态变形质量（握拳/举臂/深蹲翻面）——需 DQS/corrective 或 M6 几何细化（本轮 LBS 已证不足）；JointResolver 须区分表面 landmark 与关节中心（肩峰≠肩关节中心）。
- M3：CharacterInstanceBuilder + 缓存失效 + 权重复用判定规则。
- M4：动态 palette/顶点布局/设备能力实测/CPU-GPU parity（独立路径，冻结旧路径）。
- M5：Pose Controller/双骨 IK/程序化运动/运动质量。
- M6：分区域解剖细化（Bind + Pose 双态验收）。
- M7：Procedural Material Baker + 皮肤/头发/服装（Clay/Material/Lighting 分别验收）。
- M8：固定版本验收矩阵 + 负责人人眼最终签署。

## 4. 模块如何连接

唯一接口契约：`human-character-instance/01-character-instance-contract.md`。所有数据携带 Template Version / Base Mesh SHA / Shape Target Hash / Rig Version / Weight Version / Parameter Profile Hash。**Shape 变化→关节必重解析；权重是否复用由已验证规则决定，不得因拓扑相同默认复用；不同 Rig/Weight 版本不得共享缓存。**

## 5. 哪些可并行

- **M1 与 M2 Host 并行**（共享契约，各自白名单）。
- M6 解剖细化与 M7 材质在几何/变形稳定后可部分并行（不同子系统）。
- M5 动画依赖 M4 动态运行时稳定，不可与 M4 并行启动。

## 6. 哪些必须串行

- M3 必须在 M1+M2 Host 通过后汇合。
- M4 必须在 M3 Host 通过后（且单独授权）。
- M5 在 M4 稳定后。M8 在全部之后。
- M2 极端姿态质量改进是 M3/M6 的前置（变形不自然则整合无意义）。

## 7. 每阶段可见交付

| 阶段 | 可见交付 |
|---|---|
| M1 | 输入尺寸→输出体型网格 + 实测表 + Profile 文件 + 求解误差报告 |
| M2 | 同一人体多姿态变形图 + 逐关节质量表（已有，待质量改进） |
| M3 | CharacterInstance：换体型后关节/权重同步的多体型同动作对比 |
| M4 | Android 真机动态人体（抬臂/屈膝）+ CPU/GPU parity 日志 |
| M5 | 程序化 idle/walk/reach/squat 可重复输入输出证据 |
| M6 | Bind+Pose 双态解剖细化前后对比 |
| M7 | Clay/Material/Final Lighting 三档渲染 |
| M8 | 多体型×多动作×多设备验收矩阵 + 人眼签署 |

## 8. 每阶段技术 Gate（独立，不得互相顶替）

- M1：known-solution recovery / holdout / unreachable / 非加性 / 确定性 / 网格质量 / Profile 对比。
- M2：Bind 精确恢复 / Top-4 合法 / 逐关节区域指标 / 多视角图（Host≠Android）。
- M3：换形态后关节权重不停留旧人体 / ≥3 体型同动作 / 缓存失效正确。
- M4：静动态一致 / CPU-GPU parity / 设备能力 / Surface recreate / APK 独立 / 内存帧性能。
- M5：关节限位 / 足滑 / 接触 / 平衡 / 根运动 / 插值。
- M6：Bind+Pose 双态人眼验收。
- M7：Clay/Material/Lighting 分别验收。
- M8：技术指标通过 ≠ 视觉通过；负责人人眼最终签署。

## 9. 每阶段授权边界

- M1/M2 Host：各自既有白名单（`tools/human-template-poc/host/**` 及对应 docs）。M1 代码授权来自 WP3-HOST 系列；M2 来自 WP4-HOST-RIG-01（`01-wp4-host-rig-plan.md`）。**不得超出原白名单。**
- M3 Host：需新授权（整合模块）。
- M4 Android：**NOT_AUTHORIZED**，M3 通过后单独授权白名单；冻结 LegacyRigV1/UBO[22]/64B/JNI/mode-3/mode-4 默认行为；PAHT v3 兼容（新格式走版本升级+读旧，不静默改 v3 语义）。
- M5/M6/M7/M8：各自后续授权。

## 10. 可能导致路线调整的问题

1. 极端姿态变形（握拳/举臂/深蹲）纯 LBS 失败 → 需 DQS/corrective 或 M6 手指几何细化；若基础网格环线不足成为硬限制，M6 优先级前移。
2. 组合形态非加性 → M1 必须迭代/查表，线性叠加不可用。
3. PENDING 许可证（mhskel/mhw）若无法确认 → PAE 自有 Rig/权重是唯一分发路径（WP4 已如此）。
4. 目标设备 GLES3 UBO/palette 容量不足 → M4 需 CPU 回退或骨骼 LOD。
5. 退化判定绝对阈值（native）尺度相关 → 需白名单授权修改。
6. 肩峰表面 landmark ≠ 肩关节中心 → JointResolver 需解剖偏移模型（M2-B）。

## 11. 统一开发制度

每个工作包字段：Goal / Scope / Dependencies / Allowed Paths / Forbidden Paths / Input Contract / Output Contract / Tests / Evidence / Acceptance / Stop Conditions / Owner Sign-off。状态板 `PAE-HUMAN-GENERATION-STATUS.md` 记录 NOT STARTED / DESIGN / IMPLEMENTED / TECHNICAL PARTIAL / VISUAL BLOCKED / ACCEPTED。每包须有：实际代码 / 自动测试 / 可观察证据 / 质量性能数据 / Git Diff / 未解决问题。**禁止只提交空架构接口或纯文档而无对应能力。**

## 12. 治理原则

单一权威工作区 `E:\AI-Tools\ProceduralAvatarEngine-poc-sm`；单一主分支 `poc-sm/shape-model-v2`；并行 Agent 仅在隔离 Host 范围；生产写入单一权威写入者，其他只读；不覆盖未提交文件；不自动 commit/push；不 force push；不删历史证据；不改 LegacyRigV1；不未授权改 Android；不用未审核外部资产；Host 通过≠Android 通过；静态完整人体≠最终半写实人体。

## 13. 实施批次

1. 第一批（本轮）：总计划 + 接口契约 + M1/M2 设计 + 各阶段计划（13 文档）。
2. 第二批：M1 求解器 Host 实现 ∥ M2 变形质量改进（各自白名单）。
3. 第三批：M3 整合（需授权）。
4. 第四批：M4 Android 动态（需授权）。
5. 第五批：M5 动画/IK。
6. 第六批：M6 解剖 ∥ M7 表现。
7. 最终：M8 验收。

每阶段负责人审查 Gate，但不重复研究总路线。

## 14. 文档索引（相互引用，单一权威）

- 总计划：本文件。
- 状态板：`docs/roadmap/PAE-HUMAN-GENERATION-STATUS.md`。
- 接口契约：`docs/architecture/human-character-instance/01-character-instance-contract.md`。
- Rig V2 架构：`docs/architecture/human-rig-v2/01-rig-v2-architecture.md`。
- 分阶段：`docs/phases/VR-1B/01-wp3-r2-implementation-plan.md` … `09-m8-production-acceptance-plan.md`。
- 既有证据：`docs/phases/P1-visual-recovery/16/17/18/19` 号报告、`wp4-host-rig-01/02` 号报告及 evidence。