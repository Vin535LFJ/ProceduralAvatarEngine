# Owner Sign-off Checklist — G2 冻结后的待办

> 一页看完所有需要负责人给值/签字的事项。AI 不代填任何一项。
> 基线：`a32251b`（G2 证据）；文档提交：`e8e697e`、`92765cb`、`f792d52`、`7c451ae`、`1ce1a21`。
> 当前远端：`origin/feature/procedural-character-foundation`。
> **本版：2026-11-24 —— 5 项已签署，1 项待决。**

## 已完成（无需再处理）

| 项 | 状态 | 证据 |
|---|---|---|
| G2 公平比较证据 | 已冻结并推送 | `a32251b` |
| G3 = DEFER 采纳 | 2026-11-23 负责人确认 | SPEC §7.4 |
| **G3 决策记录签署** | **2026-11-24 签署，`DEFER`，`approved route` 空** | `05-g3-decision-record.md`；`10-decision-records/03-OQ-05-decision-g3-defer.md` |
| DEC-09 采纳分层契约 | `APPROVED` | SPEC §7.1 |
| 09 文档状态 | `ADOPTED IN PRINCIPLE` | 09 文档顶部 |
| 独立审核提示词 | 已交付，可转发审核人 | `02-route-review-prompt.md` |
| 视觉评价协议 | 协议部分完成；判定栏留空 | `04-visual-rubric.md` |
| **ADR-011 签署**（CharacterDefinition 契约） | **2026-11-24 `ACCEPTED`** | `docs/architecture-decisions/ADR-011-shape-model-contract.md` |
| **ADR-012 签署**（Saltline 研究） | **2026-11-24 `ACCEPTED — RESEARCH ONLY`** | `docs/architecture-decisions/ADR-012-saltline-module-study.md` |
| **AUTH-08** | **2026-11-24 `GRANTED`** —— 基线 `a32251b` / worktree `E:/AI-Tools/ProceduralAvatarEngine-poc-sm` / 分支 `poc-sm/shape-model-v2` | `10-decision-records/04-AUTH-08-granted.md` |
| **≤5° 口径固定（OQ-03）** | **2026-11-24：硬条件，不阻止编码** | `10-decision-records/01-OQ-03-decision-seam-hard-condition.md`；SPEC §13.1 |
| AUTH-07 | `EXECUTED` | `a32251b` |
| OQ 编号归并 | 已并入单一 Register，独立 BLOCKING 主项 5 → 1 | `07-open-issues.md` |

## 待负责人给值（**仅剩 1 项主项**）

| # | 事项 | 阻塞什么 | 当前状态 | 负责人填写 |
|---|---|---|---|---|
| **1** | **DEC-05 授权参考图 + rubric 来源**（OQ-01 及其子项 OQ-28/29/30/31/32） | 阶段 1 视觉标准；阶段 3 重跑 G2 后判 PASS；**一切几何验收判定** | `OPEN`；`04-visual-rubric.md` 判定栏全空 | ________________________________ |
| 2 | 独立审核人意见（OQ-06…OQ-13） | 是否维持 DEFER / R2 候选 | 提示词已交付，审核未执行 | 审核人 / 日期 ______ |

> ⚠️ **PoC-SM 现在可以编码（AUTH-08 已签发）。**
> 但 **DEC-05 未批准 → 几何验收与视觉判定仍不可执行**：任何 §13 指标（含 ≤5°）都只能作为
> **测量记录**产出，**不得判 PASS**，也不得宣布"视觉合格"。

## 当前推进状态

```
[已签] ADR-011 + ADR-012  ──┐
[已签] OQ-03 ≤5° 口径      ──┼──→ PoC-SM 编码（已解锁）
[已签] AUTH-08 GRANTED     ──┘         │
                                        ▼
                          Before/After + 接缝复测 + 拓扑审计
                                        │
                                        ▼
                    [待签] DEC-05 / OQ-01 ──→ 几何验收判定 + 重跑 G2
                                        │
                                        ▼
                    [待签] G3 重新批准 ──→ G4
```

## 顺序建议

```
1（参考图 DEC-05）→ PoC-SM 产出证据 → 几何验收判定 → 6（审核意见）→ 阶段 3 重跑 G2 → G3 重新批准 → G4
```

- 缺 1：任何视觉 `PASS` 都不成立；Before/After 只能做相对对照；验收判定不得执行。
- 缺 2（审核意见）：不影响推进，但建议在 **G3 重新批准前**收回，避免返工。
- OQ-27 / OQ-34 / OQ-35（五 Gate 入 SPEC / 面数可核验规则 / 材质光照基线与性能预算）仍 `OPEN`，
  不阻塞编码，但在 G3 重新批准前应给出结论。

## 一句话

**授权已到位，代码可以开工；但判定标准还没到位——DEC-05 是现在唯一真正卡住"能不能说合格"的事。**
