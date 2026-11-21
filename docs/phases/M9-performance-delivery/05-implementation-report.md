# M9 阶段报告：性能、稳定性与 MVP 交付

## 1. 阶段结论
- 状态：**PASS WITH FOLLOW-UPS**
- 完成的需求 ID：PERF-RT-001、PERF-RT-002、COMP-003、REL-002（verified）；ART-001、TEST-002（verified）
- 未完成/阻塞需求 ID：无 BLOCKED；未验证项见 04 文档

## 2. 需求与设计
- 本阶段确认的需求：见 `00-scope-and-assumptions.md`（01 部分合并记录）
- 关键设计决定：
  - 质量档位以 `BodyParameters.detailLevel`（0/1/2）为杠杆，映射函数宿主可测
  - 渲染分辨率缩放不实现（视口缩放需处理宽高比/黑边，"必要时"条款下延后）
  - `RenderStats` 增加环形缓冲帧时间百分位（P50/P95/P99）
- 新增/更新 ADR：无新增（质量分级不改变架构决策）

## 3. 代码变更
- 修改文件：
  - `native/core/config/EngineConfig.{h,cpp}`（detailLevelForQuality）
  - `native/renderer/RenderThread.{h,cpp}`（SetQuality 命令、几何重建、revision）
  - `native/renderer/stats/RenderStats.{h,cpp}`（百分位）
  - `native/runtime/EngineRuntime.{h,cpp}`（setQuality）
  - `app/src/main/cpp/jni_bridge.cpp`、`app/src/main/java/com/pae/host/NativeBridge.kt`（JNI）
  - `app/src/main/java/com/pae/host/MainActivity.kt`（Quality 按钮）
  - `tests/unit/core_tests.cpp`（新增质量映射测试）
- 删除文件：无

## 4. 验证证据
- 构建命令与结果：`ctest` 5/5（24 用例）；`./gradlew :app:assembleDebug` BUILD SUCCESSFUL
- 设备/模拟器配置：nunai_max TV 盒子（Android 8.1/API 27/arm64-v8a）
- 性能测量：见 `docs/performance/reports/mvp-device-report.md`（~48 FPS、P50 20.8ms、质量对比、12 分钟 soak、冷重启 ×3、内存/温度）
- 证据文件：`evidence/logcat-quality-{medium,low}.txt`、`evidence/soak-12min.log`、`evidence/frame-m9-final.png`
- 未执行验证及原因：摄像头路径（无摄像头设备）、真人驱动（无素材）、跨设备（单设备矩阵）

## 5. 风险与遗留项
- 已知问题：渲染线程 16ms 节拍限制帧率上限（~62 FPS，非瓶颈）；ML Kit 无人画面误报（R-11 backlog）
- 风险：R-07（单设备）、R-12（真人素材）保持 open，均为外部资源依赖
- 需要用户决定的事项：是否提供带摄像头设备 / 真人测试素材以关闭两项未验证；是否授权 git 提交

## 6. 项目收尾状态
- M0–M9 全部闭环；MVP 清单（`docs/release/mvp-readiness-checklist.md`）14 项全部核对
- 可复现命令：README.md（构建/安装/运行/测试）
