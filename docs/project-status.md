# Project Status

- 更新：2026-11-21（M9 阶段门关闭，M0–M9 全路线闭环）
- 当前阶段：**MVP 交付完成**（PASS WITH FOLLOW-UPS）
- 上一阶段：M9 — **PASS WITH FOLLOW-UPS**（质量三档/百分位统计/12 分钟长测/冷重启压力/资产审计全部通过）

## 最终状态

- 需求：23 项 ID 化，**17 项 verified**、1 项 implemented（FUN-IN-001 摄像头路径——设备无摄像头，复现步骤已写入 M7 文档）、其余按子项归并。
- 构建/测试：`ctest` 5 套件 24 用例全绿；`./gradlew :app:assembleDebug` 可复现；APK ~56MB（含 ML Kit bundled 模型）。
- 真机（nunai_max TV 盒子）验证：渲染 ~48 FPS（P50 20.8ms / P95 21–24ms）、合成动作六类、ML Kit 推理 208–240ms/帧（静态图/1080p 视频）、**真人姿态驱动端到端（Shaka "Angel One" 480p 短片：conf 0.77/0.88/0.98，角色完整跟随站立/坐姿/马步动作，2026-11-21 补验）**、视频链路（解码/PTS/播放控制/滤波重置/ENDED 状态修复）、质量三档切换、12 分钟 soak（Native Heap +24KB 无泄漏、温度 26 恒定）、冷重启 ×3 零崩溃、零外部美术资产审计。
- 未验证（外部资源依赖）：摄像头实时路径（需带摄像头设备）、跨设备兼容性。
- 阻塞：无。
- 下一步：backlog 见 `docs/implementation-plan.md`（存在性判定、推理降采样、分辨率缩放等）。
- **M10 视觉提升迭代（2026-11-21，用户反馈触发）**：卡通描边（蒙皮感知 inverted hull）、卡通脸（眼白/虹膜/高光/眉/嘴/鼻）、刘海发型、躯干 7 截面平滑化、四肢锥度与关节球收敛、手部拇指、比例卡通化、细分提升（Medium 7537 三角形）、地面接触阴影、配色暖色化——全部程序化实现，ctest 全绿，FPS 无回归（47.7+），见 `docs/phases/M10-visual-polish/`。

## M11 程序化人体解剖与视觉质量重构（2026-11-21，分支 m11-anatomy-visual-rebuild）

- **A-H 完成，I 收尾中**：CatmullRom 样条扫掠/放掠（椭圆截面/roll/锥度法线/端盖/UV 接缝修复）、SDF 融合（Surface Nets，PoC 三法对比 → ADR-008 混合定案）、人体生成器重写（BodyShapeParams 全参数 + 关节球全移除 + 髋缝消除）、WeightGenerator 区域权重（跨部位连续 + 骨骼桩修正）、FaceGenerator（剖面调制头形+全五官）、HairGenerator（3 发型）、ClothingGenerator（衬衫/长裤）、调色板 5 色、重定向修复（置信度门限/缺失保持/坐标协议）。
- 视觉矩阵：P0 维度（轮廓/躯干/肩/肢/变形/绑定位/参数生效）全部 ≥4；面部/发型/服装/材质 3-4 分（深化列 backlog）。
- 性能：Medium 14065 三角形（预算内）、50-53 FPS、P95 21.5ms——无回归。
- 详见 `docs/phases/M11-anatomy-visual-rebuild/`（05 = 视觉矩阵评分 + 实施报告）。

## 阶段门记录

- M0/M1 复核（2026-11-21）：ctest 5/5 + assembleDebug 新鲜证据（`docs/phases/M0-project-discovery/evidence/*reverify-20261121.txt`）。
- M7：PASS WITH FOLLOW-UPS——ML Kit 链路静态图验证（推理 238ms/帧）；相机三选择器回退；盒子无摄像头项标未验证。
- M8：PASS WITH FOLLOW-UPS——视频链路（MediaCodec→YUV→RenderScript→ML Kit→PTS 驱动）真机验证；EOS 语义修复；播放控制全部带滤波重置。
- M9：PASS WITH FOLLOW-UPS——质量三档（1645/3053/4957 三角形）、P50/P95/P99、12 分钟 soak（无泄漏）、冷重启 ×3、资产审计通过。
