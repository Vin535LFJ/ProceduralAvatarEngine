# MVP Readiness Checklist — 发布就绪核对

日期：2026-11-21。对照《开发指导计划 v1.0》§12 逐项核对。

| # | 项目 | 状态 | 证据 |
|---|---|---|---|
| 1 | 所有 P0 需求状态为 verified | **PASS** | RTM：P0 项（FUN-AV-001/RIG/RND/MOT/POSE-001/IN-002/SCN-001/ART-001）均 verified；FUN-IN-001 implemented（硬件限制，见 #7） |
| 2 | 零外部 3D 美术资产审计通过 | **PASS** | M9 资产扫描：仓库无模型/贴图/媒体文件；APK 仅 `assets/mlkit_pose/`（已披露 SDK 推理模型） |
| 3 | GLES 3.0 能力检测与不支持提示 | **PASS** | M2 真机 logcat（GlCapabilities 记录 + 不支持时明确错误） |
| 4 | 程序化几何/骨架/权重均有测试 | **PASS** | ctest 5 套件 24 用例（geometry/humanoid/skinning/motion/core） |
| 5 | GPU Skinning 在真实程序化人体网格上工作 | **PASS** | M5 真机截图 + UBO std140 单测 |
| 6 | Synthetic Pose 端到端动作验证 | **PASS** | M6 六类动作真机截图（motion-pose-1..5.png） |
| 7 | 至少一个真实 Pose Provider 接入 | **PASS WITH 未验证项** | ML Kit 接入并经静态图验证（238ms/帧真机推理）；**摄像头实时路径未验证：基准盒子无摄像头**；真人素材端到端未验证（无素材） |
| 8 | 摄像头和本地视频输入分别验证 | **部分** | 视频路径真机验证通过（M8）；摄像头路径未验证（见 #7） |
| 9 | 皮肤、头发、服装颜色与背景切换 | **PASS** | M5 真机（colors-and-gradient.png）+ M7/M8 回归运行 |
| 10 | Surface/Context/Activity 生命周期 | **PASS** | M9 冷重启 ×3 零崩溃；M2 重建日志 |
| 11 | 性能报告有实际设备数据 | **PASS** | `docs/performance/reports/`（本次测量）；未测设备无虚构结论 |
| 12 | 依赖清单/许可证/包体说明齐备 | **PASS** | `docs/dependencies.md`（ML Kit 18.0.0-beta5 披露：Beta、专有条款、bundled 模型 ~9.2MB、离线推理） |
| 13 | README 有构建/安装/运行/测试步骤 | **PASS** | 根目录 `README.md` |
| 14 | 已知问题/限制/backlog 记录 | **PASS** | `docs/implementation-plan.md` backlog + 各阶段 04/05 报告 |

## 结论

MVP 交付就绪，附两项如实标注的未验证项（摄像头实时路径、真人素材驱动），均因硬件/素材不可得，复现步骤已写入对应阶段文档。
