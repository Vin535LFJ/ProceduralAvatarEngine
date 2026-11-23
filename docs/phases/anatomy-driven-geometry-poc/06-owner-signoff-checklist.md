# Owner Sign-off Checklist — G2 冻结后的待办

> 一页看完所有需要负责人给值/签字的事项。AI 不代填任何一项。
> 基线：`a32251b`（G2 证据）；文档提交：`e8e697e`、`92765cb`。
> 当前远端：`origin/feature/procedural-character-foundation`。

## 已完成（无需再处理）

| 项 | 状态 | 证据 |
|---|---|---|
| G2 公平比较证据 | 已冻结并推送 | `a32251b` |
| G3 = DEFER 采纳 | 2026-11-23 负责人确认 | SPEC §7.4 |
| DEC-09 采纳分层契约 | `APPROVED` | SPEC §7.1 |
| 09 文档状态 | `ADOPTED IN PRINCIPLE` | 09 文档顶部 |
| 独立审核提示词 | 已交付，可转发审核人 | `02-route-review-prompt.md` |
| 视觉评价协议 | 协议部分完成；判定栏留空 | `04-visual-rubric.md` |
| ADR-011 / ADR-012 草案 | DRAFT 完成 | `docs/architecture-decisions/` |
| G3 决策记录草案 | DRAFT 完成，`approved route` 空 | `05-g3-decision-record.draft.md` |
| AUTH-07 | `EXECUTED` | `a32251b` |

## 待负责人给值（阻塞后续阶段）

| # | 事项 | 阻塞什么 | 当前状态 | 负责人填写 |
|---|---|---|---|---|
| 1 | **DEC-05 授权参考图 + rubric 来源** | 阶段 1 视觉标准；阶段 3 重跑 G2 后判 PASS | `OPEN`；`04-visual-rubric.md` 判定栏全空 | ________________________________ |
| 2 | **AUTH-08 基线 commit + worktree 路径** | 阶段 2 PoC-SM 代码实施 | `OPEN`；AI 不写 PoC-SM 代码 | ________________________________ |
| 3 | **ADR-011 签署**（CharacterDefinition 契约） | PoC-SM 契约定稿 | DRAFT | 签字 ______ 日期 ______ |
| 4 | **ADR-012 签署**（Saltline 研究） | 独立模块研究启动 | DRAFT | 签字 ______ 日期 ______ |
| 5 | **G3 决策记录签署** | 重开 G2 的正式起点 | DRAFT / DEFER | 签字 ______ 日期 ______ |
| 6 | **独立审核人意见** | 是否维持 DEFER / R2 候选 | 提示词已交付，审核未执行 | 审核人 / 日期 ______ |

## 顺序建议

```
1（参考图）→ 3/4（ADR 签署）→ 2（AUTH-08 授权）→ PoC-SM 实施
                    ↓
              5（G3 签署）→ 6（审核意见）→ 阶段 3 重跑 G2
```

- 缺 1：任何视觉`PASS`都不成立；Before/After 只能做相对对照。
- 缺 2：不能动 PoC-SM 代码（AUTH-07 只覆盖 R2/R3 最小实现，不覆盖新 Shape Model）。
- 缺 3/4：契约与研究边界仍是草案，实施无依据。
- 缺 5：G3 无正式签署，G4 不得启动。
- 缺 6：不影响推进，但建议在 G3 签署前收回，避免返工。

## 一句话

代码和证据都在远端、G3 已记 DEFER；现在球在负责人这边——**先给参考图，再谈授权写代码**。
