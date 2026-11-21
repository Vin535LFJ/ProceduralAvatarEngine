# M9 — 04 测试与验收

- 日期：2026-11-21
- 设备：nunai_max TV 盒子（MAX0019111000104，Android 8.1 / API 27 / arm64-v8a）
- 构建回归：`ctest` 5 套件 24 用例全通过（含新增 `quality_levels_map_to_geometry_detail`）；`./gradlew :app:assembleDebug` BUILD SUCCESSFUL

## 验收结果

| 验收项 | 结果 | 证据 |
|---|---|---|
| 质量三档（Low/Medium/High）运行时切换 | **PASS** | `evidence/logcat-quality-*.txt`：`quality=High triangles=4956 vertices=2906`；三角形 1645/3053/4957，材质/背景状态保持，avatarRevision 递增 |
| detailLevel 映射宿主单测 | **PASS** | `tests/unit/core_tests.cpp`：Low=0/Medium=1/High=2 且高低档几何量不同 |
| 帧时间百分位统计 | **PASS** | `PAE_STATS` 输出 p50/p95/p99（如 Medium：p50=20.86 / p95=23.29 / p99=23.48ms） |
| 30 FPS 目标 | **PASS** | 三档实测均 ~48 FPS（渲染线程节拍上限内），P95 ≤24.2ms << 33.4ms 预算 |
| 12 分钟稳定性长测 | **PASS** | `evidence/soak-12min.log`：24/24 采样，进程存活，fps 47.6–48.2 恒定，Native Heap 87,284→87,308 KB（无泄漏趋势），温度 26 恒定 |
| 生命周期压力（force-stop ×3 冷重启） | **PASS** | 0 崩溃，每次新进程正常重建 EGL/GLES 资源并稳定渲染 |
| 内存/温度可观测性 | **PASS** | `dumpsys meminfo`（TOTAL PSS/Native Heap）+ `dumpsys battery temperature` 每采样记录 |
| MVP 交付物齐备 | **PASS** | `README.md`、`docs/implementation-plan.md`、`docs/performance/{device-matrix,performance-test-plan,reports/mvp-device-report}.md`、`docs/release/mvp-readiness-checklist.md` |
| ART-001 资产审计 | **PASS** | 仓库扫描无模型/贴图/媒体命中（测试证据截图除外）；APK 仅 `assets/mlkit_pose/`（已披露 SDK 模型，16 项 ~9.2MB） |
| RTM/风险/依赖清单终态 | **PASS** | RTM 22 行更新（17 verified / 1 implemented）；risks.md R-01~R-12 终态；dependencies.md 含 RenderScript/MediaCodec/权限披露 |

## 未验证事项（如实记录）

1. **摄像头实时路径**（FUN-IN-001 真机项）：基准盒子无摄像头；需第二台带摄像头设备复测（复现步骤在 M7 文档）。
2. **真人姿态驱动端到端**（M7 静态图/M8 视频）：无合规素材可用（主机网络不可用）；管线与降级路径已验证。
3. **其他设备**：所有数据仅代表基准盒子（R-07），跨设备兼容性不作结论。
4. 渲染分辨率缩放未实现（质量档位当前仅几何细分），设计决策记录于 `00-scope-and-assumptions.md`。

## 阶段门结论

**PASS WITH FOLLOW-UPS**
- 全部 P0 验收通过；M0–M9 全路线闭环完成。
- 遗留项（受控）：两项未验证依赖外部资源（带摄像头设备 / 真人素材），backlog 项见 `docs/implementation-plan.md`。
