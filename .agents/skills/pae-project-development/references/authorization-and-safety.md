# Authorization & Safety

## 授权级别（每个任务必须标记，默认 READ_ONLY）

READ_ONLY / RESEARCH_ONLY / DESIGN_ONLY / HOST_IMPLEMENTATION / ANDROID_IMPLEMENTATION / INTEGRATION / GOVERNANCE。

**提出建议≠获得实施授权；编写阶段计划≠获得阶段代码修改授权；MASTER-PLAN M1–M8≠所有阶段已授权。**

## 冻结范围（未经负责人明确批准不得改）

LegacyRigV1 / BoneId 0–21 / UBO[22] / 64B Vertex Packing / 旧 JNI ABI / mode-3 Canonical / mode-4 静态默认行为 / PAHT v3 字段语义。

M4 Android 动态以真实仓库状态 + 正式授权记录为准，须 M3 Gate 通过，不得擅自启动。不得默认 Full92 Rig 适配所有 GLES3 设备。

## Repository Preflight（每任务开始）

确认 repo root / branch / HEAD / remote HEAD / ahead-behind / tracked / untracked / ignored / 其他 Agent 状态 / 任务白名单。不得假设工作区干净；不得把用户未跟踪文件当垃圾。

## Git 禁止事项（未明确授权）

reset / clean / 覆盖已有工作区 / 自动 stash / 删除证据 / 删除分支 / merge / rebase / force push / 自动 commit / 自动 push。

提交/推送须：核对暂存内容/路径/文件属性/远端状态；按任务/性质分组不混性质；生产路径改动/分支删除需显式批准；快进推送后用 `git ls-remote` 核实远端一致。

## 文件安全

项目内删文件走回收站接口（host_safe_delete），不用 Remove-Item 直删；删除失败两次即停手报告。不覆盖未提交文件，不丢弃未跟踪文件。

## ADR / 架构变更

新方向标状态：ACCEPTED / PROPOSED / EXPERIMENTAL / REJECTED / DEPRECATED / SUPERSEDED。只有正式批准的决策才成稳定约束。AI 不得把研究建议自动升级为架构决定。

改变已接受架构须：说明现有决策 → 新技术证据 → 比较备选 → 评估兼容性/迁移成本 → 记录回滚 → 形成 ADR 提案 → 等负责人批准 → 批准后更新。不得静默覆盖旧 ADR。

## 人工 Gate

G1/G2/G3/KCR/评分表/视觉验收由负责人裁决。AI 给带证据的诊断建议分，不代签；验收材料留空白裁决栏不代填。历史审批/哈希不得补造，无法验证标"无法验证版本"。