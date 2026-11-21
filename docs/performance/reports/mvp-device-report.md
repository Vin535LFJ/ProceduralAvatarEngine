# MVP Device Performance Report — 基准设备实测报告

- 日期：2026-11-21
- 设备：nunai_max（`MAX0019111000104`）电视盒子，Android 8.1（API 27），arm64-v8a，GLES 3.2（声明）/ Native GLES 3.0 Context
- 构建：`./gradlew :app:assembleDebug`（JDK 17 / AGP 7.4.2 / NDK 25.1.8937393）
- 测试场景定义：`docs/performance/performance-test-plan.md`
- 原始数据：`docs/phases/M9-performance-delivery/evidence/`

## S1 渲染基线（Medium，合成动作自动循环，1920×1008）

| 指标 | 实测值 |
|---|---|
| FPS（EMA） | 47.1–55.4（渲染线程含 16ms 节拍间隔，稳态 ~48 FPS） |
| 帧时间 P50 | 20.7–20.9 ms |
| 帧时间 P95 | 21.1–24.2 ms |
| 帧时间 P99 | 21.3–40.7 ms（瞬态峰值出现在窗口切换/冷启动早期） |
| Draw calls | 2（背景 + 角色） |
| 三角形 | 3053 |
| GPU 缓冲 | 156,944 B |

**结论：30 FPS 目标在基准设备达成（实测 ~48 FPS，P95 21–24ms 远小于 33.4ms 预算）。** 帧时间为 CPU 提交侧实测（steady_clock），设备无 GPU 查询扩展，未虚构 GPU 时间。

## S2 质量档位对比（同一会话内切换）

| 档位 | detailLevel | 三角形 | GPU 缓冲 | FPS | P50 | P95 |
|---|---|---|---|---|---|---|
| Low | 0 | 1645 | 88,848 B | 48.0 | 20.81 | 21.08 |
| Medium | 1 | 3053 | 156,944 B | 47.1–48.4 | 20.86 | 23.29 |
| High | 2 | 4957 | 246,864 B | 48.1 | 20.84 | 21.08 |

三档切换即时生效（`PAE_RENDER quality=... triangles=... vertices=...`），渲染连续、avatarRevision 递增，材质/背景状态保持。瓶颈为渲染线程节拍而非几何量（1645→4957 三角形无帧率差异）；分辨率缩放未实现（设计说明见 `docs/phases/M9-performance-delivery/00-scope-and-assumptions.md`）。

## S3 姿态推理延迟（ML Kit Accurate 18.0.0-beta5，bundled 模型）

| 输入 | 实测单帧推理 |
|---|---|
| 640×480 静态图（盒子 CPU） | 238.79 / 240.38 ms |
| 1920×1080 视频帧 | 208.3 ms（另有 RenderScript YUV→RGB 转换） |

推理频率（~4 FPS）低于渲染帧率，属 Spec 允许且预期的解耦设计；M8 时间轴跳帧策略保证视频域不积压（decoded/skipped/inferred 统计）。

## S4 稳定性长测（12 分钟，30s 采样 ×24）

- 进程全程存活（pidof 恒定），渲染无中断：全程 fps≈47.7–48.2、p50≈20.8ms 恒定。
- Native Heap：87,284 → 87,3xx KB（波动 <100KB，无增长趋势）。
- 设备报告温度：26（`dumpsys battery`，单位粗粒度，仅记录可得项）。
- 详见 `evidence/soak-12min.log`。

## S5 生命周期压力

- force-stop + 冷启动 ×3：0 次崩溃，新进程每次正常重建 EGL/GLES 资源并稳定渲染（帧计数重置后 48 FPS）。
- HOME 键在该盒子不触发 Activity 销毁（观察记录：帧计数连续），作为设备行为记录。

## 已知限制与未验证项

1. 摄像头实时路径未验证（设备无摄像头）——需带摄像头设备复测推理与端到端延迟。
2. 真人姿态驱动效果未验证（无素材）；ML Kit 无人画面存在误报关键点行为（R-11，backlog）。
3. 单设备矩阵（R-07）：以上数据仅代表基准盒子；其他设备不在此报告范围内。
4. 帧率受渲染线程 16ms 节拍限制（~62 FPS 上限），非几何/着色瓶颈；去除节拍属 backlog 优化。
