# Performance Test Plan — 性能测试计划

更新：2026-11-21（M9）。目标：Spec §12 的 30 FPS 渲染目标与可观测性指标在真实设备上的可复现测量。

## 测量场景

| 场景 | 配置 | 数据来源 |
|---|---|---|
| S1 渲染基线（合成动作自动循环） | Medium，1920×1008 Surface | `PAE_STATS` 周期日志（fps/frame_ms/p50/p95/p99/draw_calls/triangles/buffer_bytes） |
| S2 质量档位对比 | Low / Medium / High 各 ≥10s | `PAE_RENDER quality=...` + `PAE_STATS` |
| S3 姿态推理延迟 | 静态图单帧 + 视频逐帧 | `PAE_POSE pose stats` / `PAE_VIDEO video stats` |
| S4 稳定性长测 | ≥10 分钟连续渲染，30s 采样 | pidof + `PAE_STATS` 尾帧 + `dumpsys meminfo`（TOTAL PSS / Native Heap）+ `dumpsys battery temperature`（可获取项） |
| S5 Surface/进程生命周期 | force-stop + 冷启动 ×3 | 崩溃计数（FATAL EXCEPTION）+ 新进程帧计数 |

## 环境

- 必须记录：设备型号、Android 版本、分辨率、质量档位、姿态输入源（合成/静态图/视频）、测试时长。
- 帧时间百分位为渲染线程实测（steady_clock），非 GPU 计时——设备无 GPU 查询扩展时不虚构 GPU 时间（Spec §12.3）。
- 未测设备不得写结论；无法获取的指标（如温度）如实标注"不可获取"。

## 通过基线（MVP）

- 基准设备渲染 ≥30 FPS（实测 ~48 FPS）。
- P95 帧时间 ≤33.4ms（30 FPS 帧预算）。
- 长测期间 PSS 增长无失控趋势（<20% 且无崩溃）。
