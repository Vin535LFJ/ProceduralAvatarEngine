# Requirements Management

## EL-01 Intake 记录

Requirement ID / Original User Request / User Intent / Expected Outcome / Explicit Constraints / Known Context / Unknowns / Initial Risk / Initial Authorization。

区分：用户明确事实 / 用户期望 / AI 推导假设 / 尚未验证问题。不重复询问用户已提供的信息。不替用户决定技术路线。

## EL-02 Analysis

当前状态：代码实现了什么/实际行为/已有设计/历史实验/已解决/仍存在。

Bug/质量问题：先复现 → Before Baseline → 读源码与测试 → 查调用链数据流 → 区分现象与根因 → 标 HYPOTHESIS → 对高风险假设设计可执行验证。不能仅凭视觉印象重构整套系统。

需求分类：Functional / Non-functional / Architecture / Performance / Visual Quality / Compatibility / Reliability / Security-Licensing / Developer Experience。

## EL-03 Specification（可验证）

唯一 ID（REQ-TEMPLATE-001 / REQ-SHAPE-001 / REQ-RIG-001 / REQ-SKIN-001 / REQ-ANIMATION-001 / REQ-ANDROID-001 / REQ-VISUAL-001）。每项含 ID / Description / Rationale / Source / Priority / Dependencies / Acceptance Criteria / Verification Method / Status。

区分 Functional Requirements / Quality Requirements / Constraints / Non-Goals / Deferred Requirements。

错误需求："手指变形更自然。"
可验收需求："在规定握拳姿态下，验证各指节旋转范围、指间穿透、网格翻面、局部边拉伸、面积变化和真实视觉结果；采用固定相机和测量区域进行 Before/After 对照。"

无法完全量化的视觉需求保留负责人视觉验收。

## 需求追踪链

Requirement → Project Phase → Milestone → Work Package → Task → Code → Test → Evidence → Acceptance Result。任何重要需求必须追踪到最终结果。

模板：[../templates/requirement-intake.md](../templates/requirement-intake.md)、[../templates/requirements-specification.md](../templates/requirements-specification.md)。