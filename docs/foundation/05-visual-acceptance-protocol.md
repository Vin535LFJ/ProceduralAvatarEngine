# 05 — Visual Acceptance Protocol（P0 冻结）

## 固定条件（所有 Gate 必须一致）

- 设备：同一台 Android 设备；记录型号、Android 版本、CPU/GPU、GLES 版本、APK SHA。
- 分辨率：记录实际截图分辨率；不缩放/裁切后冒充原始证据。
- 相机：使用固定 camera preset id、FOV、角色缩放、目标点；记录 `cameraPreset_` 值与矩阵参数。
- 光照：统一主光方向、环境光、材质；几何验收用 gray Lambert（连续灰模），toon 只能作为补充。
- 背景：固定纯色或程序化背景，不用外部图片。
- 姿势：Gate 固定姿势；动作证据记录 action id 与时间戳。
- 质量档位：固定 detail level；Before/After 必须同档位。

## 必交视图

- 基础：front、back、left、right、45° left、45° right。
- 关节特写：shoulder、elbow、hip、knee；P1-A 至少要有 shoulder/chest close-up。
- Debug：wireframe、normals、skeleton、weight heatmap。
- 动作：按阶段要求的动作序列截图或短录屏帧；不得用静态图片冒充动态。
- 对照：P1-A 需与 M11/M12 固定相机 Before/After；P3 每次视觉改动需 Before/After。

## 状态与 Gate

- 阶段结论只用：PASS / PASS WITH FOLLOW-UPS / BLOCKED / NOT VERIFIED。
- 需求状态只用：IMPLEMENTED / TESTED / DEVICE VERIFIED / VISUALLY ACCEPTED / NOT VERIFIED / BLOCKED。
- AI 可以分析视觉缺陷并提出修复，但不得自行把用户 Gate 标为 VISUALLY ACCEPTED。
- P1-A 提交证据后停在 Gate，等用户确认视觉方向；未确认不进入 P1-B。
- 用户 Gate：P1-A、P1-B、P1-C、P1-D/P1 最终、P2、P3、P4、P5。

## 证据存放

- `docs/phases/<Phase>/evidence/`；图片为真实 Android GLES 输出，文件名含 preset/debug/action。
- 文本证据：构建/测试输出、logcat、统计（FPS/P50/P95/P99、gen_ms、vertex/triangle、memory、GPU buffer、native heap）。
- 每阶段报告必须引用证据绝对路径或仓库相对路径，并注明设备与采集命令。
