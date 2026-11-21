# Implementation Plan — 阶段状态与剩余工作

更新：2026-11-21（M9 执行中）。总路线 M0–M9 依《开发指导计划 v1.0》推进，每阶段执行"需求→设计→任务拆解→实施→验证→阶段门"闭环。

| 阶段 | 状态 | 关键交付 | 阶段门 |
|---|---|---|---|
| M0 项目发现与需求基线 | PASS | 仓库审计、需求基线、RTM、风险清单、构建基线 | 2026-11-21 复核（ctest 5/5 + assembleDebug 新鲜证据） |
| M1 架构细化与工程骨架 | PASS | 目标架构、模块契约、线程/生命周期、ADR-001~003 | 2026-11-21 复核 |
| M2 GLES 渲染底座 | PASS | EGL/GLES3、能力检测、RenderThread、命令队列 | 真机渲染验证 |
| M3 程序化几何核心 | PASS | 数学库、图元/Sweep/Loft、MeshValidator | ctest + 真机 |
| M4 参数化人体与骨架 | PASS | Humanoid、22 骨骨架、自动权重 | ctest + 真机 |
| M5 GPU Skinning 与材质背景 | PASS | UBO LBS、程序化材质、三背景 | 真机（颜色切换不重建几何） |
| M6 Motion Runtime | PASS | PoseFrame、Retargeting、滤波、合成 Provider | ctest + 真机六动作 |
| M7 摄像头 + 真实姿态 Provider | **PASS WITH FOLLOW-UPS** | ML Kit 适配、相机回退、静态图链路验证（推理 238ms/帧） | 真机摄像头路径未验证（盒子无摄像头） |
| M8 本地视频输入 | **PASS WITH FOLLOW-UPS** | VideoPoseSource、PTS 时钟、播放控制、滤波重置（ADR-007） | 真人素材端到端未验证 |
| M9 性能、稳定性与 MVP 交付 | **IMPLEMENTING** | 质量三档、帧时间百分位、性能报告、MVP 清单、README | 见下方剩余工作 |

## 剩余工作（M9 内）

1. 12 分钟稳定性长测数据归档（进行中）。
2. `docs/performance/reports/mvp-device-report.md`（质量对比 + 长测 + 内存/温度）。
3. `docs/release/mvp-readiness-checklist.md` 逐项核对。
4. ART-001 资产审计扫描（仓库 + APK 内容）。
5. RTM 终态更新（PERF-RT-001/002、COMP-003、REL-002、ART-001、TEST-002）与 `docs/risks.md` 更新。

## Backlog（MVP 后）

- 人物存在性判定（最少关键点数 + 几何合理性）抑制 ML Kit 无人画面误报。
- 视频推理降采样 / 分辨率协商（当前 1080p 全分辨率推理 ~200ms/帧）。
- 渲染分辨率缩放（当前质量档位仅几何细分；视口缩放需处理宽高比）。
- 摄像头路径真机验证（需带摄像头设备）；设备矩阵扩展。
- 播放状态按钮文字在 ENDED 时的同步（外观）。
