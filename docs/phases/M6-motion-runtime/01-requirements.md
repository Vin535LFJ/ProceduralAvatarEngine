# M6 — 01 需求

- FUN-POSE-001：统一 PoseFrame/IPoseProvider；时间戳、坐标语义、置信度、版本号。
- FUN-MOT-001：六类确定性合成动作可重复驱动角色。
- FUN-MOT-002：坐标转换、Retargeting、滤波、关节限制、低置信度/丢点恢复有单测。
- REL-001：容量 1 最新值缓冲，无积压。
- 验收：六类动作端到端驱动真实程序化网格；左右映射正确；Motion Runtime 无 ML Kit / Android UI 依赖。
