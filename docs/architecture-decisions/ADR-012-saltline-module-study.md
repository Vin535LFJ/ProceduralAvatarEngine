# ADR-012 — Saltline 程序化资源生成方法：独立技术模块研究

> **状态：ACCEPTED — RESEARCH ONLY**（2026-11-24）。研究阶段，不授权纳入生产引擎。
> 依据：负责人 2026-11-23 推进建议第 4 步「先研究通用几何生成、曲面、材质和资源构建方式」。
> **本 ADR 不是 PoC-SM 的强制前置条件。** 完成状态记为 `RESEARCH ONLY / NOT A PREREQUISITE`。

## 决策记录（DECISION RECORD）

| 字段 | 值 |
|---|---|
| 决策项 | ADR-012 状态 |
| 决策结果 | **ACCEPTED — RESEARCH ONLY** |
| 决策人 | 项目负责人 |
| 决策日期 | **2026-11-24** |
| 决策来源 | 负责人在 WorkBuddy 会话中明确选择「签 4 份 + 下发 AUTH-08」执行包 |
| 记录方式 | AI **仅作转录与归档**；未代签，未添加负责人未表达的判断 |
| 草案来源 | `ADR-012-saltline-module-study.draft.md`（SHA256 `9f47d0690ac9ef2eab4305bf2505ef557635555c87cab5e75cb765d446b788a2`，2026-11-23） |

## 背景

项目需要一个可复用的程序化资源生成方法：同一套库应能生成不同体型、风格、服装、头发及其他角色，而不是为每类资产写一套生成器。Saltline 被提出作为方法参考。

同时必须避免两个历史错误：一是直接引入网页运行时依赖（Three.js / JS），把 C++17 + Android NDK + GLES3 引擎变成 Web 栈；二是建设"迷你 Blender"——SPEC §4 明确禁止通用 quad remeshing（§14 延期项）、通用 SDF sculpting 工具与完整 NURBS 编辑器。

## 候选方案

1. **A：先做独立研究 ADR，只抽取真正复用的原语，C++17 原生实现。**
2. B：直接把 Saltline/Three.js 运行时引入引擎。
3. C：不做独立模块，在现有各生成器里各自复制能力。

## 决定

**采纳 A。** 研究阶段只产出文档与接口草图，不改生产源码、不引入新运行时依赖。

## 批准范围（本次批准的唯一内容）

本 ADR **只**批准**研究行为**：

| # | 允许 | 说明 |
|---|---|---|
| A-1 | Saltline 方法研究 | 组织思路、参数化组织方式、原语边界 |
| A-2 | 文档产出 | 研究笔记、接口草图、可行性说明 |
| A-3 | 接口草图 | 与 ADR-011 的 `CharacterDefinition` 五层子契约的对应关系 |
| A-4 | C++17 / NDK / GLES3 可行性论证 | 明确不依赖 GLES compute |

## 研究范围

只评估以下四类原语是否值得 C++17 重写并纳入：

| 类别 | 候选原语 | 与现有契约的关系 |
|---|---|---|
| 几何生成 | 分层 SDF / envelope 构造、参数化截面、guide surface | 对齐 `MeshData` 与 PoC-SM 的 Shared Skin Envelope |
| 曲面 | B-spline / 隐式场求值、法线与曲率采样 | 复用 `SurfaceMetrics` 的采样口径 |
| 材质 | 灰模 Lambert 参数、材质 id 约定 | 不改 64B GPU packing；UV 仍为 CPU metadata |
| 资源构建 | 任务化生成、版本过滤、CPU mesh cache | 已有 Geometry Worker / cache 思路，避免重复建设 |

## 硬边界

| # | 禁止 |
|---|---|
| B-1 | **不引入 Three.js**、JS 引擎或任何网页运行时 |
| B-2 | **不实现通用 quad remeshing** |
| B-3 | **不实现通用 SDF sculpting** |
| B-4 | **不实现完整 NURBS 编辑器** |
| B-5 | 不改 BoneId 0–21、UBO[22]、64B packing 等冻结 ABI |
| B-6 | 不改 RenderThread 的生产渲染入口 |
| B-7 | 研究阶段不修改生产源码 |

## 与 PoC-SM 的依赖关系

**ADR-012 不是 PoC-SM 的强制前置条件**，除非具体实现明确依赖 ADR-012 的研究结果。

若 PoC-SM 的某项具体实现**明确**需要 ADR-012 的研究结论（例如 Shared Skin Envelope 的 guide surface 组织方式采用 Saltline 的某种具体分层法），则在 **AUTH-08 的实施说明中逐项标注该依赖**，并把该依赖项标记为"需先完成 ADR-012 对应研究条目"。

**默认状态：PoC-SM 不等待 ADR-012。**

## 明确不批准的事项

| # | 不批准 | 理由 |
|---|---|---|
| N-1 | 不批准把 Saltline 运行时或任何 JS 依赖纳入引擎 | 硬边界 B-1 |
| N-2 | 不批准在研究阶段创建生产代码路径 | B-7 |
| N-3 | 不批准把研究结论直接当作实现授权 | 实现授权由 AUTH-08 单独签发 |

## 理由

- 先定义可复用原语的边界，再谈重写，避免在形态模型尚未验证时锁定工具链。
- Saltline 是**方法参考**，不是运行时依赖；借的是组织思路，不是代码。
- 与 DEC-09 的 Shape Model 契约互补：契约定义"生成什么"，本 ADR 定义"用什么工具生成"。

## 影响

- 正面：未来体型/风格/服装/头发可共享生成库。
- 负面：多一层接口设计成本；若原语抽取过宽会滑向"迷你 Blender"，必须用硬边界约束。
- 无 ABI / 生产入口影响。

## 验证

1. 研究 ADR 通过负责人审阅。
2. 抽出的一级原语能与 ADR-011 的 `CharacterDefinition` 五个子契约一一对应。
3. C++17 / NDK 25 / GLES3 可行性说明；不依赖 GLES compute。
4. 明确哪些原语**不**纳入，并给出理由。

## 回滚

研究阶段无生产变更；不通过则仅保留文档，不写入引擎。

## 签署

```
ADR-012 状态： DRAFT / ACCEPTED — RESEARCH ONLY / REJECTED
研究范围确认： 仅方法研究 / 文档 / 接口草图（不引入 Three.js、不改生产源码）
与 PoC-SM 关系： 非强制前置（除非实施说明逐项标注具体依赖）
```

**签署结果（2026-11-24）：** `ACCEPTED — RESEARCH ONLY` —— 由负责人在 WorkBuddy 会话中明确批准，AI 转录归档（见文首「决策记录」）。
