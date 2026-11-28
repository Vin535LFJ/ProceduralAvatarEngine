# Test & Evidence Protocol

## 验证类型（按 WP 目标匹配）

Unit / Host CTest / Regression / Numerical / Topology / Geometry / Pose / CPU-GPU Parity / Android NDK Build / Real Device / Lifecycle / Performance / Visual Comparison。

测试前核对当前测试目标、CMake 配置、覆盖；**不沿用历史测试数量作为当前事实**。

## 必须区分的等价误区

- 编译成功 ≠ 功能正确
- 单元测试通过 ≠ 复杂人体姿态正确
- Bind Pose 正确 ≠ Extreme Pose 正确
- 权重归一化 ≠ 权重解剖正确
- Host 通过 ≠ Android 通过
- 拓扑连通 ≠ 人体视觉自然
- 技术 PASS ≠ 视觉 PASS

## Evidence Protocol（几何/人体/形态/骨骼/蒙皮/动画/视觉任务必须生成本轮真实证据）

按任务选择：固定视角图片 / Clay / Wireframe / Normal / Weight Heatmap / Joint Closeup / CSV / 测试日志 / 性能数据 / Before-After / Git Diff。

**可比性（强制）**：相同基础网格 / 相同参数输入 / 相同姿态 / 相同相机 / 相同测量 Mask / 相同质量指标口径。

禁止：用旧图冒充新图；通过移动测量 Mask 掩盖问题；只报全身平均掩盖局部严重变形。

**负结果必须保存。**

## 质量指标

翻面数必须同时报翻面率（flips/faces，跨分辨率可比）。自交数注明检测实现与条件。区域测量用固定 Mask。区分几何穿透 / 表面折叠 / 体积损失。逐关节区域报告（Shoulder/Axilla、Elbow、Wrist、Fingers、Hip/Groin、Knee、Foot/Toes）。

## 视觉验收

无法完全量化的视觉需求保留负责人人眼 Gate。图像描述模型输出仅供参考，不替代人眼。技术 Gate 与最终视觉验收分开。

## 证据存放

`docs/phases/VR-1B/evidence/<task>/` 或 `docs/phases/P1-visual-recovery/evidence/<task>/`。PNG 用仓库 `tools/human-template-poc/host/tools/recompress_png.py` 无损重压缩（像素往返校验）。证据目录加 `.gitattributes` `-text` 规则保证跨平台字节一致。