# M9 — 00 范围与假设 / 01 需求 / 02 设计 / 03 任务拆解（合并记录）

- 日期：2026-11-21
- 设备：nunai_max TV 盒子（MAX0019111000104，Android 8.1 / API 27 / arm64-v8a / GLES 3.2）——设备矩阵仅此一台
- 范围：Low/Medium/High 质量等级接线；RenderStats 帧时间百分位（P50/P95/P99）；真机性能与稳定性测量（质量对比、10 分钟+ 长测、Surface 重建压力、内存）；MVP 交付物（README、implementation-plan、性能报告、MVP 清单、资产审计、RTM 终态）
- 非目标：跨设备优化（仅一台设备）、GLES 2.0 降级、后处理管线、GPU 计时（设备无对应扩展时不虚构）

## 01 需求

- **PERF-RT-001**（P0）：30 FPS 目标在基准设备实测；报告渲染帧率、帧时间百分位。验收：设备报告含实测 FPS + P50/P95/P99。
- **PERF-RT-002**（P1）：延迟与资源统计——推理耗时（M7/M8 已测）纳入报告；内存（dumpsys meminfo）与温度（可获取项）。验收：报告含内存曲线起止值与温度可得性说明。
- **COMP-003 质量等级**（P1，Spec §12.2）：Low/Medium/High 改变几何细分（detailLevel 0/1/2），运行时可切换；切换重建几何并保留材质/背景状态。验收：真机三档切换日志显示三角形数变化且渲染连续。
- **REL-002 稳定性**（P1）：10 分钟以上连续运行无崩溃、无明显内存增长；Surface 反复重建不崩溃。验收：长测日志 + meminfo 对比 + 重建压力日志。
- **TEST-002 / ART-001**（P0）：资产审计（仓库+APK 无外部模型/贴图）；RTM 终态；README 与 implementation-plan 齐备。验收：审计扫描记录 + 文件存在。

## 02 设计

- `EngineConfig` 增加 `uint32_t detailLevelForQuality(QualityLevel)`（Low=0/Medium=1/High=2）——宿主可测。
- `RenderThread` 新增 `SetQuality` 命令：存储 quality；触发生成 `BodyParameters{detailLevel=映射值}` 重建 humanoid + GPU mesh 上传 + `++avatarRevision_`；日志记录质量名与三角形数。默认 Medium 保持现状。
- 渲染分辨率缩放**不实现**（视口缩放会造成画面变形或黑边；Spec §12.2 的"必要时"条款下，MVP 以几何细分作为质量主杠杆，决策记录于设计说明）。
- `RenderStats` 增加帧时间环形缓冲（256 帧）与 P50/P95/P99 计算，输出到 toString() 与周期日志。
- JNI：`nativeSetQuality(handle, level)`；MainActivity `Quality` 按钮循环三档。
- 测量方法：PAE_STATS 周期日志（fps/frame_ms）+ 新增百分位；`dumpsys meminfo`（Java/Native Heap）；`dumpsys battery`（温度可获取项）；surface 重建压力 = 反复 Home→启动循环。

## 03 任务拆解

| 任务 | 文件范围 | 测试命令 | DoD |
|---|---|---|---|
| T1 质量等级接线 | `EngineConfig.{h,cpp}`, `RenderThread.{h,cpp}`, `EngineRuntime.{h,cpp}`, `jni_bridge.cpp`, `NativeBridge.kt`, `MainActivity.kt`, `tests/unit/core_tests.cpp` | ctest + 构建 + 真机三档切换 | detailLevel 映射有单测；真机三角形数随档位变化 |
| T2 RenderStats 百分位 | `native/renderer/stats/RenderStats.{h,cpp}` | ctest（toString 含 P50/P95/P99 字段） | 周期日志输出百分位 |
| T3 真机性能测量 | 无代码（测量） | adb logcat/dumpsys/screencap | 质量对比数据 + 长测 + 重建压力 + 内存 |
| T4 交付物 | `README.md`, `docs/implementation-plan.md`, `docs/performance/*`, `docs/release/*`, RTM/风险/状态 | 审计脚本扫描 | MVP 清单逐项核对 |
