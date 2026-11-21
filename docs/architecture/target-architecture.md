# Target Architecture — M1

## 模块依赖（单向）
```
Kotlin Host (app/)
  → Input Adapters (CameraX / Video)
  → Pose Provider Adapter (ML Kit → PoseFrame) ─┐
                                               ├→ Motion Runtime (C++) → BonePalette
Native JNI Bridge ← Surface/Lifecycle           │
  → RenderThread Command Queue ← Avatar/Scene params
       → EglContextManager / GlCapabilities / ShaderProgram / GpuMesh
       → Procedural Core (geometry/skeleton/materials/scene, CPU only)
```

禁止反向依赖：Motion Runtime 不依赖 SDK 类型或 Android UI；Procedural Core 不调用 GL；Renderer 不做推理/网格生成；GL 调用只在 Render Thread。

## 数据流
1. Input Adapter 产出带时间戳的帧；实时模式容量 1（latest-value），视频模式以 PTS 为基准。
2. Pose Provider 产出 `PoseFrame{ timestampNs, coordinateSpace, landmarks[], confidence, hasPerson }`。
3. Motion Runtime 转换坐标 → Retarget → 约束/滤波 → `BonePalette`（最多 22 mat4）+ 时间戳。
4. Avatar Runtime 保存 `AvatarParameters` revision；只有体型变化才触发几何/骨架重建；颜色变化只改材质参数。
5. Render Thread 消费命令，上传变化的 GPU 资源，绘制背景与角色，输出 `RenderStats`。

## 线程归属
| 线程 | 职责 |
|---|---|
| UI/Main | 权限、Activity/Surface 回调、用户设置；不做推理/生成/GL |
| Camera/Decode | CameraX ImageAnalysis 或 MediaCodec 帧生产 |
| Inference | ML Kit 推理，独立 Executor，最多一个在途帧 |
| Procedural Worker | 初次生成与体型变化重建；可异步 |
| Render Thread | 独占 EGL Context 与全部 GLES 调用；有界命令队列 |

## 资源所有权
- CPU 端 `MeshData` / `Skeleton` / 参数由 Avatar Runtime 持有；GPU 端 VAO/VBO/IBO/UBO/Program 由 Render Thread 持有，RAII 管理。
- Context Lost：GPU 对象全部失效；从 CPU 描述重建；不假设 GL ID 有效。
- `ImageProxy` 在推理完成或异常路径关闭；Native 缓冲有明确 release。

## 坐标与单位
右手系；+Y 上、+Z 角色正面、+X 角色左侧；米。内部角度用弧度。矩阵列主序，变换按列向量 `M*v` 约定；矩阵乘法链 `Parent * Local`。详见 ADR-002。
