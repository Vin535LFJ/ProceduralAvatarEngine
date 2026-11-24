# OQ-04 决策记录 — ADR-011 / ADR-012 签署

> **状态：已决策 / DECIDED**（2026-11-24）。
> 关闭 OQ-04：`BLOCKING / DRAFT` → **`DONE`**。
> 来源：`09-blocker-closure-auth08/06-adr-011-012-signoff.md`。

## 决策记录（DECISION RECORD）

| 字段 | 值 |
|---|---|
| 决策项 | OQ-04 — ADR-011 / ADR-012 是否签署？ |
| 决策结果 | **ADR-011 = ACCEPTED；ADR-012 = ACCEPTED — RESEARCH ONLY** |
| 决策人 | 项目负责人 |
| 决策日期 | **2026-11-24** |
| 决策来源 | 负责人在 WorkBuddy 会话中明确选择「签 4 份 + 下发 AUTH-08」执行包 |
| 记录方式 | AI **仅作转录与归档**；未代签，未添加负责人未表达的判断 |

## 1. 签署结果

| ADR | 主题 | 结果 | 正式文件 |
|---|---|---|---|
| **ADR-011** | CharacterDefinition 分层 Shape Model 契约 | **`ACCEPTED`** | `docs/architecture-decisions/ADR-011-shape-model-contract.md` |
| **ADR-012** | Saltline 程序化资源生成方法：独立技术模块研究 | **`ACCEPTED — RESEARCH ONLY`** | `docs/architecture-decisions/ADR-012-saltline-module-study.md` |

两份 ADR 的完整正文、批准范围、冻结边界与决策记录**写在 ADR 文件本身**；本文件只做归档索引与影响说明。

## 2. 本次批准的范围（摘要）

### ADR-011 批准的是

- `CharacterDefinition` 五层子契约的**存在与语义**：`ProportionProfile` / `AnatomicalLandmarks` /
  `AnatomicalRegions` / `ShapeParameters` / `JointDeformationRegions`。
- 冻结边界 F-1..F-6：BoneId 0–21、UBO[22]、64B GPU packing、JNI ABI 不变；
  不用独立肩球 / 圆管 / mesh append 冒充连续曲面；T3–T5 不在本 PoC 范围。

### ADR-012 批准的是

- **仅研究行为**：方法研究、文档产出、接口草图、C++17 / NDK / GLES3 可行性论证。
- 硬边界 B-1..B-7：不引入 Three.js / JS 运行时、不做通用 quad remeshing / 通用 SDF sculpting /
  完整 NURBS 编辑器、不改冻结 ABI、不改 RenderThread 生产入口、研究阶段不改生产源码。
- 完成状态记为 `RESEARCH ONLY / NOT A PREREQUISITE` —— **PoC-SM 默认不等待 ADR-012**。

## 3. 明确未批准的事项

| # | 未批准 | 归属 |
|---|---|---|
| N-1 | 不批准任何 Gate 判 PASS | 契约批准 ≠ 实现结果 |
| N-2 | 不批准具体参数默认值 / 体型原型 | OQ-01 / OQ-28（DEC-05 家族，**尚未批准**） |
| N-3 | 不批准改动 `MeshData` 契约、Renderer、RenderThread | SPEC 冻结项 |
| N-4 | 不批准以 ADR-011 为由跳过 `G-P1-GEO` 的 ≤5° 硬条件 | OQ-03 决策记录 |
| N-5 | 不批准把 Saltline 运行时或任何 JS 依赖纳入引擎 | ADR-012 硬边界 B-1 |
| N-6 | 不批准在 ADR-012 研究阶段创建生产代码路径 | ADR-012 硬边界 B-7 |

## 4. 关闭后的影响

| 后果 | 说明 |
|---|---|
| **OQ-04 关闭** | 从 `BLOCKING / DRAFT` → `DONE` |
| **DEC-04 关闭** | SPEC §7.1 `DEC-04`（ADR-011 授权方式）= **APPROVED**（选项 ①：新建 ADR-011，不覆盖 ADR-008/009/010） |
| **PoC-SM 契约依据就位** | `CharacterDefinition` 五层结构成为 PoC-SM 的上游契约 |
| **Saltline 研究边界就位** | 可启动方法研究，但产出**仅限文档与接口草图** |
| **OQ-02（AUTH-08）已独立关闭** | 见 `04-AUTH-08-granted.md` —— ADR 签署**本身不**解锁编码 |
| **`G-P1-GEO` 仍 FAIL** | ADR 是契约批准，不是实现结果；≤5° 仍须在 PoC-SM 中实际达成 |
| **不产生任何代码** | 两份 ADR 均为文档决策 |
