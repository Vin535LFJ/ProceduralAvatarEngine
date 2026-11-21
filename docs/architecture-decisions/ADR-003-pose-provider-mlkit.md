# ADR-003：姿态识别 Provider 选择（ML Kit）

- 日期：2026-11-21
- 状态：accepted（用户指定）
- 背景：需要一个真实姿态 Provider；设备为 API 27 电视盒子，需披露许可/体积/离线能力。
- 候选：(A) ML Kit Pose Detection 18.0.0-beta5；(B) MediaPipe Tasks Vision 1.2.3（Apache-2.0、minSdk 28、.task 模型约 5.5MB lite）；(C) 延迟真实 SDK。
- 决定：采用 (A)，经 `IPoseProvider` 适配，Kotlin 推理后以 JNI 推送统一 `PoseFrame`。
- 披露（必须随实现保留）：**Beta、无 SLA/弃用策略**；许可证为 **ML Kit Terms of Service（专有，非 Apache-2.0）**；minSdk 23；AAR 静态链接算法模型，APK 增重约 **10.1 MB（base）/13.3 MB（accurate）**；端侧推理，但条款允许为修复/模型更新/兼容性联系 Google 服务；输出 33 点，**Z 为实验性像素值、不是真实 3D 坐标，无 world landmarks**。
- 理由：用户指定；minSdk 23 覆盖 API 27 设备；Provider 抽象隔离 SDK 类型。
- 影响：Motion Runtime 不使用 Z 作米制真值；文档/UI 禁止称“精确 3D 动捕”。若产品改为“APK 不含任何模型二进制”，需另立 ADR 评估替代部署。
- 回滚/替代：切换到 (B) 需 minSdk≥28 与 .task 模型资产披露；接口不变。
