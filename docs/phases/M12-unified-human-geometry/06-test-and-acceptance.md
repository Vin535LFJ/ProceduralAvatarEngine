# M12-A — 06 测试与验收报告（已执行，待用户视觉验收 Gate A）

执行日期：2026-11-22。分支 `m12-unified-human-geometry`。

## 1. 宿主测试（ctest，build/host-verify）

```
100% tests passed, 0 tests failed out of 8      (47 个用例全绿)
pae_anatomy_tests ............................ Passed  (4 个 M12-A 用例)
```

`pae_anatomy_tests.exe` 实测关键输出：

| 指标 | 实测值 | 断言 |
|---|---|---|
| 连通分量 components | **1** | ==1 |
| 边界边 boundaryEdges | **0**（水密） | ==0 |
| 非流形边 nonManifoldEdges | **0** | ==0 |
| 退化三角 / 重复顶点 | **0 / 0** | <1% / ==0 |
| 三角形（测试分辨率 48×72×32） | 9,920 | — |
| 网格包围盒 | x[−0.402, 0.402] y[0.688, 1.723] z[−0.102, 0.136] | 上半身 ~0.80m 宽 / 1.03m 高 / 0.24m 厚 |
| 胸廓场值（内部） | −0.065（负=内部） | <0 |
| 体外场值 probe(−0.25,1.15,0.30) | +0.195（正=外部） | >0 |
| 胸廓轮廓（射线扫描，chestWidth 0.8→1.3） | 0.155m → **0.320m** | Δ>0.03 |
| 肩宽（deltoidVolume 0.4→1.6） | 0.260m → **0.309m** | Δ>0.01 |
| 确定性 | 两次提取 memcmp 完全一致 | == |
| 蒙皮 | 权重和=1，joints<BoneCount，前臂顶点归属 LowerArm 骨 | — |

## 2. 真机渲染证据（nunai_max / MAX0019111000104，Android 8.1，GLES 3.0）

### 2.1 Anatomy Preview Mode

- SDF 上半身**真正接入生产渲染路径**：`RenderThread::ensureResources` 调用
  `anatomy::generateAnatomyUpperBody`（默认分辨率 72×112×44）并上传 GPU；
- 渲染侧确认日志：`PAE_RENDER applied anatomyPreview=1 / cameraPreset=N / debugView=N`；
- 真机网格：**24,360 三角形**，draw_calls=3（背景+阴影+解剖体，线框/法线分支 2–3）；
- 性能：`PAE_STATS frames=... fps≈51.7 frame_ms p50=19.3ms p95=19.9ms p99=20.1ms`
  （远超 30FPS 目标，解剖预览模式下稳定）。

### 2.2 截图矩阵（evidence/ 目录，24 张）

`dv{0,1,2}_preset{0..7}.png` = 3 调试视图 × 8 固定相机：

- dv0 = 中性着色（neutral 0.72,0.66,0.60，6 级 toon，灯光 (0.42,0.78,0.55)）
- dv1 = 线框（去重边 GL_LINES，绿色 (0.05,0.35,0.08)，glDepthRangef 拉前）
- dv2 = 法线（n*0.5+0.5）
- preset 0..7 = 正/背/左/右/45左/45右/肩特写/躯干特写

采集方式为确定性驱动：`adb shell am start -n com.pae.host/.MainActivity --ez debug_anatomy true --ei debug_preset P --ei debug_view D`，每次冷启动后轮询截图直至非黑屏（body coverage>1%）再保存。

### 2.3 像素级客观核验（tools/png_probe.py，纯 zlib PNG 解码统计）

| 核验项 | 结果 |
|---|---|
| dv0 着色 | 6–7 个 toon 色阶，主色 (130,130,104)/(104,104,104)/(52,52,52) = base×shade，正面 mean (112,103,93)、背面更暗 (68,62,56)（灯光方向正确） |
| dv1 线框 | 主导色 (0,78,0)≈(0.05,0.35,0.08) 绿线，占比 53–82% |
| dv2 法线 | 192–227 个不同色（全色域），正面偏蓝、背面偏绿、侧面偏红 — 法线方向分布正确 |
| **左右对称**（bodypx） | dv0 L/R 98,982 vs 98,995（Δ0.013%）；45L/45R 149,667 vs 149,725（Δ0.039%）；dv1/dv2 同样 <0.04% —— 镜像 min 对称性在像素级证实 |
| 各预设覆盖率 | 5.13%（侧）~20.54%（躯干特写），全部 >1% 非黑屏 |

## 3. 技术选型汇报（ADR-009）

见 `docs/architecture-decisions/ADR-009-unified-sdf-upper-body.md`：
上半身采用**统一 SDF 场合成 → Surface Nets**（方案 B），取代 ADR-008 的局部补丁路线。
决定依据 = M11 实证（SDF 未入生产、merge≠fusion）+ 本次拓扑/对称/真机数据。

## 4. 仍然存在的问题（诚实清单）

1. **上半身与下半身的交界（髋/腰）尚未焊接** —— M12-B 待办；当前预览只覆盖上半身。
2. Surface Nets 在极端参数下可能产生少量 sliver 退化三角（测试容忍 <1%，当前实测 0）。
3. 分辨率限制细节（默认档 ~5–9mm/体素）；头部为占位椭球，面部/头发属 M12-B。
4. 本报告所有「视觉质量是否达标」的判断**不由 AI 作出** —— 24 张截图 + 像素统计仅供客观参考，最终由用户看图确认（Gate A）。
5. 摄像头实时路径仍受设备限制（无摄像头），与 M7–M11 同属已知未验证项。

## 5. 性能数据汇总

| 项目 | 数值 |
|---|---|
| 解剖网格生成 | 单次初始化（异步），符合 Spec §11 |
| 真机三角形 | 24,360（72×112×44） |
| 真机 FPS / 帧耗时 | ~51.7 FPS，p50=19.3ms p95=19.9ms p99=20.1ms |
| draw calls（预览） | 3（着色/法线）或 2+线框 |
| APK | assembleDebug 通过 |

## 6. 验收状态

- 代码/构建/宿主测试/真机运行/视觉证据：**全部完成**（本报告 §1–§2）。
- **Gate A（用户视觉验收）：待用户确认。** 按 M12 约束，用户未确认 Gate A 通过之前，
  不继续扩展 M12-B（完整人体/面部/头发/服装/Shader）。

证据文件：`docs/phases/M12-unified-human-geometry/evidence/dv{0,1,2}_preset{0..7}.png`（24 张）。