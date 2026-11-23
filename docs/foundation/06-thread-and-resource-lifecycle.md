# 06 — Thread & Resource Lifecycle Contract（P0）

## Geometry Generation Thread

- 纯 C++ 计算，不依赖 EGL / GL Context；可在生成线程池或异步任务中执行。
- 输出完整不可变 `MeshData`（含顶点、索引、SubMesh、AABB、region id、boundary tag）；生成完成后不再原地修改。
- 几何生成器禁止调用 OpenGL / EGL API；禁止从 RenderThread 每帧重新生成整个人体网格。
- 参数变化时可异步生成新网格；每项生成任务携带 `GenerationVersion`（单调递增值）。

## Render Thread

- 唯一持有当前 EGL Context；只有它可做 GL 对象创建、上传、销毁与绘制。
- 收到新 `MeshData` 后，在有效 EGL Context 下完成 VBO/VAO/IBO/UBO 上传；上传完成前不切换可见角色。
- 旧生成任务完成时若其 `GenerationVersion < 当前版本`，直接丢弃，不得覆盖更新版本角色。
- EGL Context 重建后，必须能从保留的 CPU `MeshData` 恢复全部 GPU Buffer；CPU 资源生命周期不得绑定 GL Context。
- 资源释放顺序：GL 对象释放 → CPU MeshData 释放；SurfaceDestroyed 时释放 GPU 资源但保留 CPU 源数据以便恢复。

## JNI / Kotlin 边界

- UI 线程只投递命令或参数，不调用 GL。
- PoseFrame/参数通过有界容量-1 队列投递；不阻塞渲染。
- 大量几何生成不得发生在 UI 线程或 RenderThread 的绘制帧内。

## 调试/证据

- P1-A 需记录 generator thread、upload thread、generation version、生成耗时与上传耗时。
- 证据存放 `docs/phases/P1A-geometry-poc/evidence/`，与固定相机截图对应。

## 不做的事（P0）

- 不在 P0 重构现有 RenderThread 或全量替换 MeshData 结构；本契约只约束 P1-A 新增代码路径。
- 不为“结构完整”而盲目改 64 字节 packing；UV 是否进入 GPU 由 P3 材质链路审计决定。
