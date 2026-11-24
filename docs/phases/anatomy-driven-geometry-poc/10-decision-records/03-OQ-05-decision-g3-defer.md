# OQ-05 决策记录 — G3 按 `DEFER` 签署

> **状态：已决策 / DECIDED**（2026-11-24）。G3 状态 = **`DEFER`**。
> 关闭 OQ-05：`BLOCKING / DRAFT` → **`DONE`**。
> 来源：`09-blocker-closure-auth08/07-g3-defer-signoff.md` §2 / §3。

## 决策记录（DECISION RECORD）

| 字段 | 值 |
|---|---|
| 决策项 | OQ-05 — G3 决策记录是否按 `DEFER` 签署？ |
| 决策结果 | **`DEFER`**，`approved route` 保持空 |
| 决策人 | 项目负责人 |
| 决策日期 | **2026-11-24** |
| 决策来源 | 负责人在 WorkBuddy 会话中明确选择「签 4 份 + 下发 AUTH-08」执行包 |
| 记录方式 | AI **仅作转录与归档**；未代签，未代填 `approved route` |

## 1. 决策文本（负责人接受）

> 1. **G3 继续 `DEFER`。**
> 2. **`approved route` 保持空。** R1 保留为参考，R2 保留为拓扑投影候选（暂停加码），R3 暂停投入。**不选择生产路线。**
> 3. **重开 G2 的前置补证共四项**，四项齐备后由**负责人**重新决策：
>    - **① DEC-05 完成**（参考体系批准 + Canonical Acceptance Sheet 批准 → OQ-01 关闭）；
>    - **② PoC-SM Before/After 证据齐备**（同相机/同光/同带/同取景的灰模对照）；
>    - **③ 肩根接缝复测**（按固定口径，≤5°）；
>    - **④ 以新 Target Reference 重跑 G2**（E1/E2/E3 分列 + Layer A/B 全指标）。
> 4. **DEFER 的含义**（按 SPEC §7.2）：补证后**重开 G2**，不是"通过"，也不是"暂停且不处理"。

## 2. 正式决策记录位置

完整的 G3 Decision Record（含 Layer A/B 证据摘要、kill 项核查、GPU parity 风险、
DEFER 期间边界 D-1..D-6）已签署并归档于：

**`docs/phases/anatomy-driven-geometry-poc/05-g3-decision-record.md`**

本文件只做 OQ-05 关闭的索引与影响说明。

## 3. 四项补证的当前状态

| # | 补证项 | 状态（2026-11-24） | 阻塞 |
|---|---|---|---|
| ① | DEC-05 完成 | `NOT RUN` | OQ-01、OQ-28、OQ-29、OQ-30、OQ-31、OQ-32 |
| ② | PoC-SM Before/After | `NOT RUN` — AUTH-08 已签发，**待实施** | 已解除 |
| ③ | 肩根接缝复测 | `NOT RUN` — 需新实现 | 已解除（OQ-14 仍 OPEN） |
| ④ | 新 Target Reference 重跑 G2 | `NOT RUN` — 依赖 ①②③ | 已解除（OQ-18 仍 OPEN） |

`[VERIFIED]` **补证 ① 仍未达成**——DEC-05 家族（OQ-01 及其子项）本次**未**签署。
这是 G3 继续 DEFER 的**唯一剩余文档阻塞**；②③④ 的授权阻塞已由 AUTH-08 解除。

## 4. 关闭后的影响

| 后果 | 说明 |
|---|---|
| **OQ-05 关闭** | 从 `BLOCKING / DRAFT` → `DONE`（状态 `DEFER`） |
| **GATE-04 状态 = `DEFER`（已签署）** | SPEC §7.4 更新；`approved route` 留空 |
| **G4 仍不可启动** | SPEC 要求 G3 已签署**且**视觉 Gate 通过；`G-FINAL-VIS` 为 `NOT RUN` |
| **`G-P1-GEO` 仍 FAIL** | G3 是投资决策，不是几何验收结果 |
| **PoC-SM 编码不因此解锁** | 解锁来自 AUTH-08，不是 G3 签署 |
