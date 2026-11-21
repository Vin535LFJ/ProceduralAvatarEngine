# Project Charter — Procedural Avatar Engine

- 版本：v1.0（M0）
- 状态：已批准基线（依据用户 2026-11-20/21 的决定）

## 项目定义
为 Android 手机与 Android TV/电视盒子构建的轻量级实时卡通数字人引擎：摄像头/本地视频 → 人体姿态 → 统一 `PoseFrame` → Motion Runtime → 代码生成的 3D 人体与骨架 → 自研 GLES 3.0 渲染器。不是通用游戏引擎，不是模型播放器。

## 硬性约束
- C++17 核心、Kotlin 宿主、JNI、Gradle + NDK + CMake。
- EGL + OpenGL ES 3.0 自研 Renderer，GLSL ES 3.00；禁止 Filament/Unity/Unreal/Godot/Three.js。
- 零外部 3D 美术资产：禁止 GLB/GLTF/FBX/OBJ/DAE、外部角色/场景贴图、HDRI、预制动画；全部几何/骨骼/权重/材质/场景由代码和参数生成。
- 姿态识别 SDK 作为可插拔软件依赖：允许其算法模型二进制，但必须披露版本、许可证、体积、离线能力与状态；禁止称之为精确真实 3D 动捕。
- 右手坐标系，+Y 上、+Z 前、+X 角色左侧，单位米；22 骨版本化骨架；每顶点 ≤4 影响；std140 UBO 上传 Bone Palette。
- 确定性程序化生成；生成阶段与实时渲染阶段分离；GL 资源仅由 Render Thread 管理。
- GLES 3.0 为最低渲染目标，运行时能力检测；不做 GLES 2.0 降级。

## MVP 成功定义
GLES 3.0 Context 可创建；程序化卡通人体与骨架/权重由代码生成；GPU Skinning 工作；Synthetic 与真实 Provider 均可通过 Motion Runtime 驱动；外观/背景可切换；无外部 3D 美术资源；可构建、可测试、可运行，未验证项如实记录。

## 已确认的用户决定
| 决定 | 内容 |
|---|---|
| 推进方式 | M0→M9 全程自主推进，每阶段门出具报告，不暂停等待确认 |
| 姿态 SDK | ML Kit Pose Detection `18.0.0-beta5`；Beta/许可/体积/Z 限制须在 ADR-003 与依赖清单披露 |
| 设备验证 | 允许向在线设备 `MAX0019111000104`（nunai_max）安装 debug APK、采集日志/截图/性能数据；不改系统设置、不卸载 |
| Git | 允许 `git init`；未经授权不 commit、不 push、不 reset |

## 优先级与冲突处理
用户最新且合规的决定 > 技术规范 > 开发指导计划 > AI 实施提示词 > Agent 优化建议。架构路线变更必须经 ADR，不得静默改变。

## 非目标（MVP）
写实人体、毛发/布料物理、逐指动作与表情捕捉、多人动捕、遮挡补全、通用引擎功能、全设备帧率保证。
