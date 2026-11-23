# Risk Register Index

完整风险登记保留在 `docs/risks.md`（R-01…R-12）。P0 新增/变更：

| ID | 风险 | 状态 | 处理 |
|---|---|---|---|
| R-01 | ADR-008/009 与 v1.0 主线定位冲突 | MITIGATED | ADR-010：SDF 可做局部辅助研究，但不作为 P1 最终动画人体唯一生产来源 |
| R-02 | Shoulder/UpperArm 语义重叠 | MITIGATED | P0 最小修复 + 回归测试 + 真机对照；待用户 P0 Gate |
| R-03 | nunai_max 无摄像头 | BLOCKED | 视频链路可验；手机摄像头测试前不得标 DEVICE VERIFIED |
| R-04 | UV 未进入 64 字节 VAO | PRELIMINARY | 完整链路审计后再判定；P3 需要时写 ADR |
| R-05 | bindLocal 仅平移是否足以表达 A-Pose | PRELIMINARY | 审计 BoneDefinition/Bind Global/IBM/SkinPalette/GLSL，确认缺失后再写 ADR |
