# Requirements Baseline — M0

状态定义：proposed / approved / designed / implemented / verified / blocked / deferred。优先级：P0 阻断主链路或硬约束；P1 MVP 必需；P2 增强。

| ID | 类型 | 需求名称 | 验收条件（可观察） | 优先级 | 依赖 |
|---|---|---|---|---|---|
| FUN-AV-001 | FUN | 程序化完整卡通人体 | 代码生成头/颈/躯干/双臂双手/双腿双脚；无外部模型；网格验证通过 | P0 | M3, M4 |
| FUN-AV-002 | FUN | 体型/外观参数与预设 | 至少 2 个体型比例、肤色/发色/服装色可运行时切换；颜色变化不重传几何 | P1 | M4, M5 |
| FUN-RIG-001 | FUN | 22 骨 Humanoid Skeleton | Bone ID 0–21 稳定；层级无环；Bind Global/IBM 有效 | P0 | M4 |
| FUN-RIG-002 | FUN | 自动蒙皮权重 | 每顶点 ≤4 影响、非负、和≈1；肘膝有过渡带 | P0 | M4 |
| FUN-RND-001 | FUN | GLES 3.0 Native 渲染 | 创建 EGL/GLES3 Context，记录 GL 字符串与限制；绘制程序化几何 | P0 | M2 |
| FUN-RND-002 | FUN | GPU LBS | std140 UBO 骨骼调色板；GLSL ES 300 每顶点 4 影响蒙皮位置与法线 | P0 | M5 |
| FUN-MOT-001 | FUN | SyntheticPoseProvider | 六类确定性动作可重复驱动角色 | P0 | M6 |
| FUN-MOT-002 | FUN | Retargeting/滤波/约束 | 坐标转换、置信度加权、One Euro、丢点渐退、关节限制有单测 | P0 | M6 |
| FUN-POSE-001 | FUN | IPoseProvider/PoseFrame | 统一接口；时间戳/坐标语义/置信度/版本号齐全；Motion Runtime 无 SDK 类型 | P0 | M1, M6 |
| FUN-POSE-002 | FUN | ML Kit 真实姿态 Adapter | 33 点 → PoseFrame → JNI；Beta/许可/体积/Z 限制披露 | P0 | M7 |
| FUN-IN-001 | FUN | 摄像头实时输入 | CameraX ImageAnalysis，最新帧优先，异常路径关闭 ImageProxy | P1 | M7 |
| FUN-IN-002 | FUN | 本地视频输入 | 文件选择器 + MediaExtractor/MediaCodec；PTS 同步；暂停/拖动/倍速/重播 | P1 | M8 |
| FUN-SCN-001 | FUN | 程序化背景 | SolidColor/Gradient/ProceduralStudio；切换背景不重建角色 | P1 | M5 |
| PERF-RT-001 | PERF | 30 FPS 目标与测量 | 在实测设备记录平均/P50/P95/P99 帧时间；无数据不宣称达标 | P1 | M9 |
| PERF-RT-002 | PERF | 延迟与资源统计 | 推理耗时、端到端延迟、CPU/内存（可获取时）记录 | P1 | M7, M9 |
| COMP-001 | COMP | GLES 3.0 能力检测 | 支持则记录字符串；不支持则明确错误、安全退出，不假装支持 | P0 | M2 |
| COMP-002 | COMP | Android 生命周期 | Surface 创建/变更/销毁、Pause/Resume、Context Lost 可恢复 | P0 | M2, M9 |
| ART-001 | ART | 零外部 3D 美术资产 | 目录扫描无模型/贴图/HDRI；全部资产由代码生成 | P0 | 全程 |
| REL-001 | REL | 线程与资源所有权 | GL 仅 Render Thread；有界队列；缓冲区所有权清晰 | P0 | M1, M2 |
| OBS-001 | OBS | 日志与 RenderStats | 记录 GL 能力、shader 错误完整 source/日志、FPS/draw calls/三角形数 | P1 | M2 |
| TEST-001 | TEST | 单元与集成测试 | Math/Geometry/Skeleton/Weights/Motion 有单测；每阶段保存证据 | P0 | 全程 |
| TEST-002 | TEST | 视觉/设备验收 | 关键动作截图、Surface 重建、稳定性有原始日志；未测项标未验证 | P1 | M4–M9 |

## 非目标（不追踪为需求）
写实人体、毛发/布料物理、逐指与表情捕捉、多人动捕、深度真值重建、通用引擎、全设备帧率承诺。
