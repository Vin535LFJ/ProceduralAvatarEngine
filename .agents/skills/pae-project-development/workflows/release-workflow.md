# Release / Handoff Workflow

适用：阶段完成交付与交接（EL-13）。

1. **交付包**：Phase Plan / WP Completion Report / Implementation Report / Test Report / Integration Report / Evidence Bundle / Review Report / Gate Assessment / Updated Status / Known Issues / Unverified Items / Next Phase Input。
2. **状态更新**：更新 STATUS（动态），核对 HEAD/文档基线一致。历史与当前分开。不擅自改写 MASTER-PLAN/ADR。
3. **交接**：用 [../templates/agent-handoff.md](../templates/agent-handoff.md)。新 Agent 仅靠仓库即可继续，不依赖隐藏会话记忆。
4. **提交/推送（需授权）**：核对暂存/路径/属性/远端；按任务分组；noreply 身份；快进推送后 ls-remote 核实。未授权不 commit/push/force。
5. **独立审查**：Review Agent 核对被审 SHA/分支/任务书版本/Diff/源码/日志/证据/ADR/Gate（不审旧快照套新版本）。
6. **下一阶段输入**：据实规划（EL-14），记录已知问题/未验证项/待授权方向。

技术 Gate 与负责人最终视觉验收分开；视觉 Gate 不代签。