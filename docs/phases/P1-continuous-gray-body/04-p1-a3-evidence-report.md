# PAE-P1-001 — P1-A3 Android 灰模与姿态观察证据报告

日期：2026-11-24
状态：实现与证据整理完成，待项目负责人独立验收；本报告不构成 Gate PASS。

## 1. 范围与结论

P1-A3 已将 Canonical Mesh、22 骨骼 LBS、固定姿态、自由相机及 Clay/Wireframe/Normal/Material ID/Weight Heatmap 观察路径接入 Android。Canonical detail 1 在真机生成并显示，拓扑和权重检查未发现 P1-A3 新增失败。

本阶段证明“完整连续灰模可在 Android 运行并观察”。任务书要求的姿态截面周长、局部体积、左右偏差和穿透量化尚未完整执行，因此这些子项列为 **PARTIALLY VERIFIED**，不以截图替代量化验收。

## 2. Canonical 拓扑与权重

最终 Host 专项日志：`evidence/p1-b/host-canonical-tests-final.log`。

| Detail | Canonical vertices | Triangles | Components | Boundary | Non-manifold | Degenerate | Bad weights | Self-intersections |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 0 | 44,508 | 89,020 | 1 | 0 | 0 | 0 | 0 | 0 |
| 1 | 56,476 | 112,956 | 1 | 0 | 0 | 0 | 0 | 0 |
| 2 | 70,390 | 140,784 | 1 | 0 | 0 | 0 | 0 | 0 |

- 自交检测使用三角形 AABB、BVH 宽相位和窄相位，并排除共享顶点三角形。
- detail 0/1/2 的候选对分别为 2,501,119 / 3,806,215 / 4,131,872，窄相位相交对均为 0。
- 测试同时覆盖不同密度边界重采样、相交区域焊接、按 BoneId 合并权重、Top-4 归一化和 Render-to-Canonical 接缝映射。
- 专项结果：12 cases, 0 failures。
- `holes` 是闭合壳 Euler 拓扑报告中的柄/通道计数，不是开放边界；detail 1 报告为 2，`boundary=0`。最终视觉验收仍需确认不存在非预期通道。

## 3. Android 观察证据

P1-A3 原始真机证据目录：`evidence/p1-a3/`。

已保存：

- Bind Pose 正面、背面、左右侧面、左右 45°、后侧角度。
- 全身 Clay、Wireframe、Normal、Material ID、Weight Heatmap。
- 肩部、面部、自由相机及肘、膝、髋、手臂姿态截图。
- 设备、APK、Host 测试和渲染统计日志。

最终 P1-B APK 补充证据：

- `evidence/p1-b/canonical-clay-front.png`
- `evidence/p1-b/canonical-clay-shoulder.png`
- `evidence/p1-b/device-final-canonical.log`

上述最终截图已目视检查：完整身体轮廓、肩颈连接、躯干/骨盆/髋部过渡、手腕和脚踝均可见；Clay 正面与肩部近景未观察到明显裂缝、尖刺或法线翻转。

### 证据版本说明

P1-A3 目录中的阶段 APK SHA-256 为 `7df2069b...357cb`，PID 16149；最终 P1-B APK SHA-256 为 `1f4dfc23...6b2`。两者不是同一个二进制。最终 APK 已重新验证 Canonical detail 1、UV、GPU/CPU 一致性、模式切换和 Surface 重建，但不能把早期 P1-A3 截图追溯为最终 APK 截图。

## 4. 骨骼与姿态

- 骨骼数量和编号保持 0–21。
- CPU Skinning 专项：7 cases, 0 failures，日志 `evidence/p1-b/host-skinning-tests-final.log`。
- GPU/CPU Bind Pose、肘 85°/膝 80°、非均匀模型缩放三组验证均在 P1-B 最终真机日志中通过，详见 `05-p1-b-uv-render-gpu-evidence-report.md`。
- Android 已提供固定姿态截图，但以下任务书量化项未完整执行：
  - 各关节截面的周长/面积和局部体积。
  - 对应边最大分离。
  - 左右偏差。
  - 姿态下穿透和法线翻转的定量统计。

因此姿态项状态为 **PARTIALLY VERIFIED**。

## 5. 性能和环境

最终 detail 1 Canonical/UV 路径稳定样本：

- FPS 约 47.4–47.8。
- frame_ms 约 21.0。
- draw calls 5。
- triangles 112,956。
- buffer bytes 30,725,312。
- 稳定期 p50 约 21.0 ms，p95 约 21.2–21.5 ms，p99 约 21.5–21.9 ms。
- 启动最初两个窗口的 p99 约 47–49 ms，已单独识别为启动抖动，不混入稳定期数据。

设备：`MAX0019111000104` / `nunai_max` / Android 8.1 / API 27 / Mali-T860。应用 surface 为 1920×1008。详细环境、内存和温度见 `evidence/p1-b/device-info-and-temperature-final.txt`、`device-memory-final.txt`。

## 6. 回归与未验证项

- 全量 CTest：17/18 suites passed。
- 唯一失败为 P0 已记录的 `pae_anatomy_tests`：`components=1 boundary=0 nonmanifold=2`。
- 原始日志：`evidence/p1-b/ctest-full-final.log`；P0 对照：`evidence/p0/ctest-baseline.log`。
- 该失败属于既有 M12 解剖预览路径，不是 Canonical detail 1 的拓扑报告；本报告未将其隐藏或改判为通过。
- 早期阶段 APK 与最终 APK 的截图不可混为同一构建证据。
- 最终负责人视觉验收、姿态量化验收及 P1 Gate 决定仍待执行。
