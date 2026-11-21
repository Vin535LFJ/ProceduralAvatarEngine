# Device Matrix — 设备准入与验证矩阵

更新：2026-11-21（M9）

| 设备 | 类型 | Android | ABI | GLES | 验证状态 |
|---|---|---|---|---|---|
| nunai_max（`MAX0019111000104`） | 电视盒子（leanback-only） | 8.1（API 27） | arm64-v8a | 3.2（声明）/ 3.0 Native Context 实测可用 | **已验证**：渲染/合成动作/静态图推理/视频链路/质量三档/长测 |
| 带摄像头的 Android 手机 | 手机 | — | — | ≥3.0 | **未验证**（无可用硬件）——M7 摄像头实时路径待测 |

已知设备事实：
- 盒子无可用摄像头（Manifest 声明与运行时不符，运行时为准）；无系统 DocumentsUI（文件选择走 ADR-007 回退）。
- 摄像头路径与真人姿态驱动的复现步骤见 `docs/phases/M7-pose-provider-camera/04-test-and-acceptance.md`。

准入规则：GLES 3.0 Context 创建失败时给出明确错误并停止渲染（M2 实现）；不按设备名称硬编码能力。
